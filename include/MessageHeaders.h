#pragma once

#include <vector>

struct Header {
    unsigned requestSize;
    unsigned requestType;
    unsigned requestSequence;
};

// Login Request (type=0)
struct LoginRequest {
    Header header;
    char username[32];
    char password[32];
};

// Login Response (type=1)
struct LoginResponse {
    Header header;
    unsigned statusCode;
};

// Echo Request (type=2)
struct EchoMessage {
    Header header;
    unsigned messageSize;
    std::vector<uint8_t> message;
};

enum class MessageType : unsigned {
    LoginRequest = 0,  // 0
    LoginResponse = 1, // 1
    EchoRequest = 2,   // 2
    EchoResponse = 3  // 3
};