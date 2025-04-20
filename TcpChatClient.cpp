#include "TcpChatClient.h"
#include "InputHandler.h"
#include "debug.h"
#include <iostream>
#include <string>
#include <cstring>       // memset
#include <sys/types.h>
#include <sys/socket.h>  // socket, connect, AF_INET, SOCK_STREAM
#include <netdb.h>       // getaddrinfo, addrinfo, freeaddrinfo
#include <unistd.h>      // close()
#include <cerrno>        // perror
#include <thread>
#include <sys/select.h>  // for select()
#include "MessageTcp.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>   // for istringstream
#include <cstdlib>   // for std::exit
#include <algorithm>

// Constructor that initializes the server address and port
TcpChatClient::TcpChatClient(const std::string& host, int port)
    : server(host), port(port), sockfd(-1),  authenticated(false) {}

// Destructor that closes the socket if it's open
TcpChatClient::~TcpChatClient() {
    if (sockfd != -1) close(sockfd);
}

// The connectToServer function is based on the example code “Simple Stream Client” from Beej’s Guide to Network Programming, section 6.2.
// https://beej.us/guide/bgnet/html/split/client-server-background.html#a-simple-stream-client
// The original example supported both IPv4 and IPv6 and demonstrated recv/print.
// This version was modified to:
//  - hints.ai_family = AF_INET → only IPv4  
//  - removed cyclic recv/print
//  - prints the connected IPv4 address using inet_ntop()  
//  - releases the struct addrinfo by calling freeaddrinfo()  
// This version only works with a TCP connection and prints IPv4-only addresses.

bool TcpChatClient::connectToServer() {
    struct addrinfo hints{}, *servinfo, *p;
    int rv;

    // Initialize the hints structure for getaddrinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;       // Only IPv4
    hints.ai_socktype = SOCK_STREAM;   // TCP

    // Get address info for the specified server and port
    if ((rv = getaddrinfo(server.c_str(), std::to_string(port).c_str(), &hints, &servinfo)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << "\n";
        return false;
    }

    // Loop through all the results and connect to the first available one
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) { 
            perror("socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd); 
            perror("connect");
            continue;
        }
        break;
    }

    // If no connection was made, print an error and return false
    if (p == nullptr) {
        std::cerr << "client: failed to connect\n";
        freeaddrinfo(servinfo);
        return false;
    }

    // Print the server's IPv4 address
    auto *ipv4 = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
    char ip4[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ipv4->sin_addr, ip4, sizeof(ip4));
    std::cerr << "client: connecting to " << ip4 << std::endl;

    // Free the address info structure as we no longer need it
    freeaddrinfo(servinfo);

    // Print a success message
  printf_debug("TCP client connected successfully.");
    return true;
}


Message TcpChatClient::parseMessage(const std::string& buffer) {
    // Check the first word in the buffer to determine the message type
    if (buffer.find("AUTH") == 0) {
        return Message::fromBuffer(buffer);  // Handle AUTH message
    } else if (buffer.find("JOIN") == 0) {
        return Message::fromBuffer(buffer);  // Handle JOIN message
    } else if (buffer.find("BYE") == 0) {
        return Message::fromBuffer(buffer);  // Handle BYE message
    } else if (buffer.find("ERR") == 0) {
        return Message::fromBuffer(buffer);  // Handle ERR message
    } else if (buffer.find("REPLY") == 0) {
        return Message::fromBuffer(buffer);  // Handle REPLY message
    } else if (buffer.find("MSG") == 0) {
        return Message::fromBuffer(buffer);  // Handle MSG message
    }
    return Message(Message::Type::MSG, "Unknown message type");  // Return a message indicating an unknown type
}

