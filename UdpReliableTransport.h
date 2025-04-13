#ifndef UDPRELIABLETRANSPORT_H
#define UDPRELIABLETRANSPORT_H

#include <vector>
#include <cstdint>
#include <chrono>
#include <unordered_map>
#include <netinet/in.h> 
// Struktura pro zaznamenání informací o odeslaných zprávách, pro které čekáme potvrzení.
struct PendingMessage {
    std::vector<uint8_t> buffer; 
    uint8_t retransmissionCount; 
    std::chrono::steady_clock::time_point lastSentTime; 
};


class UdpReliableTransport {
public:
   
    UdpReliableTransport(int sockfd, const struct sockaddr_in& serverAddr);

 
    bool sendMessageWithConfirm(const std::vector<uint8_t>& buffer,
                                uint16_t messageId,
                                int timeoutMs = 250,
                                int maxRetries = 3);


    void processIncomingPacket(const std::vector<uint8_t>& buffer);

private:
    int sockfd;
    struct sockaddr_in serverAddr;
   
    std::unordered_map<uint16_t, PendingMessage> pendingMessages;

    bool waitForConfirm(uint16_t expectedMessageId, int timeoutMs);
};

#endif // UDPRELIABLETRANSPORT_H
