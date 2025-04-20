#ifndef UDPRELIABLETRANSPORT_H
#define UDPRELIABLETRANSPORT_H

#include <vector>
#include <cstdint>
#include <chrono>
#include <unordered_map>
#include <netinet/in.h> 

// Structure to store information about a sent message that is waiting for confirmation.
struct PendingMessage {
    std::vector<uint8_t> buffer;  // The message data to be sent
    uint8_t retransmissionCount;  // Number of retransmission attempts
    std::chrono::steady_clock::time_point lastSentTime;  // Time when the message was last sent
};

// Class that implements reliable UDP transport by waiting for confirmation (CONFIRM messages).
class UdpReliableTransport {
public:
    // Constructor that initializes the UDP socket and server address
    UdpReliableTransport(int sockfd, const struct sockaddr_in& serverAddr);

    // Sends a message and waits for confirmation with optional timeout and retry limit
    bool sendMessageWithConfirm(const std::vector<uint8_t>& buffer,
                                uint16_t messageId,
                                int timeoutMs = 250,
                                int maxRetries = 3);

    // Processes an incoming packet and handles any expected CONFIRM messages
    void processIncomingPacket(const std::vector<uint8_t>& buffer);

private:
    int sockfd;  // UDP socket file descriptor
    struct sockaddr_in serverAddr;  // Server address to send messages to

    // A map of message IDs to pending messages waiting for confirmation
    std::unordered_map<uint16_t, PendingMessage> pendingMessages;

    // Internal helper function to wait for a CONFIRM message for a specific message ID
    bool waitForConfirm(uint16_t expectedMessageId, int timeoutMs);
};

#endif // UDPRELIABLETRANSPORT_H