// Function to receive and process server responses
void TcpChatClient::receiveServerResponse() {
    char buffer[2048];   // Buffer to store incoming data
    std::string leftover; // String to store leftover data if the message is not complete

    while (true) {
        ssize_t n = read(sockfd, buffer, sizeof(buffer));  // Read data from the socket
        if (n <= 0) std::exit(0); // If no data or error, exit

        leftover.append(buffer, n);  // Append the data to leftover string

        size_t pos;
        // Process each line received from the server
        while ((pos = leftover.find("\r\n")) != std::string::npos) {
            std::string line = leftover.substr(0, pos); // Extract a complete line
            leftover.erase(0, pos + 2);  // Remove processed line from leftover

            // If the server sends a message about joining the default channel
            if (line.find("MSG FROM Server IS") == 0 && line.find("joined default") != std::string::npos) {
                std::cout << "Server: " << line << std::endl;
                sendChannelJoinConfirmation();  // Send confirmation of joining the default channel
                continue;
            }

            // If the message is invalid (not matching any known type)
            if (line.rfind("AUTH", 0) != 0 && line.rfind("JOIN", 0) != 0 && 
                line.rfind("REPLY", 0) != 0 && line.rfind("MSG", 0) != 0 &&
                line.rfind("ERR", 0) != 0 && line.rfind("BYE", 0) != 0) {
                processInvalidMessage(line);  // Handle invalid message
                continue;
            }

            // Process ERROR messages
            else if (line.rfind("ERR", 0) == 0) {
                // Format: "ERR FROM DisplayName IS MessageContent"
                std::istringstream iss(line);
                std::string tag, from, sender, is;
                iss >> tag >> from >> sender >> is;

                std::string content;
                std::getline(iss, content);  // Extract message content
                if (!content.empty() && content.front() == ' ') content.erase(0, 1);

                std::cout << "ERROR FROM " << sender << ": " << content << std::endl; // Print the error
                std::exit(1); // Exit the program
            }

            // Process BYE messages (exit the program)
            else if (line.rfind("BYE", 0) == 0) {
                std::exit(0);  // Exit the program
            }
            // Process REPLY messages
            else if (line.rfind("REPLY", 0) == 0) {
                Message reply = Message::fromBuffer(line + "\r\n");
                process_reply(reply);  // Process the REPLY message
            }
            // Process regular MSG messages
            else if (line.rfind("MSG", 0) == 0) {
                std::istringstream iss(line);
                std::string tag, from, sender, is;
                iss >> tag >> from >> sender >> is;
                std::string content;
                std::getline(iss, content);  // Extract message content
                if (!content.empty() && content.front() == ' ') content.erase(0, 1);
                std::cout << sender << ": " << content << "\n";  // Print the message
            }
        }
    }
}
//  Function 'run' is the main loop of the TcpChatClient class that handles user input, message processing, and communication with the server.
void TcpChatClient::run() {
    std::string line;
    std::string messageToSend;

    // Start a new thread to receive server responses
    std::thread receiverThread(&TcpChatClient::receiveServerResponse, this);

    // Loop to read commands and messages from user input
    while (std::getline(std::cin, line)) {
        std::string messageToSend;

        // Process the /help command to show usage instructions
        if (line.rfind("/help", 0) == 0) {
            printHelp();
            continue;
        }

        // If user tries to authenticate when already authenticated, show an error
        if (line.rfind("/auth", 0) == 0) {
            if (authenticated) {
                std::cout << "ERROR: You are already authenticated!" << std::endl;
                return;  // Do not authenticate again
            }

            // Process the /auth command if the user is not authenticated
            auto cmd = InputHandler::parseAuthCommand(line);
            if (cmd) {
                messageToSend = "AUTH " + cmd->username + " AS " + cmd->displayName + " USING " + cmd->secret + "\r\n";
                // After successful authentication, set displayName and authenticated to true
                displayName = cmd->displayName;
            } else {
                  printf_debug("Invalid /auth command format.");
                continue;
            }
        }
        // If the user is not authenticated and tries to send a command other than /auth, show an error
        else if (displayName.empty() && line.rfind("/auth", 0) != 0) {
            std::cout << "ERROR: not authenticated\n";
            continue;
        }

        // Process the /join command to join a channel
        else if (line.rfind("/join", 0) == 0) {
            auto cmd = InputHandler::parseJoinCommand(line);
            if (cmd) {
                messageToSend = "JOIN " + cmd.value() + " AS " + displayName + "\r\n";
            } else {
                  printf_debug("Invalid /join command format.");
                continue;
            }
        }

        // Process the /rename command to change the display name
        else if (line.rfind("/rename", 0) == 0) {
            std::string newDisplayName = line.substr(8);  // Trim the "/rename " part from the line
            if (newDisplayName.empty()) {
                printf_debug("Invalid /rename command format: Display name cannot be empty.");

                continue;
            }
            displayName = newDisplayName;  // Change the display name
            printf_debug("Display name changed to: %s", displayName.c_str());
            continue;
        }

        // If the line is not a command, treat it as a message to be sent to the server
        else {
            messageToSend = "MSG FROM " + displayName + " IS " + line + "\r\n";
        }

        // Send the message or command to the server
        std::cerr << "Sending: " << messageToSend << std::endl;
        if (send(sockfd, messageToSend.c_str(), messageToSend.size(), 0) == -1) {
            std::perror("ERROR: send failed");  // If sending the message fails, print an error
            break;
        }
    }

    sendByeMessage();  // Call sendByeMessage to send a "BYE" message after communication ends
    receiverThread.join();  // Wait for the receiver thread to finish
}


