#include <iostream>
#include <unistd.h>
#include "ClientHandler.h"
#include "MessageHeaders.h"
#include "EchoMessageHandler.h"
#include "Protocol.h"

ClientHandler::ClientHandler(int clientSocket) : clientSocket(clientSocket), isLoggedIn(false), isProcessing(false) {}
ClientHandler::~ClientHandler() {}

// Single request process (non-blocking)
bool ClientHandler::processRequest() {
    // If the client is not logged it, attempt login
    if (!isLoggedIn) {
        isLoggedIn = handleLogin();
        return isLoggedIn;
    }

    // If the client is already logged in, handle an echo request
    return handleEchoRequest();
}

// Handle login
bool ClientHandler::handleLogin() {
    // Read login
    LoginRequest loginRequest;
    if (!Protocol::readLogin(clientSocket, loginRequest)) {
        std::cerr << "Error reading login" << std::endl;
        return false;
    }

    #ifdef USE_XOR_CIPHER
    cipher.calculatePartialInitialKey(loginRequest.username, loginRequest.password);
    #endif

    // Login response
    if (!Protocol::responseLogin(clientSocket, loginRequest.header.requestSequence)) {
        std::cerr << "Error responsing login" << std::endl;
        return false;
    }

    isLoggedIn = true;
    std::cout << "Client " << clientSocket << " logged in" << std::endl;

    return true;
}

// Handle echo request
bool ClientHandler::handleEchoRequest() {
    // Read message
    EchoMessage request;
    if (!Protocol::readEchoMessage(clientSocket, request, MessageType::EchoRequest)) {
        return false;
    }

    // Start preparing response
    EchoMessage response;
    response.header.requestSequence = request.header.requestSequence;
    response.header.requestType = static_cast<unsigned>(MessageType::EchoResponse);

    // If enabled encryption, decrypt
    #ifdef USE_XOR_CIPHER
    response.message = cipher.decrypt(request.message, request.header.requestSequence);
    #else
    response.message = request.message;
    #endif

    // Finish preparing response
    response.messageSize = response.message.size();
    response.header.requestSize = sizeof(response.header) + sizeof(response.message) + response.messageSize;

    if (!Protocol::sendEchoMessage(clientSocket, response)) {
        std::cerr << "Error responsing echo message." << std::endl;
        return false;
    }

    return true;
}