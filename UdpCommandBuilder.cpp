#include "UdpCommandBuilder.h"
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <iterator>
#include <arpa/inet.h>
#include <cstring>

// Converts a string to a vector<uint8_t> and appends a null terminator (0).
std::vector<uint8_t> packString(const std::string& s) {
    std::vector<uint8_t> result(s.begin(), s.end());
    result.push_back(0); 
    return result;
}

// Builds an AUTH message in UDP format using the provided AuthCommand data.
UdpMessage buildAuthUdpMessage(const AuthCommand& cmd, uint16_t messageId) {
    UdpMessage msg;
    msg.type = UdpMessageType::AUTH;
    msg.messageId = messageId;
    
    std::vector<uint8_t> payload;

    // Add username (null-terminated)
    std::vector<uint8_t> usernameBytes = packString(cmd.username);
    payload.insert(payload.end(), usernameBytes.begin(), usernameBytes.end());

    // Add display name (null-terminated)
    std::vector<uint8_t> displayNameBytes = packString(cmd.displayName);
    payload.insert(payload.end(), displayNameBytes.begin(), displayNameBytes.end());

    // Add secret (null-terminated)
    std::vector<uint8_t> secretBytes = packString(cmd.secret);
    payload.insert(payload.end(), secretBytes.begin(), secretBytes.end());

    msg.payload = payload;
    return msg;
}

// Builds a JOIN message in UDP format with channel and display name.
UdpMessage buildJoinUdpMessage(const std::string& channel, const std::string& displayName, uint16_t messageId) {
    UdpMessage msg;
    msg.type = UdpMessageType::JOIN;
    msg.messageId = messageId;
    
    std::vector<uint8_t> payload;

    // Add channel ID (null-terminated)
    std::vector<uint8_t> channelBytes = packString(channel);
    payload.insert(payload.end(), channelBytes.begin(), channelBytes.end());

    // Add display name (null-terminated)
    std::vector<uint8_t> displayNameBytes = packString(displayName);
    payload.insert(payload.end(), displayNameBytes.begin(), displayNameBytes.end());
    
    msg.payload = payload;
    return msg;
}

// Builds a MSG message in UDP format containing display name and message content.
UdpMessage buildMsgUdpMessage(const std::string& displayName, const std::string& messageContent, uint16_t messageId) {
    UdpMessage msg;
    msg.type = UdpMessageType::MSG;
    msg.messageId = messageId;
    
    std::vector<uint8_t> payload;

    // Add display name (null-terminated)
    std::vector<uint8_t> displayNameBytes = packString(displayName);
    payload.insert(payload.end(), displayNameBytes.begin(), displayNameBytes.end());

    // Add message content (null-terminated)
    std::vector<uint8_t> contentBytes = packString(messageContent);
    payload.insert(payload.end(), contentBytes.begin(), contentBytes.end());
    
    msg.payload = payload;
    return msg;
}

// Builds a CONFIRM message referencing a message ID that was received.
UdpMessage buildConfirmUdpMessage(uint16_t refMessageId) {
    UdpMessage msg;
    msg.type = UdpMessageType::CONFIRM;
    msg.messageId = 0;  // CONFIRM message doesn't require its own ID

    // Convert refMessageId to network byte order
    uint16_t netRefId = htons(refMessageId);
    msg.payload.resize(2);
    std::memcpy(msg.payload.data(), &netRefId, 2);

    return msg;
}

// Builds a REPLY message including success/failure result and the original message ID.
UdpMessage buildReplyUdpMessage(const std::string& messageContent, uint16_t messageId, uint16_t refMessageId, uint8_t result) {
    UdpMessage msg;
    msg.type = UdpMessageType::REPLY;
    msg.messageId = messageId;
    
    std::vector<uint8_t> payload;

    // Add result byte: 1 for success, 0 for failure
    payload.push_back(result);
    
    // Add reference message ID in network byte order
    uint16_t netRefMessageId = htons(refMessageId);
    uint8_t* refIdBytes = reinterpret_cast<uint8_t*>(&netRefMessageId);
    payload.push_back(refIdBytes[0]);
    payload.push_back(refIdBytes[1]);
    
    // Add message content (null-terminated)
    std::vector<uint8_t> messageBytes = packString(messageContent);
    payload.insert(payload.end(), messageBytes.begin(), messageBytes.end());
    
    msg.payload = payload;
    return msg;
}
