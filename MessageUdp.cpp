#include "MessageUdp.h"
#include <arpa/inet.h>
#include <cstring>

std::vector<uint8_t> packUdpMessage(const UdpMessage& msg) {
    std::vector<uint8_t> buffer;

    buffer.push_back(static_cast<uint8_t>(msg.type));

    if (msg.type != UdpMessageType::CONFIRM) {
        
        uint16_t netMsgId = htons(msg.messageId);
        buffer.push_back(reinterpret_cast<uint8_t*>(&netMsgId)[0]);
        buffer.push_back(reinterpret_cast<uint8_t*>(&netMsgId)[1]);
    }

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
