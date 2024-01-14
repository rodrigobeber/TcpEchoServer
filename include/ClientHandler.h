#pragma once

#include "XORCipher.h"

class ClientHandler {
private:
    int client_socket;
    XORCipher cipher;
public:
    ClientHandler(int client_socket);
    ~ClientHandler();
    void handle();
};