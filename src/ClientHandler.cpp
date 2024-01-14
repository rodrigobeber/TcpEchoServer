#include <iostream>
#include <unistd.h>
#include "ClientHandler.h"
#include "Protocol.h"
#include "XORCipher.h"

ClientHandler::ClientHandler(int client_socket): client_socket(client_socket) {}
ClientHandler::~ClientHandler() {}

void ClientHandler::handle() {

    // Read login
    LoginRequest loginRequest;
    if (!Protocol::readLogin(client_socket, loginRequest)) {
        std::cerr << "Error reading login." << std::endl;
        close(client_socket);
        return;
    }

    #ifdef USE_XOR_CIPHER
    cipher.calculatePartialInitialKey(loginRequest.username, loginRequest.password);
    #endif

    // Login response
    if (!Protocol::responseLogin(client_socket, loginRequest.header.requestSequence)) {
        std::cerr << "Error responsing login." << std::endl;
        close(client_socket);
        return;
    }

    while (true) {

        // Read message
        EchoMessage request;
        if (!Protocol::readEchoMessage(client_socket, request, MessageType::EchoRequest)) {
            break;
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

        if (!Protocol::sendEchoMessage(client_socket, response)) {
            std::cerr << "Error responsing echo message." << std::endl;
            break;
        }

    }

    close(client_socket);
}

