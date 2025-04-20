#include "UdpReliableTransport.h"
#include "MessageUdp.h" 
#include <sys/select.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>

// Constructor to initialize the UDP transport with socket and server address
UdpReliableTransport::UdpReliableTransport(int sockfd, const struct sockaddr_in& serverAddr)
    : sockfd(sockfd), serverAddr(serverAddr) {
}

// Waits for a CONFIRM message with the expected message ID within the timeout
//Funkcion write with AI
bool UdpReliableTransport::waitForConfirm(uint16_t expectedMessageId, int timeoutMs) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    // Set the timeout
    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    // Wait for incoming data on the socket
    int ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
    if (ready > 0 && FD_ISSET(sockfd, &readfds)) {
        uint8_t buffer[1024];
        struct sockaddr_in fromAddr;
        socklen_t addrLen = sizeof(fromAddr);
        ssize_t received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                    (struct sockaddr*)&fromAddr, &addrLen);
        if (received > 0) {
            std::vector<uint8_t> data(buffer, buffer + received);
            UdpMessage receivedMsg;

            // Try to unpack the message
            if (unpackUdpMessage(data, receivedMsg)) {
                // Check if it is a CONFIRM and the ID matches
                if (receivedMsg.type == UdpMessageType::CONFIRM &&
                    receivedMsg.payload.size() >= 2) {
                    uint16_t refId;
                    std::memcpy(&refId, receivedMsg.payload.data(), sizeof(uint16_t));
                    refId = ntohs(refId);
                    if (refId == expectedMessageId) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// Sends a message and waits for confirmation. Retries if confirmation is not received.
bool UdpReliableTransport::sendMessageWithConfirm(const std::vector<uint8_t>& buffer,
                                                   uint16_t messageId,
                                                   int timeoutMs,
                                                   int maxRetries) {
    PendingMessage pending;
    pending.buffer = buffer;
    pending.retransmissionCount = 0;
    pending.lastSentTime = std::chrono::steady_clock::now();

    pendingMessages[messageId] = pending;

    while (pendingMessages[messageId].retransmissionCount < maxRetries) {
        // Send the UDP packet
        ssize_t sentBytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                                   (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            perror("ERROR: Odeslání zprávy selhalo");  // Sending failed
            return false;
        }

        // Wait for confirmation
        if (waitForConfirm(messageId, timeoutMs)) {
            pendingMessages.erase(messageId);
            return true;
        } else {
            // Retry sending
            pendingMessages[messageId].retransmissionCount++;
            std::cout << "Timeout, opakuji odeslání zprávy s MessageID: " << messageId
                      << " (pokus " << (int)pendingMessages[messageId].retransmissionCount << ")" << std::endl;
        }
    }

    // Max retries reached, confirmation not received
    std::cout << "Zpráva s MessageID: " << messageId << " nebyla potvrzena po " << maxRetries << " pokusech." << std::endl;
    pendingMessages.erase(messageId);
    return false;
}

// Handles an incoming UDP packet (for example, a CONFIRM)
void UdpReliableTransport::processIncomingPacket(const std::vector<uint8_t>& buffer) {
    UdpMessage receivedMsg;
    if (unpackUdpMessage(buffer, receivedMsg)) {
        if (receivedMsg.type == UdpMessageType::CONFIRM) {
            if (receivedMsg.payload.size() >= 2) {
                uint16_t refId;
                std::memcpy(&refId, receivedMsg.payload.data(), sizeof(uint16_t));
                refId = ntohs(refId);
               
                // Remove the message from pending list if confirmed
                pendingMessages.erase(refId);
                std::cout << "Obdrženo CONFIRM pro MessageID: " << refId << std::endl;
            }
        } else {
            // Handle other message types if needed
        }
    }
}
