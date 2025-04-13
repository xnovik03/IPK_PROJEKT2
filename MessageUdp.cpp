#include "MessageUdp.h"
#include <arpa/inet.h>
#include <cstring>

// Sestaví (pack) UDP zprávu do binárního bufferu.
std::vector<uint8_t> packUdpMessage(const UdpMessage& msg) {
    std::vector<uint8_t> buffer;
    
    // Přidej 1 bajt: typ zprávy
    buffer.push_back(static_cast<uint8_t>(msg.type));
    
    // Připrav MessageID ve formátu network byte order
    uint16_t netMessageId = htons(msg.messageId);
    uint8_t* idBytes = reinterpret_cast<uint8_t*>(&netMessageId);
    buffer.push_back(idBytes[0]);
    buffer.push_back(idBytes[1]);
    
    // Přidej payload (pokud existuje)
    buffer.insert(buffer.end(), msg.payload.begin(), msg.payload.end());
    
    return buffer;
}

// Rozbalí (unpack) binární buffer zpět do struktury UdpMessage.
bool unpackUdpMessage(const std::vector<uint8_t>& buffer, UdpMessage& msg) {
    if (buffer.size() < 3) {
        return false;
    }
    
    // První bajt je typ zprávy
    msg.type = static_cast<UdpMessageType>(buffer[0]);
    
    // Následující 2 bajty jsou MessageID 
    uint16_t netMessageId;
    std::memcpy(&netMessageId, &buffer[1], sizeof(uint16_t));
    msg.messageId = ntohs(netMessageId);
    
    // Zbytek bufferu je payload
    msg.payload = std::vector<uint8_t>(buffer.begin() + 3, buffer.end());
    
    return true;
}
