#include "UdpChatClient.h"
#include "UdpCommandBuilder.h"
#include "MessageUdp.h"
#include "InputHandler.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unordered_set>
#include <algorithm>
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
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("ERROR: Nelze vytvořit UDP socket");
        return false;
    }

    struct sockaddr_in localAddr;
    std::memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(0);

    if (bind(sockfd, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
        perror("ERROR: Bind selhal");
        close(sockfd);
        sockfd = -1;
        return false;
    }

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
    if (!bindSocket()) {
        return false;
    }
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

        if (input.empty()) continue;

        // Kontrola, zda se jedná o příkaz začínající lomítkem
        if (input[0] == '/') {
            handleCommand(input);  
        } else {
            sendMessage(input);  
        }
    }
}

void UdpChatClient::handleCommand(const std::string& input) {
    if (input == "/help") {
        printHelp();  
    } else if (input.rfind("/auth", 0) == 0) {
        handleAuthCommand(input);  
    } else if (displayName.empty()) {
        std::cout << "ERROR: Nejprve se musíte autentizovat (/auth) než odešlete zprávu." << std::endl;
    } else if (input.rfind("/join", 0) == 0) {
        handleJoinCommand(input);  
    } else if (input.rfind("/rename", 0) == 0) {
        handleRenameCommand(input); 
    } else {
        std::cout << "ERROR: Neznámý příkaz: " << input << std::endl;
    }
}

void UdpChatClient::printHelp() {
    std::cout << "Podporované příkazy:" << std::endl;
    std::cout << "  /auth {Username} {Secret} {DisplayName}  - Přihlášení uživatele" << std::endl;
    std::cout << "  /join {ChannelID}                     - Připojení do kanálu" << std::endl;
    std::cout << "  /rename {DisplayName}                  - Změna zobrazovaného jména" << std::endl;
    std::cout << "  /help                                  - Zobrazení této nápovědy" << std::endl;
}

void UdpChatClient::handleAuthCommand(const std::string& input) {
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
}




