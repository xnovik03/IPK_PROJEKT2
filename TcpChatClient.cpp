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
#include <sys/select.h>  // pro select()
#include "MessageTcp.h"
#include <arpa/inet.h>
#include <netinet/in.h>

TcpChatClient::TcpChatClient(const std::string& host, int port)
    : server(host), port(port), sockfd(-1) {}

TcpChatClient::~TcpChatClient() {
    if (sockfd != -1) close(sockfd);
}

// Funkce connectToServer vychází z ukázkového kódu “Simple Stream Client”(Beej’s Guide to Network Programming, sekce 6.2).
// https://beej.us/guide/bgnet/html/split/client-server-background.html#a-simple-stream-client
// Původní příklad podporoval IPv4 i IPv6 a demonstroval recv/print.
// Tuto verzi  upravila:
//  - hints.ai_family = AF_INET → pouze IPv4  
//  - odstraněno cyklické recv/print
//  - vypisujeme připojenou IPv4 adresu pomocí inet_ntop()  
//  - uvolnění struct addrinfo voláním freeaddrinfo()  
// Tímto se funkce jenom s TCP připojením a IPv4‑only výpis.

bool TcpChatClient::connectToServer() {
    struct addrinfo hints{}, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;       // jen IPv4
    hints.ai_socktype = SOCK_STREAM;   // TCP

    if ((rv = getaddrinfo(server.c_str(), std::to_string(port).c_str(), &hints, &servinfo)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << "\n";
        return false;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) { perror("socket"); continue; }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd); perror("connect"); continue;
        }
        break;
    }

    if (p == nullptr) {
        std::cerr << "client: failed to connect\n";
        freeaddrinfo(servinfo);
        return false;
    }

    // Vypiš IPv4 adresu serveru
    auto *ipv4 = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
    char ip4[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ipv4->sin_addr, ip4, sizeof(ip4));
    std::cout << "client: connecting to " << ip4 << std::endl;

    freeaddrinfo(servinfo);
    std::cout << "TCP client connected successfully.\n";
    return true;
}


Message TcpChatClient::parseMessage(const std::string& buffer) {
    // Assuming the first word indicates the message type
    if (buffer.find("AUTH") == 0) {
        return Message::fromBuffer(buffer);  // Handle AUTH
    } else if (buffer.find("JOIN") == 0) {
        return Message::fromBuffer(buffer);  // Handle JOIN
    } else if (buffer.find("BYE") == 0) {
        return Message::fromBuffer(buffer);  // Handle BYE
    } else if (buffer.find("ERR") == 0) {
        return Message::fromBuffer(buffer);  // Handle ERR
    } else if (buffer.find("REPLY") == 0) {
        return Message::fromBuffer(buffer);  // Handle REPLY
    } else if (buffer.find("MSG") == 0) {
        return Message::fromBuffer(buffer);  // Handle MSG
    }
    return Message(Message::Type::MSG, "Unknown message type");  // Use Message::Type
}


// this function implementade with AI
void TcpChatClient::receiveServerResponse() {
    char buffer[1024];
    while (true) {
        ssize_t received = read(sockfd, buffer, sizeof(buffer) - 1); 
        if (received <= 0) {
            std::cerr << "Connection closed or error occurred.\n";
            break;
        }
        buffer[received] = '\0';
        std::cout << "Server response: " << buffer << std::endl;

        // Parse the response into a Message object (this depends on your Message class structure)
        Message reply = parseMessage(buffer);

        // Process the reply
        process_reply(reply);

        // Odpovědi typu ERR nebo BYE
        if (std::string(buffer).find("ERR") == 0) {
            std::cerr << "ERROR FROM SERVER: " << buffer + 4 << std::endl;
            close(sockfd);  // Ukončení spojení při chybě
            break;
        } else if (std::string(buffer).find("BYE") == 0) {
            std::cout << "Server is closing the connection...\n";
            close(sockfd);  // Ukončení spojení při obdržení BYE
            break;
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

    sendByeMessage();  // Zavolání sendByeMessage až po ukončení komunikace
    receiverThread.join();
}

void TcpChatClient::printHelp() {
    std::cout << "/auth {Username} {Secret} {DisplayName} - Authenticate user\n";
    std::cout << "/join {ChannelID} - Join a channel\n";
    std::cout << "/rename {DisplayName} - Change your display name\n";
    std::cout << "/help - Show this help message\n";
}

void TcpChatClient::sendByeMessage() {
    if (!displayName.empty()) {
        Message byeMessage = Message::createByeMessage(displayName);
        std::cout << "Sending BYE message: " << byeMessage.getContent() << "\r\n" << std::endl;
        byeMessage.sendMessage(sockfd);  
    }
}

void TcpChatClient::process_reply(const Message& reply)
{
    if (reply.getType() == Message::REPLY) {
        std::string content = reply.getContent(); 
        auto pos = content.find(' ');
        if (pos == std::string::npos) {
            std::cout << "Malformed REPLY: " << content << std::endl;
            return;
        }
        std::string status = content.substr(0, pos); // "OK" nebo "NOK"
        std::string msg = content.substr(pos + 1);     // text zprávy

        if (status == "OK") {
            std::cout << "Action Success: " << msg << std::endl;
        } else if (status == "NOK") {
            std::cout << "Action Failure: " << msg << std::endl;
        } else {
            std::cout << "Unknown REPLY status: " << content << std::endl;
        }
    }
}
