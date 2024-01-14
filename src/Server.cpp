#include "Server.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include "ThreadPool.h"
#include "ClientHandler.h"

Server::Server(size_t port, size_t maxQueuedConnections, size_t poolSize)
    : port(port), maxQueuedConnections(maxQueuedConnections), isRunning(false), threadPool(poolSize) {
    setupSocket();
}

Server::~Server() {
    stop();
}

void Server::setupSocket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        throw std::runtime_error("Socket creation failed");
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        throw std::runtime_error("setsockopt failed");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
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
    while (isRunning) {
        socklen_t addrlen = sizeof(address);
        int client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_socket < 0) {
            // Get the error message
            int err = errno;
            std::cerr << "Accept failed: " << strerror(err) << std::endl;

            // Decide whether to continue or stop based on the error
            if (err == EAGAIN || err == EWOULDBLOCK) {
                // Non-critical errors, can continue
                continue;
            } else {
                // Critical error, might want to stop the server
                std::cerr << "Critical error encountered. Stopping server." << std::endl;
                stop();
                break;
            }
        }

        // Use thread pool to handle the connection
        if (threadPool.getThreadCount() > 0) {
            threadPool.execute([client_socket]() {
                ClientHandler clientHandler(client_socket);
                clientHandler.handle();
            });
        } else {
            ClientHandler clientHandler(client_socket);
            clientHandler.handle();
        }
    }
}

void Server::run() {
    if (isRunning) {
        std::cerr << "Server is already running!" << std::endl;
        return;
    }

    isRunning = true;
    bindSocket();
    startListening();

    size_t poolSize = threadPool.getThreadCount();
    if (poolSize > 0) {
        std::cout << "Using a thread pool of size " << poolSize << '!' << std::endl;
    } else {
        std::cout << "Using single thread." << std::endl;
    }

    #ifdef USE_XOR_CIPHER
    std::cout << "Decryption enabled!" << std::endl;
    #else
    std::cout << "Decryption disabled." << std::endl;
    #endif

    acceptConnections();
}

void Server::stop() {
    if (isRunning) {
        isRunning = false;
        close(server_fd);
    }
}