void UdpChatClient::handleJoinCommand(const std::string& input) {
    auto joinOpt = InputHandler::parseJoinCommand(input);
    if (joinOpt) {
        if (displayName.empty()) {
            std::cout << "ERROR: Nejprve se musíte autentizovat (/auth)." << std::endl;
            return;
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
        std::cout << "ERROR:Neplatný /join příkaz. Správný formát: /join {ChannelID}" << std::endl;
    }
}

void UdpChatClient::handleRenameCommand(const std::string& input) {
    std::string newDisplayName = input.substr(8);
    newDisplayName.erase(0, newDisplayName.find_first_not_of(" \t"));
    newDisplayName.erase(newDisplayName.find_last_not_of(" \t") + 1);
    if (newDisplayName.empty()) {
        std::cout << "ERROR: Neplatný /rename příkaz: Display name nesmí být prázdný." << std::endl;
        return;
    }
    displayName = newDisplayName;
    std::cout << "Display name změněn na: " << displayName << std::endl;
}

void UdpChatClient::sendMessage(const std::string& message) {
    if (displayName.empty()) {
        std::cout << "ERROR: Nejprve se musíte autentizovat (/auth) než odešlete zprávu." << std::endl;
    } else {
        UdpMessage msgMsg = buildMsgUdpMessage(displayName, message, nextMessageId++);
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
void UdpChatClient::sendByeMessage() {
    if (!displayName.empty()) {
        UdpMessage byeMsg;
        byeMsg.type = UdpMessageType::BYE;
        byeMsg.messageId = nextMessageId++;   
        byeMsg.payload = packString(displayName);
        std::vector<uint8_t> buffer = packUdpMessage(byeMsg);
        std::cout << "DEBUG: Odesílám BYE zprávu s MessageID " << byeMsg.messageId << ", payload size = " << byeMsg.payload.size() << std::endl;
        ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                                   (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            perror("ERROR: Odeslání UDP BYE zprávy selhalo");
        } else {
            std::cout << "UDP BYE zpráva odeslána." << std::endl;
        }
    }
}

void UdpChatClient::receiveServerResponseUDP() {
    uint8_t recvBuffer[1024];
    struct sockaddr_in fromAddr;
    socklen_t addrLen = sizeof(fromAddr);
    ssize_t bytesReceived = recvfrom(sockfd, recvBuffer, sizeof(recvBuffer), 0,
                                     (struct sockaddr*)&fromAddr, &addrLen);
    if (bytesReceived <= 0) return;

    std::vector<uint8_t> data(recvBuffer, recvBuffer + bytesReceived);
    UdpMessage receivedMsg;
    if (unpackUdpMessage(data, receivedMsg)) {
        switch (receivedMsg.type) {
            case UdpMessageType::REPLY:
                processReplyMessage(receivedMsg);
                break;

            case UdpMessageType::CONFIRM:
                processConfirmMessage(receivedMsg);
                break;

            case UdpMessageType::MSG:
                processMsgMessage(receivedMsg);
                break;

            case UdpMessageType::ERR:
                processErrMessage(receivedMsg);
                break;

            default:
                std::cerr << "ERROR: Neznámý typ zprávy: " << static_cast<int>(receivedMsg.type) << std::endl;
                break;
        }
    }
}


void UdpChatClient::processErrMessage(const UdpMessage& errMsg) {
    // Check if the payload has at least 1 byte
    if (errMsg.payload.size() < 1) {
        std::cerr << "ERR message is empty." << std::endl;
        return;
    }
    
    // Convert the payload to a string (error message)
    std::string errorContent(errMsg.payload.begin(), errMsg.payload.end());
    // Remove any null terminators at the end
    size_t pos = errorContent.find('\0');
    if (pos != std::string::npos) {
        errorContent = errorContent.substr(0, pos);
    }

    // Display the error message
    std::cerr << "Error from server (ERR): " << errorContent << std::endl;

    // Handle the error (you may want to exit or handle gracefully)
    exit(EXIT_FAILURE);
}
void UdpChatClient::processReplyMessage(const UdpMessage& replyMsg) {
    if (replyMsg.payload.size() < 3) {
        std::cerr << "ERROR:REPLY zpráva je příliš krátká." << std::endl;
        return;
    }

    uint8_t result = replyMsg.payload[0];  // Výsledek akce: 1 pro OK, 0 pro NOK
    uint16_t refMessageId;
    std::memcpy(&refMessageId, &replyMsg.payload[1], sizeof(uint16_t));
    refMessageId = ntohs(refMessageId);  // Získáme MessageID

    // Zbytek payloadu je textová zpráva, ukončená nulou
    std::string content;
    if (replyMsg.payload.size() > 3) {
        content = std::string(replyMsg.payload.begin() + 3, replyMsg.payload.end());
        size_t pos = content.find('\0');
        if (pos != std::string::npos) {
            content = content.substr(0, pos);
        }
    }

    // Výsledek (OK nebo NOK)
    std::string status = content.substr(0, 2); // "OK" nebo "NOK"
    std::string msg = content.substr(3);       // Text zprávy (což je vše za "OK"/"NOK")

    if (status == "OK") {
        std::cout << "Action Success: " << msg << std::endl;

        // Po úspěšném REPLY se automaticky připojíme k výchozímu kanálu
        std::cout << "Autentizace úspěšná. Připojuji se k výchozímu kanálu..." << std::endl;
        handleJoinCommand("/join default");  
    } else if (status == "NOK") {
        std::cout << "Action Failure: " << msg << std::endl;
    } else {
        std::cout << "ERROR: Unknown REPLY status: " << content << std::endl;
    }

    // Odpovědní zpráva pro REPLY s MessageID z předchozího příkazu
    UdpMessage generatedReplyMsg = buildReplyUdpMessage("Odpověd' na vaši zprávu", nextMessageId++, refMessageId, 1);
    std::vector<uint8_t> buffer = packUdpMessage(generatedReplyMsg);
    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes < 0) {
        perror("ERROR: Odeslání UDP REPLY zprávy selhalo");
    } else {
        std::cout << "UDP REPLY zpráva odeslána." << std::endl;
    }
}

void UdpChatClient::processMsgMessage(const UdpMessage& msgMsg) {
      if (displayName.empty()) {
        std::cout << "ERROR: Nejprve se musíte autentifikovat (/auth) než odešlete zprávu." << std::endl;
        return; 
    }
    if (msgMsg.payload.size() > 0) {
        std::string message(msgMsg.payload.begin(), msgMsg.payload.end());
        std::cout << "Server: " << message << std::endl;

        // Potvrzení připojení k výchozímu kanálu
        UdpMessage confirmMsg;
        confirmMsg.type = UdpMessageType::CONFIRM;
        confirmMsg.messageId = nextMessageId++;   
        confirmMsg.payload.clear();  // Můžeme vyčistit payload, pokud není potřeba
        std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);
        ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                                   (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            perror("ERROR: Odeslání UDP CONFIRM zprávy selhalo");
        } else {
            std::cout << "UDP CONFIRM zpráva odeslána." << std::endl;
        }
    }
}

void UdpChatClient::processConfirmMessage(const UdpMessage& confirmMsg) {
    std::cout << "Obdržena CONFIRM zpráva od serveru." << std::endl;

    // Vytvoření odpovědi pro CONFIRM a odeslání zpět serveru
    UdpMessage generatedConfirmMsg;
    generatedConfirmMsg.type = UdpMessageType::CONFIRM;
    generatedConfirmMsg.messageId = nextMessageId++;
    generatedConfirmMsg.payload.clear();  

    std::vector<uint8_t> buffer = packUdpMessage(generatedConfirmMsg);
    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes < 0) {
        perror("ERROR: Odeslání UDP CONFIRM zprávy selhalo");
    } else {
        std::cout << "UDP CONFIRM zpráva odeslána." << std::endl;
    }

 
    if (displayName.empty()) {
        std::cout << "ERROR: Nejprve se musíte autentizovat (/auth)." << std::endl;
    } else {
        std::cout << "Autentizace úspěšná. Připojuji se k výchozímu kanálu..." << std::endl;
        handleJoinCommand("/join default");
    }
}

