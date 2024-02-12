#include "Server.h"
#include "ClientHandler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <stdexcept>

Server::Server(size_t port, size_t maxQueuedConnections, size_t poolSize)
    : port(port), maxQueuedConnections(maxQueuedConnections), isRunning(false), threadPool(poolSize) {
    setupSocket();
    setupEpoll();
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
        close(epoll_fd);
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

void Server::setupEpoll() {

    // Create an epoll instance and obtain an epoll file descriptor.
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        throw std::runtime_error("Epoll creation failed");
    }

    // Start monitoring server socket
    monitorSocket(server_fd);
}

void Server::acceptConnections() {
    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    while (isRunning) {
        // Wait for events on any of the file descriptors.
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n < 0) {
            std::cerr << "Epoll wait error: " << strerror(errno) << std::endl;
            continue;
        }
        handleEvents(n, events);
    }
}

void Server::handleEvents(int events_count, struct epoll_event* events) {
    char buffer[1]; // Buffer for peeking
    for (int i = 0; i < events_count; ++i) {
        if (events[i].data.fd == server_fd) {
            acceptNewConnection();
        } else {
            // Peek at the socket to check for data
            ssize_t bytesPeeked = recv(events[i].data.fd, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
            if (bytesPeeked > 0) {
                // Data is available, process the client
                processClient(events[i].data.fd);
            } else if (bytesPeeked == 0 || (bytesPeeked < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                // No data or an error occurred, handle accordingly
                // If bytesPeeked == 0, the connection has been closed by the client
                cleanupClient(events[i].data.fd);
            }
            // If bytesPeeked < 0 and errno == EAGAIN or EWOULDBLOCK, just skip to the next event
        }
    }
}

void Server::monitorSocket(int socket) {
    // Initializes structure to specify the interest in read events on the new socket.
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN; // Interested in input events
    event.data.fd = socket;

    // Add the new socket to epoll monitoring.
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket, &event) < 0) {
        std::cerr << "Failed to add new socket to epoll" << std::endl;
        close(socket);
    }
}

int Server::acceptNewConnection() {
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(client_address);
    int new_socket = accept(server_fd, (struct sockaddr *)&client_address, &addrlen);

    if (new_socket >= 0) {

        // Set the new socket to non-blocking mode to handle it asynchronously.
        fcntl(new_socket, F_SETFL, O_NONBLOCK);

        // Start monitoring new socket
        monitorSocket(new_socket);

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
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket, NULL);
        clientHandlers.erase(socket);
    }
    close(socket);
}

