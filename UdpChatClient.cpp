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
#include "debug.h"
#include <netdb.h>
// Constructor for initializing the UDP client with the server address and port
// It also initializes the sockfd to -1, nextMessageId to 0, and displayName to an empty string
UdpChatClient::UdpChatClient(const std::string& server, int port, int timeoutMs, int retries)
    : serverAddress(server),
      serverPort(port),
      sockfd(-1),
      nextMessageId(0),
      displayName(""),
      timeoutMs(timeoutMs),
      maxRetries(retries) {
}

// Destructor to clean up resources by closing the socket if it is open
UdpChatClient::~UdpChatClient() {
    if (sockfd != -1) {
        close(sockfd);  // Close the socket if it's open
    }
}

// Binds the UDP socket to a local address and port
// This allows the client to send and receive UDP messages
bool UdpChatClient::bindSocket() {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Create the UDP socket
    if (sockfd < 0) {
        perror("ERROR: Unable to create UDP socket");  // If socket creation fails, print an error
        return false;
    }

    struct sockaddr_in localAddr;
    std::memset(&localAddr, 0, sizeof(localAddr));  // Initialize the structure to 0
    localAddr.sin_family = AF_INET;  // IPv4 address family
    localAddr.sin_addr.s_addr = INADDR_ANY;  // Accept messages from any address
    localAddr.sin_port = htons(0);  // Bind to an available port

    // Bind the socket to the local address
    if (bind(sockfd, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
        perror("ERROR: Bind failed");  // If bind fails, print an error and close the socket
        close(sockfd);
        sockfd = -1;
        return false;
    }

    socklen_t len = sizeof(localAddr);
    if (getsockname(sockfd, (struct sockaddr*)&localAddr, &len) == 0) {
       printf_debug("UDP client is using local port: %d", ntohs(localAddr.sin_port));
     }
    return true;
}

// Resolves the server address from the given string (e.g., IP address or hostname)
// If the address is valid, it fills the serverAddr structure
bool UdpChatClient::resolveServerAddr() {
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    struct addrinfo hints{}, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP

    int status = getaddrinfo(serverAddress.c_str(), nullptr, &hints, &res);
    if (status != 0 || res == nullptr) {
        std::cerr << "ERROR: getaddrinfo failed for " << serverAddress << ": " << gai_strerror(status) << std::endl;
        return false;
    }

    struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
    serverAddr.sin_addr = ipv4->sin_addr;

    freeaddrinfo(res);  // Always free the result!
    return true;
}

// Connects to the server by binding the socket and resolving the server address
// It prints a message indicating that the client is ready to send messages
bool UdpChatClient::connectToServer() {
    if (!bindSocket()) {  // Attempt to bind the socket
        return false;
    }
    if (!resolveServerAddr()) {  // Attempt to resolve the server address
        return false;
    }
    std::cerr << "UDP client ready to send messages to " << serverAddress 
              << ":" << serverPort << std::endl;  // Print confirmation of the connection
    return true;
}

// Main loop for handling user input and processing server responses
// This is where the client waits for input, processes commands, and sends messages
void UdpChatClient::run() {
    std::cerr << "UDP client started. Enter a command: " << std::endl;

    // Start background receiver
    receiverThread = std::thread(&UdpChatClient::backgroundReceiverLoop, this);

    retransmissionThread = std::thread([this]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  
            checkRetransmissions();
        }
    });

    running = true;

    std::string input;
    while (true) {
        if (!std::getline(std::cin, input)) {
            std::cerr << "Stdin closed. Sending BYE and exiting." << std::endl;
            sendByeMessage();
            break;
        }

        if (input.empty()) continue;

        if (input[0] == '/') {
            handleCommand(input);
        } else {
            sendMessage(input);
        }
    }

    running = false;
    if (receiverThread.joinable()) receiverThread.join();
    if (retransmissionThread.joinable()) retransmissionThread.join();
}

