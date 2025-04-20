#ifndef UDPCOMMANDBUILDER_H
#define UDPCOMMANDBUILDER_H

#include "MessageUdp.h"      
#include "InputHandler.h"   
#include <vector>
#include <string>

// Function to pack a string into a vector of uint8_t, adding a null-terminator at the end
std::vector<uint8_t> packString(const std::string& s);
// Functions to build various types of UDP messages
UdpMessage buildAuthUdpMessage(const AuthCommand& cmd, uint16_t messageId);

UdpMessage buildJoinUdpMessage(const std::string& channel, const std::string& displayName, uint16_t messageId);

UdpMessage buildMsgUdpMessage(const std::string& displayName, const std::string& messageContent, uint16_t messageId);

UdpMessage buildConfirmUdpMessage(uint16_t refMessageId);

UdpMessage buildReplyUdpMessage(const std::string& messageContent, uint16_t messageId, uint16_t refMessageId, uint8_t result);

#endif // UDPCOMMANDBUILDER_H
