#include "UdpCommandBuilder.h"
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <iterator>

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
    
    // Vytvoříme payload složený z jednotlivých polí ukončených nulou.
    std::vector<uint8_t> payload;
    
    // Přidání username
    std::vector<uint8_t> usernameBytes = packString(cmd.username);
    payload.insert(payload.end(), usernameBytes.begin(), usernameBytes.end());
    
    // Přidání display name
    std::vector<uint8_t> displayNameBytes = packString(cmd.displayName);
    payload.insert(payload.end(), displayNameBytes.begin(), displayNameBytes.end());
    
    // Přidání secret
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
