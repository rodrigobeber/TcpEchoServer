#pragma once

#include <netinet/in.h>
#include <unordered_map>
#include "ThreadPool.h"
#include "ClientHandler.h"
#include <shared_mutex>
#include <unordered_set>
#include <sys/epoll.h>

class Server {
private:
    int server_fd, epoll_fd;
    struct sockaddr_in address;
    size_t port;
    size_t maxQueuedConnections;
    ThreadPool threadPool;
    bool isRunning;

    std::unordered_map<int, std::shared_ptr<ClientHandler>> clientHandlers; // Map to maintain client states
    std::unordered_set<int> clientsProcessing; // Set to track if a socket is being processed
    std::mutex clientsProcessingMutex;
    std::mutex clientHandlersMutex;

    void monitorSocket(int socket);
    void setupSocket();
    void bindSocket();
    void startListening();
    void acceptConnections();
    void setupEpoll();
    void handleEvents(int events_count, struct epoll_event* events);

    int acceptNewConnection();
    std::shared_ptr<ClientHandler> getClientHandlerInstance(int socket);
    void processClient(int socket);
    void cleanupClient(int socket);

public:
    Server(size_t port, size_t maxQueuedConnections, size_t poolSize);
    ~Server();
    void run();
    void stop();
};