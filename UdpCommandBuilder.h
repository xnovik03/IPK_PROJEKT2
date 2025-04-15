#ifndef UDPCOMMANDBUILDER_H
#define UDPCOMMANDBUILDER_H

#include "MessageUdp.h"      
#include "InputHandler.h"   
#include <vector>
#include <string>


std::vector<uint8_t> packString(const std::string& s);

UdpMessage buildAuthUdpMessage(const AuthCommand& cmd, uint16_t messageId);

UdpMessage buildJoinUdpMessage(const std::string& channel, const std::string& displayName, uint16_t messageId);

UdpMessage buildMsgUdpMessage(const std::string& displayName, const std::string& messageContent, uint16_t messageId);

UdpMessage buildConfirmUdpMessage(uint16_t refMessageId);
#endif // UDPCOMMANDBUILDER_H