// Handles different commands based on user input
// It processes commands like /help, /auth, /join, /rename, etc.
void UdpChatClient::handleCommand(const std::string& input) {
    if (input == "/help") {
        printHelp();  // Show help information for available commands
    } else if (input.rfind("/auth", 0) == 0) {
        handleAuthCommand(input);  // Handle authentication command
    } else if (displayName.empty()) {  // Check if the user is authenticated
        std::cout << "ERROR: You must authenticate first (/auth) before sending messages." << std::endl;  // Show error if not authenticated
    } else if (input.rfind("/join", 0) == 0) {
        handleJoinCommand(input);  // Handle join channel command
    } else if (input.rfind("/rename", 0) == 0) {
        handleRenameCommand(input); // Handle rename display name command
    } else {
        std::cerr << "ERROR: Unknown command: " << input << std::endl;  // Show error for unknown commands
          std::exit(1); 
    }
}


// Print available commands to the user
// This function displays the list of supported commands for the UDP client
void UdpChatClient::printHelp() {
    std::cout << "Supported commands:" << std::endl;
    std::cout << "  /auth {Username} {Secret} {DisplayName}  - Authenticate user" << std::endl;
    std::cout << "  /join {ChannelID}                     - Join a channel" << std::endl;
    std::cout << "  /rename {DisplayName}                  - Change your display name" << std::endl;
    std::cout << "  /help                                  - Show this help message" << std::endl;
}

// Handle the authentication command (/auth)
// This function parses the input, validates it, and sends the authentication message to the server
void UdpChatClient::handleAuthCommand(const std::string& input) {
    if (!displayName.empty()) {
        std::cout << "ERROR: You are already authenticated!" << std::endl;
        return;  // Do not authenticate again
    }

    auto authOpt = InputHandler::parseAuthCommand(input);  // Parse the /auth command
    if (authOpt) {
        this->displayName = authOpt->displayName;  // Set the display name after successful authentication

        //  Build the AUTH message
        UdpMessage authMsg = buildAuthUdpMessage(*authOpt, nextMessageId++);

        // Send the AUTH message using the reliable sending mechanism
        sendRawUdpMessage(authMsg);  

        printf_debug("UDP AUTH message sent.");

        // Wait for the server's REPLY response 
        receiveServerResponseUDP();
    } else {
        printf_debug("Invalid /auth command. Correct format: /auth {Username} {Secret} {DisplayName}");
    }
}


// Handle the join command (/join)
// This function parses the /join command, checks if the user is authenticated,
// and sends a join message to the server
void UdpChatClient::handleJoinCommand(const std::string& input) {
    auto joinOpt = InputHandler::parseJoinCommand(input);  // Parse the /join command
    if (joinOpt) {
        if (displayName.empty()) {  // Check if the user is authenticated
            std::cout << "ERROR: You must authenticate first (/auth)." << std::endl;
            return;
        }
        // Build the join message and send it to the server
        UdpMessage joinMsg = buildJoinUdpMessage(joinOpt.value(), displayName, nextMessageId++);
        std::vector<uint8_t> buffer = packUdpMessage(joinMsg);
        ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0, 
                                   (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            perror("ERROR: Sending UDP JOIN message failed");
        } else {
             printf_debug("UDP JOIN message sent.");
        }
    } else {
          printf_debug("ERROR: Invalid /join command. Correct format: /join {ChannelID}");
    }
}

// Handle the rename command (/rename)
// This function allows the user to change their display name
void UdpChatClient::handleRenameCommand(const std::string& input) {
    std::string newDisplayName = input.substr(8);  // Extract the new display name from the input
    newDisplayName.erase(0, newDisplayName.find_first_not_of(" \t"));  // Trim leading whitespace
    newDisplayName.erase(newDisplayName.find_last_not_of(" \t") + 1);  // Trim trailing whitespace

    if (newDisplayName.empty()) {
         printf_debug("ERROR: Invalid /rename command: Display name cannot be empty.");
        return;
    }

    if (displayName.empty()) {
        std::cout << "ERROR: You must authenticate first (/auth)." << std::endl;
        return;
    }

    displayName = newDisplayName;  // Update the display name
    std::cerr << "Display name changed to: " << displayName << std::endl;
    std::cerr << "[DEBUG] New displayName set: " << displayName << std::endl;  // Debugging output
}

// Send a message to the server
// If the user is authenticated, this function sends the provided message
void UdpChatClient::sendMessage(const std::string& message) {
    if (displayName.empty()) {  // Check if the user is authenticated
        std::cout << "ERROR: You must authenticate first (/auth) before sending a message." << std::endl;
        return;
    }

    printf_debug("Sending message as '%s'", displayName.c_str());
    // Build the message and send it to the server
    UdpMessage msgMsg = buildMsgUdpMessage(displayName, message, nextMessageId++);
    sendRawUdpMessage(msgMsg);
}


