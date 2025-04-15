#include "UdpCommandBuilder.h"
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <iterator>
#include <arpa/inet.h>
#include <cstring>

// Převod řetězce na vector<uint8_t> se 0 na konci.
std::vector<uint8_t> packString(const std::string& s) {
    std::vector<uint8_t> result(s.begin(), s.end());
    result.push_back(0); 
    return result;
}

UdpMessage buildAuthUdpMessage(const AuthCommand& cmd, uint16_t messageId) {
    UdpMessage msg;
    msg.type = UdpMessageType::AUTH;
    msg.messageId = messageId;
    
    std::vector<uint8_t> payload;
    
    
    std::vector<uint8_t> usernameBytes = packString(cmd.username);
    payload.insert(payload.end(), usernameBytes.begin(), usernameBytes.end());
    
   
    std::vector<uint8_t> displayNameBytes = packString(cmd.displayName);
    payload.insert(payload.end(), displayNameBytes.begin(), displayNameBytes.end());

    std::vector<uint8_t> secretBytes = packString(cmd.secret);
    payload.insert(payload.end(), secretBytes.begin(), secretBytes.end());
    
    msg.payload = payload;
    return msg;
}

UdpMessage buildJoinUdpMessage(const std::string& channel, const std::string& displayName, uint16_t messageId) {
    UdpMessage msg;
    msg.type = UdpMessageType::JOIN;
    msg.messageId = messageId;
    
    std::vector<uint8_t> payload;
    
    // Přidáme channel ID a display name, každý ukončený nulou.
    std::vector<uint8_t> channelBytes = packString(channel);
    payload.insert(payload.end(), channelBytes.begin(), channelBytes.end());
    
    std::vector<uint8_t> displayNameBytes = packString(displayName);
    payload.insert(payload.end(), displayNameBytes.begin(), displayNameBytes.end());
    
    msg.payload = payload;
    return msg;
}
UdpMessage buildMsgUdpMessage(const std::string& displayName, const std::string& messageContent, uint16_t messageId) {
    UdpMessage msg;
    msg.type = UdpMessageType::MSG;
    msg.messageId = messageId;
    
    std::vector<uint8_t> payload;
    
 
    std::vector<uint8_t> displayNameBytes = packString(displayName);
    payload.insert(payload.end(), displayNameBytes.begin(), displayNameBytes.end());
    
 
    std::vector<uint8_t> contentBytes = packString(messageContent);
    payload.insert(payload.end(), contentBytes.begin(), contentBytes.end());
    
    msg.payload = payload;
    return msg;
}
UdpMessage buildConfirmUdpMessage(uint16_t refMessageId) {
    UdpMessage msg;
    msg.type = UdpMessageType::CONFIRM;  
    msg.messageId = 0;
    uint16_t netRefId = htons(refMessageId);
    msg.payload.resize(sizeof(uint16_t));
    std::memcpy(msg.payload.data(), &netRefId, sizeof(uint16_t));
    
    return msg;
}