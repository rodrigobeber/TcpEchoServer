#pragma once

#include <vector>
#include "MessageHeaders.h"

class EchoMessageHandler {
public:
    static void deserialize(const std::vector<uint8_t>& requestData, EchoMessage& echoRequest);
    static void serialize(const EchoMessage& echoRequest, std::vector<uint8_t>& buffer);
    static void print(const EchoMessage &echoRequest);
    static std::string getStringMessage(const EchoMessage& echoRequest);
};