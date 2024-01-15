#include <thread>
#include "Server.h"

int main(int argc, char* argv[]) {

    // Default settings
    size_t port = 8080;
    size_t poolSize = std::thread::hardware_concurrency(); // Number of CPU cores available
    size_t maxQueuedConnections = 100;

    // Run the server
    Server server(port, maxQueuedConnections, poolSize);
    server.run();

    return 0;
}