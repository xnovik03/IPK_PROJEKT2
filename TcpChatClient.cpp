#include "TcpChatClient.h"
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

TcpChatClient::TcpChatClient(const std::string& server, int port)
    : server(server), port(port), sockfd(-1) {}

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
        perror("ERROR: socket creation failed");
        freeaddrinfo(res);
        return false;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("ERROR: connect failed");
        close(sockfd);
        freeaddrinfo(res);
        return false;
    }

    std::cout << "Connected to " << server << ":" << port << " over TCP.\n";
    freeaddrinfo(res);
    return true;
}
