#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include "EchoMessageHandler.h"
#include "MessageHeaders.h"
#include "Connection.h"
#include "Protocol.h"
#include "XORCipher.h"

// Function to send data
bool sendData(int socket, const void* data, size_t size) {
    ssize_t bytes_sent = send(socket, data, size, 0);
    return bytes_sent == size;
}

int main() {
    // Server address and port
    const char* server_ip = "127.0.0.1";
    int server_port = 8080;
    unsigned requestSequence = 0;

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket. Exiting." << std::endl;
        return 1;
    }

    std::cout << "Successfully created socket." << std::endl;

    // Fill in server address structure
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to server. Exiting." << std::endl;
        return 1;
    }

    std::cout << "Connected to server." << std::endl;

    // Prepare LoginRequest
    LoginRequest loginRequest;
    memset(&loginRequest, 0, sizeof(loginRequest));
    loginRequest.header.requestType = static_cast<unsigned>(MessageType::LoginRequest);
    loginRequest.header.requestSize = sizeof(LoginRequest);
    loginRequest.header.requestSequence = requestSequence++;
    strcpy(loginRequest.username, "testuser");
    strcpy(loginRequest.password, "testpass");

    // Prepare buffer
    std::vector<u_int8_t> sendBuffer(sizeof(LoginRequest));
    std::memcpy(sendBuffer.data(), &loginRequest, sizeof(LoginRequest));

    // Send login request
    if (Connection::sendData(sock, sendBuffer) != OperationStatus::Success) {
        std::cerr << "Failed to send login request." << std::endl;
        close(sock);
        return 1;
    }

    // Receive and process LoginResponse
    std::vector<uint8_t> readBuffer(sizeof(LoginResponse));
    if (Connection::receiveData(sock, readBuffer) != OperationStatus::Success) {
        std::cerr << "Failed reading login response." << std::endl;
        close(sock);
        return 1;
    }
    LoginResponse loginResponse;
    std::memcpy(&loginResponse, readBuffer.data(), sizeof(LoginResponse));

    // Check response status
    if (loginResponse.statusCode != 1) {
        std::cerr << "Login failed. Exiting." << std::endl;
        close(sock);
        return 1;
    }
    std::cout << "Login successful." << std::endl;

    #ifdef USE_XOR_CIPHER
    XORCipher cipher;
    cipher.calculatePartialInitialKey(loginRequest.username, loginRequest.password);
    std::cout << "Encryption enabled." << std::endl;
    #else
    std::cout << "Encryption disabled." << std::endl;
    #endif

    std::cout << std::endl;

    // Function to send an echo request and receive a response
    auto sendEchoRequest = [&](const std::string& message) {

        // Prepare request
        EchoMessage echoRequest;
        echoRequest.header.requestType = static_cast<unsigned>(MessageType::EchoRequest);
        echoRequest.header.requestSequence = requestSequence++;
        echoRequest.messageSize = message.size(); // Obs.: Encryption won't change the size
        echoRequest.header.requestSize = sizeof(echoRequest.header) + sizeof(echoRequest.messageSize) + echoRequest.messageSize;
        echoRequest.message = std::vector<uint8_t>(message.begin(), message.end()); // Set here for printing in the next line
        EchoMessageHandler::print(echoRequest);

        #ifdef USE_XOR_CIPHER
        echoRequest.message = cipher.encrypt(std::vector<uint8_t>(message.begin(), message.end()), echoRequest.header.requestSequence);
        #endif

        echoRequest.messageSize = echoRequest.message.size();
        echoRequest.header.requestSize = sizeof(echoRequest.header) + sizeof(echoRequest.messageSize) + echoRequest.messageSize;

        // Send echo request
        if (Protocol::sendEchoMessage(sock, echoRequest)) {
            EchoMessage echoResponse;
            if (Protocol::readEchoMessage(sock, echoResponse, MessageType::EchoResponse)) {
                std::cout << "Echo Response: " << std::endl
                        << '\t' << "Sequence -> " << echoResponse.header.requestSequence << std::endl
                        << '\t' << "Message -> " <<
                            EchoMessageHandler::getStringMessage(echoResponse)
                            << std::endl << std::endl;
            } else {
                std::cerr << "Error reading echo response." << std::endl;
            }
        } else {
            std::cerr << "Error sending message." << std::endl;
        }

        return true;
    };

    // Send two echo requests
    if (!sendEchoRequest("Hello, server!")) {
        close(sock);
        return 1;
    }
    if (!sendEchoRequest("Second message")) {
        close(sock);
        return 1;
    }

    // Close the socket
    close(sock);
    std::cout << "Disconnected from server." << std::endl;
    return 0;
}