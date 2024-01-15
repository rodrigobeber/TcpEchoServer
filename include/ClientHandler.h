#pragma once

#include <atomic>
#include "XORCipher.h"

class ClientHandler {
private:
    int clientSocket;
    XORCipher cipher;
    bool handleLogin();
    bool handleEchoRequest();
public:
    ClientHandler(int clientSocket);
    ~ClientHandler();
    bool isLoggedIn;
    bool isProcessing;
    bool processRequest();
};