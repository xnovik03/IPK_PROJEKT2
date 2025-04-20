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

// Constructor for initializing the UDP client with the server address and port
// It also initializes the sockfd to -1, nextMessageId to 0, and displayName to an empty string
UdpChatClient::UdpChatClient(const std::string& server, int port)
    : serverAddress(server), serverPort(port), sockfd(-1),
      nextMessageId(0), displayName("") {
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
        std::cout << "UDP client is using local port: " << ntohs(localAddr.sin_port) << std::endl;  // Print the assigned local port
    }
    return true;
}

// Resolves the server address from the given string (e.g., IP address or hostname)
// If the address is valid, it fills the serverAddr structure
bool UdpChatClient::resolveServerAddr() {
    std::memset(&serverAddr, 0, sizeof(serverAddr));  // Clear the structure
    serverAddr.sin_family = AF_INET;  // Set the address family to IPv4
    serverAddr.sin_port = htons(serverPort);  // Set the server port

    // Convert the server address string to network address format
    if (inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "ERROR: Invalid server IP address: " << serverAddress << "\n";  // Print error if address conversion fails
        return false;
    }
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
    std::cout << "UDP client ready to send messages to " << serverAddress 
              << ":" << serverPort << std::endl;  // Print confirmation of the connection
    return true;
}

// Main loop for handling user input and processing server responses
// This is where the client waits for input, processes commands, and sends messages
void UdpChatClient::run() {
    std::cout << "UDP client started. Enter a command: " << std::endl;
    std::string input;
    while (std::getline(std::cin, input)) {  // Read user input

        if (input.empty()) continue;  // Skip empty inputs

        // Check if the input is a command starting with '/'
        if (input[0] == '/') {
            handleCommand(input);  // Process the command
        } else {
            sendMessage(input);  // Otherwise, treat it as a message to send
        }

        // Process incoming responses from the server
        receiveServerResponseUDP();  // This function processes incoming messages
    }
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
        std::cout << "ERROR: Unknown command: " << input << std::endl;  // Show error for unknown commands
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
    auto authOpt = InputHandler::parseAuthCommand(input);  // Parse the /auth command
    if (authOpt) {
        this->displayName = authOpt->displayName;  // Set the display name after successful authentication
        // Build the authentication message and send it to the server
        UdpMessage authMsg = buildAuthUdpMessage(*authOpt, nextMessageId++);
        std::vector<uint8_t> buffer = packUdpMessage(authMsg);
        ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0, 
                                   (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            perror("ERROR: Sending UDP AUTH message failed");
        } else {
            std::cout << "UDP AUTH message sent." << std::endl;
        }
        
        // Wait for the server's REPLY response
        receiveServerResponseUDP();  // This function waits and processes the server's reply
    } else {
        std::cout << "Invalid /auth command. Correct format: /auth {Username} {Secret} {DisplayName}" << std::endl;
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
            std::cout << "UDP JOIN message sent." << std::endl;
        }
    } else {
        std::cout << "ERROR: Invalid /join command. Correct format: /join {ChannelID}" << std::endl;
    }
}

// Handle the rename command (/rename)
// This function allows the user to change their display name
void UdpChatClient::handleRenameCommand(const std::string& input) {
    std::string newDisplayName = input.substr(8);  // Extract the new display name from the input
    newDisplayName.erase(0, newDisplayName.find_first_not_of(" \t"));  // Trim leading whitespace
    newDisplayName.erase(newDisplayName.find_last_not_of(" \t") + 1);  // Trim trailing whitespace

    if (newDisplayName.empty()) {
        std::cout << "ERROR: Invalid /rename command: Display name cannot be empty." << std::endl;
        return;
    }

    if (displayName.empty()) {
        std::cout << "ERROR: You must authenticate first (/auth)." << std::endl;
        return;
    }

    displayName = newDisplayName;  // Update the display name
    std::cout << "Display name changed to: " << displayName << std::endl;
    std::cout << "[DEBUG] New displayName set: " << displayName << std::endl;  // Debugging output
}

