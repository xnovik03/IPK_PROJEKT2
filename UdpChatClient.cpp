#include "UdpChatClient.h"
#include "UdpCommandBuilder.h"
#include "MessageUdp.h"
#include "InputHandler.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

UdpChatClient::UdpChatClient(const std::string& server, int port)
    : serverAddress(server), serverPort(port), sockfd(-1),
      nextMessageId(0), displayName("") {
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
    std::cout << "UDP klient spuštěn. Zadejte příkaz: " << std::endl;
    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "/quit") break;

        // Kontrola, zda se jedná o příkaz začínající lomítkem
        if (!input.empty() && input[0] == '/') {
         
            if (input.rfind("/auth", 0) == 0) {
                auto authOpt = InputHandler::parseAuthCommand(input);
                if (authOpt) {
                    
                    this->displayName = authOpt->displayName;
                    UdpMessage authMsg = buildAuthUdpMessage(*authOpt, nextMessageId++);
                    std::vector<uint8_t> buffer = packUdpMessage(authMsg);
                    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
                    if (sentBytes < 0) {
                        perror("ERROR: Odeslání UDP AUTH zprávy selhalo");
                    } else {
                        std::cout << "UDP AUTH zpráva odeslána." << std::endl;
                    }
                } else {
                    std::cout << "Neplatný /auth příkaz. Správný formát: /auth {Username} {Secret} {DisplayName}" << std::endl;
                }
                continue;
            }

            // Zpracování /join příkazu
            if (input.rfind("/join", 0) == 0) {
                // Vstupní formát očekává: /join channelID
                auto joinOpt = InputHandler::parseJoinCommand(input);
                if (joinOpt) {
                    if (displayName.empty()) {
                        std::cout << "Nejprve se musíte autentizovat (/auth)." << std::endl;
                        continue;
                    }
                    UdpMessage joinMsg = buildJoinUdpMessage(joinOpt.value(), displayName, nextMessageId++);
                    std::vector<uint8_t> buffer = packUdpMessage(joinMsg);
                    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
                    if (sentBytes < 0) {
                        perror("ERROR: Odeslání UDP JOIN zprávy selhalo");
                    } else {
                        std::cout << "UDP JOIN zpráva odeslána." << std::endl;
                    }
                } else {
                    std::cout << "Neplatný /join příkaz. Správný formát: /join {ChannelID}" << std::endl;
                }
                continue;
            }
            // Přidání  příkazu /rename
            if (input.rfind("/rename", 0) == 0) {
              
                std::string newDisplayName = input.substr(8);  
                // Odstraníme mezery na začátku a konci
                newDisplayName.erase(0, newDisplayName.find_first_not_of(" \t"));
                newDisplayName.erase(newDisplayName.find_last_not_of(" \t") + 1);
                if (newDisplayName.empty()) {
                    std::cout << "Neplatný /rename příkaz: Display name nesmí být prázdný." << std::endl;
                    continue;
                }
                // Aktualizace lokální proměnné displayName
                displayName = newDisplayName;
                std::cout << "Display name změněn na: " << displayName << std::endl;
                continue;
            }
            if (input.rfind("/help", 0) == 0) {
                printHelp();
                continue;
        }
}
        else {
           
            if (displayName.empty()) {
                std::cout << "Nejprve se musíte autentizovat (/auth) než odešlete zprávu." << std::endl;
            } else {
                UdpMessage msgMsg = buildMsgUdpMessage(displayName, input, nextMessageId++);
                std::vector<uint8_t> buffer = packUdpMessage(msgMsg);
                ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                                           (struct sockaddr*)&serverAddr, sizeof(serverAddr));
                if (sentBytes < 0) {
                    perror("ERROR: Odeslání UDP MSG zprávy selhalo");
                } else {
                    std::cout << "UDP MSG zpráva odeslána." << std::endl;
                }
            }
        }
      
    }
}
// Definice funkce pro tisk nápovědy
void UdpChatClient::printHelp() {
    std::cout << "Podporované příkazy:" << std::endl;
    std::cout << "  /auth {Username} {Secret} {DisplayName}  - Přihlášení uživatele" << std::endl;
    std::cout << "  /join {ChannelID}                     - Připojení do kanálu" << std::endl;
    std::cout << "  /rename {DisplayName}                  - Změna zobrazovaného jména" << std::endl;
    std::cout << "  /help                                  - Zobrazení této nápovědy" << std::endl;
    std::cout << "  /quit                                  - Ukončení klienta" << std::endl;
}
// Definice  funkce pro zpracování REPLY zprávy
void processReplyMessage(const UdpMessage& replyMsg) {
    if (replyMsg.payload.size() < 3) {
        std::cerr << "REPLY zpráva je příliš krátká." << std::endl;
        return;
    }
    
    uint8_t result = replyMsg.payload[0];
    
    uint16_t refMessageId;
    std::memcpy(&refMessageId, &replyMsg.payload[1], sizeof(uint16_t));
    refMessageId = ntohs(refMessageId);
    
    // Zbytek payloadu je textová zpráva, ukončená nulou.
    std::string content;
    if (replyMsg.payload.size() > 3) {
        content = std::string(replyMsg.payload.begin() + 3, replyMsg.payload.end());
        size_t pos = content.find('\0');
        if (pos != std::string::npos) {
            content = content.substr(0, pos);
        }
    }
    
    if (result == 1) {
        std::cout << "Action Success: " << content << std::endl;
    } else {
        std::cout << "Action Failure: " << content << std::endl;
    }
}

// Definice  funkce pro zpracování ERR zprávy
void processErrMessage(const UdpMessage& errMsg) {
    // Ověříme, že payload má minimálně 1 bajt 
    if (errMsg.payload.size() < 1) {
        std::cerr << "ERR zpráva je prázdná." << std::endl;
        return;
    }
    
  
    std::string errorContent = std::string(errMsg.payload.begin(), errMsg.payload.end());
    // Odstraníme ukončovací nulový bajt, pokud existuje.
    size_t pos = errorContent.find('\0');
    if (pos != std::string::npos) {
        errorContent = errorContent.substr(0, pos);
    }
    
    std::cerr << "Chyba od serveru (ERR): " << errorContent << std::endl;
    
    exit(EXIT_FAILURE);
}
