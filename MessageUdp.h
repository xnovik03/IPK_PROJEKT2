#ifndef MESSAGEUDP_H
#define MESSAGEUDP_H

#include <cstdint>
#include <vector>
#include <string>

// Výčtový typ pro UDP zprávy dle specifikace
enum class UdpMessageType : uint8_t {
    CONFIRM = 0x00,  
    REPLY   = 0x01,  
    AUTH    = 0x02,  
    JOIN    = 0x03,  
    MSG     = 0x04,  
    PING    = 0xFD,  
    ERR     = 0xFE,  
    BYE     = 0xFF   
};

// Struktura reprezentující UDP zprávu
struct UdpMessage {
    UdpMessageType type;       // Typ zprávy (1 bajt)
    uint16_t messageId;        //  MessageID (2 bajty)
    std::vector<uint8_t> payload; 
};

// Funkce na sestavení (pack) zprávy do binárního bufferu pro odeslání přes UDP
std::vector<uint8_t> packUdpMessage(const UdpMessage& msg);

// Funkce pro rozbalení (unpack) přijatého bufferu do struktury UdpMessage
bool unpackUdpMessage(const std::vector<uint8_t>& buffer, UdpMessage& msg);

#endif // MESSAGEUDP_H
