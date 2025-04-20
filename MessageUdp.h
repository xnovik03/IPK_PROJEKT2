#ifndef MESSAGEUDP_H
#define MESSAGEUDP_H

#include <cstdint>
#include <vector>
#include <string>

// Enumeration for all UDP message types as specified in the protocol
enum class UdpMessageType : uint8_t {
    CONFIRM = 0x00,  // Confirmation message
    REPLY   = 0x01,  // Server response (OK/NOK)
    AUTH    = 0x02,  // Authentication request
    JOIN    = 0x03,  // Join a channel
    MSG     = 0x04,  // Standard message
    PING    = 0xFD,  // Ping message (keep-alive)
    ERR     = 0xFE,  // Error message
    BYE     = 0xFF   // Terminate connection
};

// Structure representing a UDP message to be sent or received
struct UdpMessage {
    UdpMessageType type;       // Message type (1 byte)
    uint16_t messageId;        // Message ID (2 bytes) for deduplication/retry
    std::vector<uint8_t> payload; // Message content (can be empty)
};

// Converts a UdpMessage struct into a raw byte buffer for sending via UDP
std::vector<uint8_t> packUdpMessage(const UdpMessage& msg);

// Converts a received raw byte buffer into a UdpMessage struct
bool unpackUdpMessage(const std::vector<uint8_t>& buffer, UdpMessage& msg);

#endif // MESSAGEUDP_H
