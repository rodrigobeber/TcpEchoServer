#pragma once

#include "MessageHeaders.h"
#include "Connection.h"

class Protocol {
private:
    static OperationStatus readHeader(int socket, Header& header); // Used only for Echo requests
public:
    static bool readLogin(int socket, LoginRequest &request);
    static bool responseLogin(int socket, unsigned sequence);

    // These two can be used by a server or a client
    static bool sendEchoMessage(int socket, EchoMessage &request);
    static bool readEchoMessage(int socket, EchoMessage &request, MessageType type); // Type = expected type for checking
};