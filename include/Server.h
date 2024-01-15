#pragma once

#include <netinet/in.h>
#include <unordered_map>
#include "ThreadPool.h"
#include "ClientHandler.h"
#include <shared_mutex>
#include <unordered_set>

class Server {
private:
    int server_fd, max_sd;
    struct sockaddr_in address;
    size_t port;
    size_t maxQueuedConnections;
    ThreadPool threadPool;
    bool isRunning;
    fd_set master_set;

    std::unordered_map<int, std::shared_ptr<ClientHandler>> clientHandlers; // Map to maintain client states
    std::unordered_set<int> clientsProcessing; // Map to track if a socket is being processed
    std::mutex clientsProcessingMutex;
    std::mutex clientHandlersMutex;

    void setupSocket();
    void bindSocket();
    void startListening();
    void acceptConnections();

    int acceptNewConnection();
    std::shared_ptr<ClientHandler> getClientHandlerInstance(int socket);
    void processClient(int socket);
    void cleanupClient(int socket);
    ssize_t checkSocket(int socket);

public:
    Server(size_t port, size_t maxQueuedConnections, size_t poolSize);
    ~Server();
    void run();
    void stop();
};