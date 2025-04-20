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

        // Zpracování přijatých zpráv
        receiveServerResponseUDP();  // Tato funkce zpracuje příchozí zprávy
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
        
        // Čekání na odpověď REPLY od serveru
        receiveServerResponseUDP();  // Zde čekáme na odpověď od serveru
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

    if (displayName.empty()) {
        std::cout << "ERROR: Nejprve se musíte autentizovat (/auth)." << std::endl;
        return;
    }

    displayName = newDisplayName;
    std::cout << "Display name změněn na: " << displayName << std::endl;
    std::cout << "[DEBUG] Nastaven nový displayName: " << displayName << std::endl;
}



void UdpChatClient::sendMessage(const std::string& message) {
    if (displayName.empty()) {
        std::cout << "ERROR: Nejprve se musíte autentizovat (/auth) než odešlete zprávu." << std::endl;
    } else {
        std::cout << "[DEBUG] Odesílám zprávu jako: " << displayName << std::endl;

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
    if (bytesReceived <= 0) {
        std::cerr << "Chyba při přijímání zprávy!" << std::endl;
        return;
    }

    std::vector<uint8_t> data(recvBuffer, recvBuffer + bytesReceived);
    UdpMessage receivedMsg;
    
    if (unpackUdpMessage(data, receivedMsg)) {
        switch (receivedMsg.type) {
            case UdpMessageType::REPLY:
                processReplyMessage(receivedMsg);  // Zpracování REPLY zprávy
                break;

            case UdpMessageType::CONFIRM:
                processConfirmMessage(receivedMsg);  // Zpracování CONFIRM zprávy
                break;

            case UdpMessageType::MSG:
                // Zpracování zprávy MSG od serveru
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
    // Check if payload contains at least 1 byte
    if (errMsg.payload.size() < 1) {
        std::cerr << "ERR message is empty." << std::endl;
        return;
    }

    // Convert payload to string (error message)
    std::string errorContent(errMsg.payload.begin(), errMsg.payload.end());

    // Find the first null byte (indicates the end of the server name)
    size_t firstNullPos = errorContent.find('\0');
    if (firstNullPos != std::string::npos) {
        // Print content up to the first null byte (display name or start of error message)
        std::cout << "ERROR FROM " << errorContent.substr(0, firstNullPos) << ": ";
        
        // Find the second null byte (indicates the end of the error message)
        size_t secondNullPos = errorContent.find('\0', firstNullPos + 1);
        if (secondNullPos != std::string::npos) {
            // Print content between the null bytes (error message)
            std::cout << errorContent.substr(firstNullPos + 1, secondNullPos - firstNullPos - 1) << std::endl;
        } else {
            std::cerr << "Error: Could not find the second null byte!" << std::endl;
        }
    } else {
        std::cerr << "Error: Could not find the first null byte!" << std::endl;
        return;
    }

    // Debugging: Print payload for better insight
    std::cerr << "Payload size: " << errMsg.payload.size() << std::endl;
    std::cerr << "Payload (raw data): ";
    for (auto byte : errMsg.payload) {
        std::cerr << std::hex << static_cast<int>(byte) << " "; // Hexadecimal output
    }
    std::cerr << std::dec << std::endl; // Switch back to decimal output

    // Send dynamic CONFIRM message with the actual message ID
    UdpMessage confirmMsg;
    confirmMsg.type = UdpMessageType::CONFIRM;
    confirmMsg.messageId = errMsg.messageId;  // Use the received message ID for confirmation
    confirmMsg.payload.clear();  // You can choose to send additional payload here if necessary

    std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);
    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes < 0) {
        perror("ERROR: Sending UDP CONFIRM message failed");
    } else {
        std::cout << "UDP CONFIRM message sent." << std::endl;
    }

    // You can choose whether or not to exit the application here
    std::exit(EXIT_FAILURE);
}




void UdpChatClient::processReplyMessage(const UdpMessage& replyMsg) {
    if (replyMsg.payload.size() < 3) {
        std::cerr << "ERROR: REPLY zpráva je příliš krátká." << std::endl;
        return;
    }

    uint8_t result = replyMsg.payload[0];
    uint16_t refMessageId = ntohs(*(uint16_t*)&replyMsg.payload[1]);
    std::string content(replyMsg.payload.begin() + 3, replyMsg.payload.end());

    if (result == 1) {
        std::cout << "Action Success: " << content << std::endl;

        if (content == "Joined default.") {
            std::cout << "Autentizace úspěšná. Připojuji se k výchozímu kanálu..." << std::endl;
        }
    } else {
        std::cout << "Action Failure: " << content << std::endl;
        displayName.clear();  // Autentizace selhala, nesmíme pokračovat
    }

    std::cout << "[DEBUG] Posílám CONFIRM na REPLY (messageId: " << replyMsg.messageId << ")" << std::endl;

    UdpMessage confirmMsg = buildConfirmUdpMessage(replyMsg.messageId);
    std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);

    std::cout << "[DEBUG] Obsah CONFIRM zprávy (hex): ";
    for (auto byte : buffer) {
        std::cout << std::hex << std::uppercase << (int)byte << " ";
    }
    std::cout << std::dec << std::endl;

    sendto(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
}

void UdpChatClient::processMsgMessage(const UdpMessage& msgMsg) {
    if (msgMsg.payload.size() > 0) {
        std::string message(msgMsg.payload.begin(), msgMsg.payload.end());

        std::cout << "[DEBUG] Obdržena MSG zpráva: " << message << std::endl;

        if (message == "Joined default.") {
            std::cout << "Autentizace úspěšná. Připojuji se k výchozímu kanálu..." << std::endl;
        }

        UdpMessage confirmMsg = buildConfirmUdpMessage(msgMsg.messageId);
        UdpMessage msgMsg = buildMsgUdpMessage(displayName, message, nextMessageId++);

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
    std::cout << "Obdržena CONFIRM zpráva od serveru (RefID: " << confirmMsg.messageId << ")." << std::endl;


}


void UdpChatClient::sendPingMessage() {
    UdpMessage pingMsg;
    pingMsg.type = UdpMessageType::PING;
    pingMsg.messageId = nextMessageId++;
    pingMsg.payload.clear(); // PING zpráva neobsahuje žádné další data.

    std::vector<uint8_t> buffer = packUdpMessage(pingMsg);
    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0, 
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes < 0) {
        perror("ERROR: Odeslání UDP PING zprávy selhalo");
    } else {
        std::cout << "UDP PING zpráva odeslána." << std::endl;
    }
}
void UdpChatClient::processPingMessage(const UdpMessage& pingMsg) {
    (void)pingMsg; // Marking the parameter as unused

    std::cout << "Obdržena PING zpráva od serveru." << std::endl;

    UdpMessage confirmMsg;
    confirmMsg.type = UdpMessageType::CONFIRM;
    confirmMsg.messageId = nextMessageId++;
    confirmMsg.payload.clear(); // Confirm může mít prázdný payload.

    std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);
    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes < 0) {
        perror("ERROR: Odeslání CONFIRM zprávy pro PING selhalo");
    } else {
        std::cout << "CONFIRM zpráva pro PING odeslána." << std::endl;
    }
}
