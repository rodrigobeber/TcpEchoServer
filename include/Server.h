#pragma once

#include <netinet/in.h>
#include "ThreadPool.h"

class Server {
private:
    int server_fd;
    struct sockaddr_in address;
    size_t port;
    size_t maxQueuedConnections;
    ThreadPool threadPool;
    bool isRunning;

    void setupSocket();
    void bindSocket();
    void startListening();
    void acceptConnections();

public:
    Server(size_t port, size_t maxQueuedConnections, size_t poolSize);
    ~Server();
    void run();
    void stop();
};