// Send a BYE message to the server
// This function is used when the user wants to disconnect from the server
void UdpChatClient::sendByeMessage() {
    if (!displayName.empty()) {  // Check if the user is authenticated
        UdpMessage byeMsg;
        byeMsg.type = UdpMessageType::BYE;  // Set the message type to BYE
        byeMsg.messageId = nextMessageId++;  // Assign a unique message ID
        byeMsg.payload = packString(displayName);  // Pack the display name as the payload

        std::vector<uint8_t> buffer = packUdpMessage(byeMsg);
        std::cerr << "DEBUG: Sending BYE message with MessageID " << byeMsg.messageId
                  << ", payload size = " << byeMsg.payload.size() << std::endl;

        ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                                   (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            perror("ERROR: Sending UDP BYE message failed");
        } else {
            std::cerr << "UDP BYE message sent." << std::endl;
        }
    }
}

// This function listens for incoming UDP messages and processes them based on their type
void UdpChatClient::receiveServerResponseUDP() {
    uint8_t recvBuffer[1024];
    struct sockaddr_in fromAddr;
    socklen_t addrLen = sizeof(fromAddr);
    
    // Receive a message from the server
 ssize_t bytesReceived = recvfrom(sockfd, recvBuffer, sizeof(recvBuffer), 0,
                                     (struct sockaddr*)&fromAddr, &addrLen);
    if (bytesReceived <= 0) {
        std::cerr << "Error receiving message!" << std::endl;
        return;
    }

    std::vector<uint8_t> data(recvBuffer, recvBuffer + bytesReceived);
    UdpMessage receivedMsg;

    // Unpack the received UDP message
    if (unpackUdpMessage(data, receivedMsg)) {
        // Process the message based on its type
        switch (receivedMsg.type) {
            case UdpMessageType::REPLY:
                processReplyMessage(receivedMsg, fromAddr);  // Handle REPLY message
                break;

            case UdpMessageType::CONFIRM:
                processConfirmMessage(receivedMsg);  // Handle CONFIRM message
                break;

            case UdpMessageType::MSG:
                processMsgMessage(receivedMsg);  // Handle MSG message
                break;

            case UdpMessageType::ERR:
                processErrMessage(receivedMsg);  // Handle ERROR message
                break;
            case UdpMessageType::BYE:
                 processByeMessage(receivedMsg);  
                 break;
            case UdpMessageType::PING:
                 processPingMessage(receivedMsg);  
                 break;

   default:
    std::cout << "ERROR: Unknown message type: " << static_cast<int>(receivedMsg.type) << std::endl;

    // Send CONFIRM for unknown message type
    UdpMessage confirmMsg;
    confirmMsg.type = UdpMessageType::CONFIRM;
    confirmMsg.messageId = 0;
    confirmMsg.payload.resize(2);

    uint16_t netId = htons(receivedMsg.messageId);
    std::memcpy(confirmMsg.payload.data(), &netId, sizeof(uint16_t));

    std::vector<uint8_t> confirmBuf = packUdpMessage(confirmMsg);
    sendto(sockfd, confirmBuf.data(), confirmBuf.size(), 0,
           (struct sockaddr*)&fromAddr, sizeof(fromAddr));
    std::cerr << "UDP CONFIRM message sent for unknown message type." << std::endl;

    // Build and send ERR message
    UdpMessage errMsg;
    errMsg.type = UdpMessageType::ERR;
    errMsg.messageId = nextMessageId++;

    std::string errSender = displayName.empty() ? "client" : displayName;
    std::string errorMsg = "Unknown message type: " + std::to_string(static_cast<int>(receivedMsg.type));

    errMsg.payload.insert(errMsg.payload.end(), errSender.begin(), errSender.end());
    errMsg.payload.push_back('\0');
    errMsg.payload.insert(errMsg.payload.end(), errorMsg.begin(), errorMsg.end());
    errMsg.payload.push_back('\0');

    std::vector<uint8_t> errBuf = packUdpMessage(errMsg);
    ssize_t errSent = sendto(sockfd, errBuf.data(), errBuf.size(), 0,
                             (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    if (errSent < 0) {
        perror("ERROR: Sending ERR message for unknown type failed");
    } else {
        std::cerr << "ERR message sent for unknown message type." << std::endl;
    }

    break;


        }
    }
}

// Process error message (ERR)
// This function extracts and prints the error message and its details
void UdpChatClient::processErrMessage(const UdpMessage& errMsg) {
    // Check if the payload contains at least one byte
    if (errMsg.payload.size() < 1) {
        std::cerr << "ERR message is empty." << std::endl;
        return;
    }

    // Convert payload to a string
    std::string errorContent(errMsg.payload.begin(), errMsg.payload.end());

    // Extract the first null byte position
    size_t firstNullPos = errorContent.find('\0');
    if (firstNullPos != std::string::npos) {
        std::cout << "ERROR FROM " << errorContent.substr(0, firstNullPos) << ": ";
        
        // Extract the second null byte position and print error message
        size_t secondNullPos = errorContent.find('\0', firstNullPos + 1);
        if (secondNullPos != std::string::npos) {
            std::cout << errorContent.substr(firstNullPos + 1, secondNullPos - firstNullPos - 1) << std::endl;
        } else {
            std::cerr << "Error: Could not find the second null byte!" << std::endl;
        }
    } else {
        std::cerr << "Error: Could not find the first null byte!" << std::endl;
        return;
    }

    // Debugging: Print the payload size and raw data
    std::cerr << "Payload size: " << errMsg.payload.size() << std::endl;
    std::cerr << "Payload (raw data): ";
    for (auto byte : errMsg.payload) {
        std::cerr << std::hex << static_cast<int>(byte) << " "; // Print in hexadecimal
    }
    std::cerr << std::dec << std::endl; // Switch back to decimal output

    // Send a dynamic CONFIRM message with the message ID
    UdpMessage confirmMsg;
    confirmMsg.type = UdpMessageType::CONFIRM;
    confirmMsg.messageId = 0;  
    confirmMsg.payload.resize(sizeof(uint16_t));
    uint16_t netRefId = htons(errMsg.messageId);  // Network byte order
    std::memcpy(confirmMsg.payload.data(), &netRefId, sizeof(uint16_t));

    std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);
    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes < 0) {
        perror("ERROR: Sending UDP CONFIRM message failed");
    } else {
        std::cerr << "UDP CONFIRM message sent." << std::endl;
    }

    // Exit the application after processing the error
    std::exit(EXIT_FAILURE);
}

