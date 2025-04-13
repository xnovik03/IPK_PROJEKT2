#include "UdpChatClient.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

UdpChatClient::UdpChatClient(const std::string& server, int port)
    : serverAddress(server), serverPort(port), sockfd(-1) {

}

UdpChatClient::~UdpChatClient() {
    if (sockfd != -1) {
        close(sockfd);
    }
}

bool UdpChatClient::bindSocket() {
    // Vytvoření UDP socketu
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("ERROR: Nelze vytvořit UDP socket");
        return false;
    }

    // Nastavení lokální adresy pro bind
    struct sockaddr_in localAddr;
    std::memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(0); // 0 = dynamicky přidělený port

    if (bind(sockfd, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
        perror("ERROR: Bind selhal");
        close(sockfd);
        sockfd = -1;
        return false;
    }

    // Volitelně vypíšeme lokální port
    socklen_t len = sizeof(localAddr);
    if (getsockname(sockfd, (struct sockaddr*)&localAddr, &len) == 0) {
        std::cout << "UDP client používá lokální port: " << ntohs(localAddr.sin_port) << std::endl;
    }
    return true;
}

bool UdpChatClient::resolveServerAddr() {
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "ERROR: Neplatná serverová IP adresa: " << serverAddress << "\n";
        return false;
    }
    return true;
}

bool UdpChatClient::connectToServer() {
    // Vytvoříme a připojíme socket (bind)
    if (!bindSocket()) {
        return false;
    }
    // Nastavíme serverovou adresu
    if (!resolveServerAddr()) {
        return false;
    }
    std::cout << "UDP client připraven odesílat zprávy na " << serverAddress 
              << ":" << serverPort << std::endl;
    return true;
}

void UdpChatClient::run() {
    std::cout << "UDP klient spuštěn. Zadejte zprávu pro odeslání na server :\n";

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;
       
        if (line.find("\r\n") == std::string::npos)
            line += "\r\n";
        
        ssize_t sentBytes = sendto(sockfd, line.c_str(), line.size(), 0,
                                   (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            perror("ERROR: Odeslání zprávy selhalo");
        } else {
            std::cout << "Odeslána zpráva: " << line;
        }
    }

    std::cout << "Ukončuji UDP klienta...\n";
}