// Send a message to the server
// If the user is authenticated, this function sends the provided message
void UdpChatClient::sendMessage(const std::string& message) {
    if (displayName.empty()) {  // Check if the user is authenticated
        std::cout << "ERROR: You must authenticate first (/auth) before sending a message." << std::endl;
    } else {
        std::cout << "[DEBUG] Sending message as: " << displayName << std::endl;  // Debugging output

        // Build the message and send it to the server
        UdpMessage msgMsg = buildMsgUdpMessage(displayName, message, nextMessageId++);
        std::vector<uint8_t> buffer = packUdpMessage(msgMsg);
        ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0, 
                                   (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            perror("ERROR: Sending UDP MSG message failed");
        } else {
            std::cout << "UDP MSG message sent." << std::endl;
        }
    }
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
        std::cout << "DEBUG: Sending BYE message with MessageID " << byeMsg.messageId << ", payload size = " << byeMsg.payload.size() << std::endl;
        ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0, 
                                   (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            perror("ERROR: Sending UDP BYE message failed");
        } else {
            std::cout << "UDP BYE message sent." << std::endl;
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
                processReplyMessage(receivedMsg);  // Handle REPLY message
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

            default:
                std::cerr << "ERROR: Unknown message type: " << static_cast<int>(receivedMsg.type) << std::endl;
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
        std::cout << "UDP CONFIRM message sent." << std::endl;
    }

    // Exit the application after processing the error
    std::exit(EXIT_FAILURE);
}

// Process the reply message (REPLY)
// This function checks the result and processes the content accordingly
void UdpChatClient::processReplyMessage(const UdpMessage& replyMsg) {
    if (replyMsg.payload.size() < 3) {
        std::cerr << "ERROR: REPLY message is too short." << std::endl;
        return;
    }

    uint8_t result = replyMsg.payload[0];  // Result: 1 for success, 0 for failure
    uint16_t refMessageId = ntohs(*(uint16_t*)&replyMsg.payload[1]);  // Reference message ID
    std::string content(replyMsg.payload.begin() + 3, replyMsg.payload.end());  // Message content

    // Handle success or failure based on the result
    if (result == 1) {
        std::cout << "Action Success: " << content << std::endl;

        if (content == "Joined default.") {
            std::cout << "Authentication successful. Joining default channel..." << std::endl;
        }
    } else {
        std::cout << "Action Failure: " << content << std::endl;
        displayName.clear();  // Authentication failed, clear display name
    }

    // Debugging: Print the message ID and prepare the CONFIRM message
    std::cout << "[DEBUG] Sending CONFIRM for REPLY (messageId: " << replyMsg.messageId << ")" << std::endl;

    UdpMessage confirmMsg = buildConfirmUdpMessage(replyMsg.messageId);
    std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);

    std::cout << "[DEBUG] Confirm message content (hex): ";
    for (auto byte : buffer) {
        std::cout << std::hex << std::uppercase << (int)byte << " ";  // Print in hexadecimal
    }
    std::cout << std::dec << std::endl;  // Switch back to decimal output

    sendto(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
}

// Process the message (MSG) received from the server
void UdpChatClient::processMsgMessage(const UdpMessage& msgMsg) {
    if (msgMsg.payload.size() > 0) {
        std::string message(msgMsg.payload.begin(), msgMsg.payload.end());
        std::cout << "[DEBUG] Received MSG message: " << message << std::endl;

        if (message == "Joined default.") {
            std::cout << "Authentication successful. Joining default channel..." << std::endl;
        }

        // Prepare the CONFIRM message and send it back
        UdpMessage confirmMsg = buildConfirmUdpMessage(msgMsg.messageId);
        UdpMessage msgMessage = buildMsgUdpMessage(displayName, message, nextMessageId++);

        std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);
        ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                                   (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            perror("ERROR: Sending UDP CONFIRM message failed");
        } else {
            std::cout << "UDP CONFIRM message sent." << std::endl;
        }
    }
}

// Process the CONFIRM message received from the server
void UdpChatClient::processConfirmMessage(const UdpMessage& confirmMsg) {
    std::cout << "Received CONFIRM message from server (RefID: " << confirmMsg.messageId << ")." << std::endl;
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
        std::cout << "UDP PING message sent." << std::endl;
    }
}

// Process the PING message received from the server
void UdpChatClient::processPingMessage(const UdpMessage& pingMsg) {
    (void)pingMsg; // Marking the parameter as unused
    std::cout << "Received PING message from server." << std::endl;

    UdpMessage confirmMsg;
    confirmMsg.type = UdpMessageType::CONFIRM;  // Set message type to CONFIRM
    confirmMsg.messageId = nextMessageId++;  // Assign a new message ID
    confirmMsg.payload.clear();  // Confirm can have an empty payload

    std::vector<uint8_t> buffer = packUdpMessage(confirmMsg);
    ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes < 0) {
        perror("ERROR: Sending CONFIRM message for PING failed");
    } else {
        std::cout << "CONFIRM message for PING sent." << std::endl;
    }
}
