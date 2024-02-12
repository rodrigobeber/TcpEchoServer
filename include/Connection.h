#pragma once

#include <vector>

enum class OperationStatus {
    Success,
    Error,
    ConnectionClosed
};

class Connection {
public:
    static OperationStatus receiveData(int socket, std::vector<uint8_t>& buffer);
    static OperationStatus sendData(int socket, const std::vector<uint8_t>& buffer);
};