// Process the reply message (REPLY)
// This function checks the result and processes the content accordingly
void UdpChatClient::processReplyMessage(const UdpMessage& replyMsg, const sockaddr_in& fromAddr) {
    if (replyMsg.payload.size() < 3) {
        std::cerr << "ERROR: REPLY message is too short." << std::endl;
        return;
    }

    uint8_t result = replyMsg.payload[0];  // Result: 1 for success, 0 for failure
    std::string content(replyMsg.payload.begin() + 3, replyMsg.payload.end());  // Message content

    serverAddr = fromAddr;

    // Handle success or failure based on the result
    if (result == 1) {
        std::cout << "Action Success: " << content << std::endl;

        if (content == "Joined default.") {
            std::cerr << "Authentication successful. Joining default channel..." << std::endl;
        }
    } else {
        std::cout << "Action Failure: " << content << std::endl;
        displayName.clear();  // Authentication failed, clear display name
    }
    // Debugging: Print the message ID and prepare the CONFIRM message
    std::cerr << "[DEBUG] Sending CONFIRM for REPLY (messageId: " << replyMsg.messageId << ")" << std::endl;

    UdpMessage confirmMsg = buildConfirmUdpMessage(replyMsg.messageId);
    std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);

    std::cerr << "[DEBUG] Confirm message content (hex): ";
    for (auto byte : buffer) {
        std::cerr << std::hex << std::uppercase << (int)byte << " ";  // Print in hexadecimal
    }
    std::cerr << std::dec << std::endl;

    sendto(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
}