// Function to display help message with available commands
void TcpChatClient::printHelp() {
    std::cout << "/auth {Username} {Secret} {DisplayName} - Authenticate user\n";  // Command to authenticate
    std::cout << "/join {ChannelID} - Join a channel\n";  // Command to join a specified channel
    std::cout << "/rename {DisplayName} - Change your display name\n";  // Command to change the user's display name
    std::cout << "/help - Show this help message\n";  // Command to show help information
}

// Function to send a "BYE" message to the server to indicate the end of the session
void TcpChatClient::sendByeMessage() {
    if (!displayName.empty()) {  // Check if the display name is set
        Message byeMessage = Message::createByeMessage(displayName);  // Create a BYE message
        std::cerr << "Sending BYE message: " << byeMessage.getContent() << "\r\n" << std::endl;  // Display the message content
        byeMessage.sendMessage(sockfd);  // Send the BYE message to the server
    }
}

// Function to process the reply message from the server
void TcpChatClient::process_reply(const Message& reply) {
    if (reply.getType() == Message::REPLY) {  // Check if the reply message type is REPLY
        std::string content = reply.getContent();  // Extract the content of the reply
        auto pos = content.find(' ');  // Find the position of the first space (separating status from message)
        if (pos == std::string::npos) {  // If the space is not found, it's an invalid REPLY
            std::cerr << "Malformed REPLY: " << content << std::endl;
            return;
        }
        std::string status = content.substr(0, pos);  // Extract the status ("OK" or "NOK")
        std::string msg = content.substr(pos + 1);     // Extract the message content

        // Process the reply based on the status
        if (status == "OK") {
            std::cout << "Action Success: " << msg << std::endl;  // Success message
            authenticated = true;  // Set authenticated to true
        } else if (status == "NOK") {
            std::cout << "Action Failure: " << msg << std::endl;  // Failure message
            authenticated = false;  // Set authenticated to false
        } else {
            std::cerr << "ERROR: Unknown REPLY status: " << status << std::endl;  // Handle unexpected statuses
        }
    }
}

// Function to process an invalid message and send an error message to the server
void TcpChatClient::processInvalidMessage(const std::string& invalidMessage) {
    std::cerr << "ERROR: " << invalidMessage << std::endl;  // Print the invalid message error

    // Before sending the error message to the server, format it
    std::string errorMessage = "ERROR FROM " + displayName + " IS " + invalidMessage + "\r\n";
    send(sockfd, errorMessage.c_str(), errorMessage.size(), 0);  // Send the error message to the server
}

// Function to send a confirmation message when the user joins the default channel
void TcpChatClient::sendChannelJoinConfirmation() {
    // Create and send a confirmation message stating that the user joined the default channel
    std::string msg = "MSG FROM " + displayName + " IS " + displayName + " joined default.\r\n";
    printf_debug("Sending: %s", msg.c_str());
    if (send(sockfd, msg.c_str(), msg.size(), 0) == -1) {  // Send the confirmation message
        std::perror("ERROR: send failed");  // If sending fails, display an error
        std::exit(1);  // Exit the program if sending the message fails
    }
}
