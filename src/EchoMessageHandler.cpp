#include <iostream>
#include <cstring>
#include "EchoMessageHandler.h"

void EchoMessageHandler::deserialize(const std::vector<uint8_t>& requestData, EchoMessage& echoRequest) {
    // Ensure the requestData is large enough to contain at least the fixed-size parts
    if (requestData.size() < sizeof(Header) + sizeof(unsigned)) {
        throw std::runtime_error("requestData too short to contain an EchoRequest");
    }

    // Copy the fixed-size parts (Header and messageSize)
    std::memcpy(&echoRequest.header, requestData.data(), sizeof(Header));
    std::memcpy(&echoRequest.messageSize, requestData.data() + sizeof(Header), sizeof(unsigned));

    // The rest of the data is the cipherMessage
    echoRequest.message.assign(
        requestData.begin() + sizeof(Header) + sizeof(unsigned),
        requestData.end()
    );
}

void EchoMessageHandler::serialize(const EchoMessage& echoRequest, std::vector<uint8_t>& buffer) {

    // Resize the buffer to the total size
    buffer.resize(echoRequest.header.requestSize);

    // Serialize the fixed-size parts
    std::memcpy(buffer.data(), &echoRequest.header, sizeof(echoRequest.header));
    std::memcpy(buffer.data() + sizeof(echoRequest.header), &echoRequest.messageSize, sizeof(echoRequest.messageSize));

    // Append the variable-size part
    if (!echoRequest.message.empty()) {
        std::memcpy(buffer.data() + sizeof(echoRequest.header) + sizeof(echoRequest.messageSize),
                    echoRequest.message.data(),
                    echoRequest.message.size());
    }
}

// Used by a client test program for debugging
std::string EchoMessageHandler::getStringMessage(const EchoMessage& echoRequest) {
    return std::string(echoRequest.message.begin(), echoRequest.message.end());
}

// Used by a client test program for debugging
void EchoMessageHandler::print(const EchoMessage &echoRequest) {
    std::cout << "Echo "
                << (echoRequest.header.requestType == 2 ? "Request"
                    : echoRequest.header.requestType == 3 ? "Response"
                        : "Unknown type") << ":" << std::endl
                << '\t' << "Sequence -> " << echoRequest.header.requestSequence << std::endl
                << '\t' << "Message -> " << EchoMessageHandler::getStringMessage(echoRequest) << std::endl
                << '\t' << "Header size -> " << sizeof(echoRequest.header) << std::endl
                << '\t' << "Other fixed size -> " << sizeof(echoRequest.messageSize) << std::endl
                << '\t' << "Message size -> " << echoRequest.messageSize << std::endl
                << '\t' << "Request total size -> " << echoRequest.header.requestSize << std::endl;
}