// Process the message (MSG) received from the server
void UdpChatClient::processMsgMessage(const UdpMessage& msgMsg) {
    if (msgMsg.payload.empty()) return;

    // Duplikáty
    if (receivedMsgIds.count(msgMsg.messageId)) {
        printf_debug("Duplicate MSG message received (ID %d), sending CONFIRM only", msgMsg.messageId);
    } else {
        receivedMsgIds.insert(msgMsg.messageId);

        auto it = std::find(msgMsg.payload.begin(), msgMsg.payload.end(), '\0');
        if (it == msgMsg.payload.end()) return; // není odděleno

        std::string displayName(msgMsg.payload.begin(), it);
        std::string content(it + 1, msgMsg.payload.end());

        std::cout << displayName << ": " << content << std::endl;
        printf_debug("Received MSG message: %s", content.c_str());
    }

    // Prepare the CONFIRM message and send it back
    UdpMessage confirmMsg = buildConfirmUdpMessage(msgMsg.messageId);
    std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);
    sendto(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    std::cerr << "UDP CONFIRM message sent." << std::endl;
}

// Process the CONFIRM message received from the server
void UdpChatClient::processConfirmMessage(const UdpMessage& confirmMsg) {
    std::cerr << "Received CONFIRM message from server (RefID: " << confirmMsg.messageId << ")." << std::endl;
    sentMessages.erase(confirmMsg.messageId);
}

// Send a PING message to the server
void UdpChatClient::sendPingMessage() {
    UdpMessage pingMsg;
    pingMsg.type = UdpMessageType::PING;  // Set message type to PING
    pingMsg.messageId = nextMessageId++;  // Assign a new message ID
    pingMsg.payload.clear();  // PING message has no payload

    std::vector<uint8_t> buffer = packUdpMessage(pingMsg);
    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0, 
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes < 0) {
        perror("ERROR: Sending UDP PING message failed");
    } else {
        std::cerr << "UDP PING message sent." << std::endl;
    }
}

// Process the PING message received from the server
void UdpChatClient::processPingMessage(const UdpMessage& pingMsg) {
    printf_debug("Received PING message from server.");

    UdpMessage confirmMsg;
    confirmMsg.type = UdpMessageType::CONFIRM; // Set message type to CONFIRM
    confirmMsg.messageId = 0;  
    confirmMsg.payload.resize(sizeof(uint16_t));

    uint16_t netRefId = htons(pingMsg.messageId);  
    std::memcpy(confirmMsg.payload.data(), &netRefId, sizeof(uint16_t));

    std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);
    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes < 0) {
        perror("ERROR: Sending CONFIRM message for PING failed");
    } else {
        printf_debug("CONFIRM message for PING sent.");
    }
}

void UdpChatClient::processByeMessage(const UdpMessage& byeMsg) {
    std::cerr << "Received BYE message from server. Terminating client." << std::endl;

    UdpMessage confirmMsg = buildConfirmUdpMessage(byeMsg.messageId);
    std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);
    sendto(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    std::exit(EXIT_SUCCESS);
}
void UdpChatClient::backgroundReceiverLoop() {
    while (running) {
        receiveServerResponseUDP();
    }
}
void UdpChatClient::checkRetransmissions() {
    auto now = std::chrono::steady_clock::now();

    for (auto it = sentMessages.begin(); it != sentMessages.end(); ) {
        auto& msg = it->second;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - msg.timestamp);

        printf_debug("Checking message ID %d: elapsed = %ld ms", msg.messageId, elapsed.count());

        if (elapsed.count() >= timeoutMs) {
            if (msg.retryCount >= maxRetries) {
                std::cout << "ERROR: Confirmation not received for message ID " << msg.messageId << std::endl;
                it = sentMessages.erase(it); 
                continue;
            }

            // Retransmise
            printf_debug("[RETRANS] Resending message ID %d", msg.messageId);
            sendto(sockfd, msg.data.data(), msg.data.size(), 0,
                   (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            msg.timestamp = now;
            msg.retryCount++;
        }

        ++it;
    }
}

void UdpChatClient::sendRawUdpMessage(const UdpMessage& msg) {
    std::vector<uint8_t> buffer = packUdpMessage(msg);
    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0, 
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes < 0) {
        perror("ERROR: Sending UDP message failed");
        return;
    }

    printf_debug("Sent message with ID %d (type %d, size %zu)", msg.messageId, static_cast<int>(msg.type), buffer.size());

    sentMessages[msg.messageId] = {
        buffer,
        msg.messageId,
        std::chrono::steady_clock::now()
    };
}
