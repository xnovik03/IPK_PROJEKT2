#include "TcpChatClient.h"
#include "InputHandler.h"
#include <iostream>
#include <string>
#include <cstring>       // memset
#include <sys/types.h>
#include <sys/socket.h>  // socket, connect, AF_INET, SOCK_STREAM
#include <netdb.h>       // getaddrinfo, addrinfo, freeaddrinfo
#include <unistd.h>      // close()
#include <cerrno>        // perror

TcpChatClient::TcpChatClient(const std::string& host, int port)
    : server(host), port(port), sockfd(-1) {}

TcpChatClient::~TcpChatClient() {
    if (sockfd != -1) close(sockfd);
}

bool TcpChatClient::connectToServer() {
    struct addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(server.c_str(), std::to_string(port).c_str(), &hints, &res);
    if (status != 0) {
        std::cerr << "ERROR: getaddrinfo failed: " << gai_strerror(status) << "\n";
        return false;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        std::perror("ERROR: socket creation failed");
        freeaddrinfo(res);
        return false;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        std::perror("ERROR: connect failed");
        close(sockfd);
        sockfd = -1;
        freeaddrinfo(res);
        return false;
    }

    freeaddrinfo(res);
    std::cout << "TCP client connected successfully.\n";
    return true;
}

void TcpChatClient::run() {
    std::string line;
    char buffer[1024];

    while (std::getline(std::cin, line)) {
        if (line.rfind("/auth", 0) == 0) {
            auto cmd = InputHandler::parseAuthCommand(line);
            if (cmd) {
                std::string authMessage = "AUTH " + cmd->username + " AS " + cmd->displayName + " USING " + cmd->secret + "\r\n";
                std::cout << "Sending: " << authMessage << std::endl;
                if (send(sockfd, authMessage.c_str(), authMessage.size(), 0) == -1) {
                    std::perror("ERROR: send failed");
                    break;
                }

                // Čekáme na odpověď od serveru
                ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
                if (received <= 0) {
                    std::cerr << "Connection closed or error occurred.\n";
                    break;
                }
                buffer[received] = '\0';
                std::cout << "Server: " << buffer;

            } else {
                std::cerr << "Invalid /auth command format.\n";
                continue;
            }
        } else if (line.rfind("/join", 0) == 0) {
            auto cmd = InputHandler::parseJoinCommand(line);
            if (cmd) {
                std::string joinMessage = "JOIN " + *cmd + "\r\n";  
                std::cout << "Sending: " << joinMessage << std::endl;
                if (send(sockfd, joinMessage.c_str(), joinMessage.size(), 0) == -1) {
                    std::perror("ERROR: send failed");
                    break;
                }

                // Čekáme na odpověď od serveru
                ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
                if (received <= 0) {
                    std::cerr << "Connection closed or error occurred.\n";
                    break;
                }
                buffer[received] = '\0';
                std::cout << "Server: " << buffer;
            } else {
                std::cerr << "Invalid /join command format.\n";
                continue;
            }
        } else {
            std::cerr << "Unknown command or message format.\n";
            continue;
        }
    }
}




