#include <iostream>
#include <cstring>
#include "Connection.h"
#include "EchoMessageHandler.h"
#include "Protocol.h"

bool Protocol::readLogin(int socket, LoginRequest &request) {
    // Read the request
    std::vector<u_int8_t> buffer(sizeof(LoginRequest));
    if (Connection::receiveData(socket, buffer) != OperationStatus::Success) {
        std::cerr << "Error reading login request." << std::endl;
        return false;
    }
    std::memcpy(&request, buffer.data(), sizeof(LoginRequest));

    // Check type
    if (request.header.requestType != static_cast<unsigned>(MessageType::LoginRequest)) {
        std::cerr << "Message read is not a login request." << std::endl;
        return false;
    }

    // Check consistency
    if (request.header.requestSize != sizeof(LoginRequest)) {
        std::cerr << "Inconsistent login request header." << std::endl;
        return false;
    }

    std::cout << "Login successful." << std::endl;

    return true;
}

OperationStatus Protocol::readHeader(int socket, Header& header) {
    // Read the header
    std::vector<u_int8_t> buffer(sizeof(Header));
    OperationStatus status = Connection::receiveData(socket, buffer);
    if (status == OperationStatus::Success) {
        std::memcpy(&header, buffer.data(), sizeof(Header));
    }
    return status;
}

bool Protocol::responseLogin(int socket, unsigned sequence) {

    // Prepare the request
    LoginResponse response;
    response.header.requestType = static_cast<unsigned>(MessageType::LoginResponse);
    response.header.requestSequence = sequence;
    response.header.requestSize = sizeof(LoginResponse);
    response.statusCode = 1;
    std::vector<uint8_t> buffer(sizeof(LoginResponse));
    std::memcpy(buffer.data(), &response, sizeof(LoginResponse));

    return Connection::sendData(socket, buffer) == OperationStatus::Success;
}

bool Protocol::readEchoMessage(int socket, EchoMessage &request, MessageType type) {

    // Read header, verify if connection was closed (no more messages)
    Header header;
    OperationStatus status = Protocol::readHeader(socket, header);
    if (status != OperationStatus::Success) {
        return false;
    }

    // Check type
    if (header.requestType != static_cast<unsigned>(type)) {
        std::cerr << "Echo message have a wrong type." << std::endl;
        return false;
    }

    // Read remaining data
    std::vector<uint8_t> remainingData(header.requestSize - sizeof(Header));
    if (Connection::receiveData(socket, remainingData) != OperationStatus::Success) {
        std::cerr << "Error reading the rest of the message." << std::endl;
        return false;
    }

    // Concat fixed part + remaining part
    std::vector<uint8_t> buffer(header.requestSize);
    std::memcpy(buffer.data(), &header, sizeof(Header));
    std::memcpy(buffer.data() + sizeof(Header), remainingData.data(), remainingData.size());

    // Deserialize
    EchoMessageHandler::deserialize(buffer, request);

    return true;
}

bool Protocol::sendEchoMessage(int socket, EchoMessage &request) {

    // Prepare buffer
    std::vector<uint8_t> buffer(request.header.requestSize);
    EchoMessageHandler::serialize(request, buffer);

    // Send
    if (Connection::sendData(socket, buffer) != OperationStatus::Success) {
        std::cerr << "Error sending echo request." << std::endl;
        return false;
    }

    return true;
}

