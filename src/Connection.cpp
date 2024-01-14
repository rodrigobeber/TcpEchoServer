#include <vector>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include "Connection.h"

OperationStatus Connection::receiveData(int socket, std::vector<uint8_t>& buffer) {
    size_t totalBytesRead = 0;

    // Expected size is always the buffer size that is already resized
    while (totalBytesRead < buffer.size()) {
        ssize_t bytesRead = read(socket, buffer.data() + totalBytesRead, buffer.size() - totalBytesRead);

        if (bytesRead < 0) {
            std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
            return OperationStatus::Error;
        } else if (bytesRead == 0) {
            std::cerr << "Connection closed by the peer." << std::endl;
            return OperationStatus::ConnectionClosed;
        }

        totalBytesRead += bytesRead;
    }

    if (totalBytesRead < buffer.size()) {
        std::cerr << "Partial data received." << std::endl;
        return OperationStatus::Partial;
    }

    return OperationStatus::Success;
}

OperationStatus Connection::sendData(int socket, const std::vector<uint8_t>& buffer) {
    size_t totalBytesSent = 0;

    while (totalBytesSent < buffer.size()) {
        ssize_t bytesSent = send(socket, buffer.data() + totalBytesSent, buffer.size() - totalBytesSent, 0);
        if (bytesSent < 0) {
            std::cerr << "Error sending data: " << strerror(errno) << std::endl;
            return OperationStatus::Error;
        } else if (bytesSent == 0) {
            std::cerr << "Connection closed by the peer." << std::endl;
            return OperationStatus::ConnectionClosed;
        }

        totalBytesSent += bytesSent;
    }

    return OperationStatus::Success;
}