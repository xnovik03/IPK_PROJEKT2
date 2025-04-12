
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
#include <thread>
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

void TcpChatClient::receiveServerResponse() {
    char buffer[1024];
    while (true) {
        ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            std::cerr << "Connection closed or error occurred.\n";
            break;
        }
        buffer[received] = '\0';
        std::cout << "Server response: " << buffer << std::endl;

        // Zpracování odpovědi od serveru
        if (std::string(buffer).find("REPLY OK") != std::string::npos) {
            std::cout << "Command successfully processed." << std::endl;
        } else if (std::string(buffer).find("REPLY NOK") != std::string::npos) {
            std::cerr << "Command failed." << std::endl;
        } else {
            std::cerr << "Unknown response from server: " << buffer << std::endl;
        }
    }
}

void TcpChatClient::run() {
    std::string line;

    // Spustíme vlákno pro příjem zpráv
    std::thread receiverThread(&TcpChatClient::receiveServerResponse, this);

    // Smyčka pro čtení příkazů a zpráv
    while (std::getline(std::cin, line)) {
        std::string messageToSend;

        // Zpracování příkazů
        if (line.rfind("/help", 0) == 0) {
            printHelp();
            continue;
        }
        if (line.rfind("/auth", 0) == 0) {
            auto cmd = InputHandler::parseAuthCommand(line);
            if (cmd) {
                messageToSend = "AUTH " + cmd->username + " AS " + cmd->displayName + " USING " + cmd->secret + "\r\n";
                // Po úspěšném přihlášení nastavíme displayName
                displayName = cmd->displayName;
            } else {
                std::cerr << "Invalid /auth command format.\n";
                continue;
            }
        } else if (line.rfind("/join", 0) == 0) {
            auto cmd = InputHandler::parseJoinCommand(line);
            if (cmd) {
                messageToSend = "JOIN " + cmd.value() + " AS " + displayName + "\r\n";
            } else {
                std::cerr << "Invalid /join command format.\n";
                continue;
            }
        } else if (line.rfind("/rename", 0) == 0) {
            std::string newDisplayName = line.substr(8);  // Oříznutí "/rename "
            if (newDisplayName.empty()) {
                std::cerr << "Invalid /rename command format: Display name cannot be empty.\n";
                continue;
            }
            displayName = newDisplayName;  // Změníme displayName
            std::cout << "Display name changed to: " << displayName << std::endl;
            continue;
        } else {
            // Pokud to není příkaz, považujeme to za zprávu, kterou pošleme na server
            messageToSend = "MSG FROM " + displayName + " IS " + line + "\r\n";
        }

        // Odeslání zprávy nebo příkazu na server
        std::cout << "Sending: " << messageToSend << std::endl;
        if (send(sockfd, messageToSend.c_str(), messageToSend.size(), 0) == -1) {
            std::perror("ERROR: send failed");
            break;
        }
    }

    receiverThread.join();
}


void TcpChatClient::printHelp() {
    std::cout << "/auth {Username} {Secret} {DisplayName} - Authenticate user\n";
    std::cout << "/join {ChannelID} - Join a channel\n";
    std::cout << "/rename {DisplayName} - Change your display name\n";
    std::cout << "/help - Show this help message\n";
}

