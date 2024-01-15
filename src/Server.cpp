#include "Server.h"
#include "ClientHandler.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <stdexcept>

Server::Server(size_t port, size_t maxQueuedConnections, size_t poolSize)
    : port(port), maxQueuedConnections(maxQueuedConnections), isRunning(false), threadPool(poolSize) {
    setupSocket();
}

Server::~Server() {
    stop();
}

void Server::run() {
    isRunning = true;

    bindSocket();
    startListening();

    std::cout << "Listening on port " << port << std::endl;
    std::cout << "Using a thread pool of size " << threadPool.getThreadCount() << std::endl;

    acceptConnections();
}

void Server::stop() {
    if (isRunning) {
        isRunning = false;
        close(server_fd);
    }
}

void Server::setupSocket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        throw std::runtime_error("Socket creation failed");
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        throw std::runtime_error("setsockopt failed");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Set the socket to non-blocking mode
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
}

void Server::bindSocket() {
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Bind failed");
    }
}

void Server::startListening() {
    if (listen(server_fd, maxQueuedConnections) < 0) {
        throw std::runtime_error("Listen failed");
    }
}

void Server::acceptConnections() {

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    max_sd = server_fd;

    while (isRunning) {

        // Copy master_set to a temporary set for select() call.
        fd_set read_set = master_set;

        // Use select() to wait for activity on any socket. It modifies read_set to indicate ready sockets.
        if (select(max_sd + 1, &read_set, nullptr, nullptr, nullptr) < 0) {
            std::cerr << "Select error: " << strerror(errno) << std::endl;
            continue;
        }

        // Iterate over file descriptors to check which ones are ready.
        for (int i = 0; i <= max_sd; ++i) {
            // Check if the current file descriptor is set in read_set and not being processed.
            if (FD_ISSET(i, &read_set)) {
                // If the ready socket is the server socket, means a new connection.
                if (i == server_fd) {
                    // Accept new connection
                    if (int new_socket = acceptNewConnection(); new_socket >= 0) {
                        processClient(new_socket);
                    }
                } else {
                    ssize_t socketStatus = checkSocket(i);
                    // Check if client disconnected before processing request.
                    if (socketStatus == 0) {
                        std::cout << "Client " << i << " logged out" << std::endl;
                        cleanupClient(i);
                    } else if (socketStatus > 0 && clientsProcessing.find(i) == clientsProcessing.end()) {
                        std::cout << "Client " << i << " echo requested" << std::endl;
                        processClient(i);
                    }
                }
            }
        }
    }
}

ssize_t Server::checkSocket(int socket) {
    char buffer[1];
    return recv(socket, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
}


int Server::acceptNewConnection() {
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(client_address);
    int new_socket = accept(server_fd, (struct sockaddr *)&client_address, &addrlen);
    if (new_socket >= 0) {
        // Set the new socket to non-blocking mode to handle it asynchronously.
        fcntl(new_socket, F_SETFL, O_NONBLOCK);
        // Add the new socket to the master file descriptor set for monitoring.
        FD_SET(new_socket, &master_set);
        // Update the maximum socket descriptor if the new socket is greater.
        max_sd = std::max(max_sd, new_socket);
    }
    return new_socket;
}

void Server::processClient(int socket) {
    {
        // Turn on processing flag for this client
        std::lock_guard<std::mutex> lock(clientsProcessingMutex);
        clientsProcessing.insert(socket);
    }
    std::shared_ptr<ClientHandler> clientHandler;
    {
        // Get the state of the client
        std::lock_guard<std::mutex> lock(clientHandlersMutex);
        clientHandler = getClientHandlerInstance(socket);
    }
    // Offload executing in a separated thread
    threadPool.execute([this, socket, clientHandler]() {
        if (!clientHandler->processRequest()) {
            cleanupClient(socket);
        }
        std::lock_guard<std::mutex> lock(clientsProcessingMutex);
        // Turn off processing flag for this client
        clientsProcessing.erase(socket);
    });
}

// Get an instance of the client state or create one if it does not exist.
std::shared_ptr<ClientHandler> Server::getClientHandlerInstance(int socket) {
    std::shared_ptr<ClientHandler> clientHandler;
    auto it = clientHandlers.find(socket);
    if (it == clientHandlers.end()) {
        clientHandler = std::make_shared<ClientHandler>(socket);
        clientHandlers.emplace(socket, clientHandler);
    } else {
        clientHandler = it->second;
    }
    return clientHandler;
}

void Server::cleanupClient(int socket) {
    {
        std::lock_guard<std::mutex> handlerLock(clientHandlersMutex);
        FD_CLR(socket, &master_set);
        clientHandlers.erase(socket);
    }
    close(socket);
}

