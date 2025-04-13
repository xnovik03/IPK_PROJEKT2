#ifndef UDPCOMMANDBUILDER_H
#define UDPCOMMANDBUILDER_H

#include "MessageUdp.h"      
#include "InputHandler.h"   
#include <vector>
#include <string>


std::vector<uint8_t> packString(const std::string& s);

UdpMessage buildAuthUdpMessage(const AuthCommand& cmd, uint16_t messageId);

UdpMessage buildJoinUdpMessage(const std::string& channel, const std::string& displayName, uint16_t messageId);

#endif // UDPCOMMANDBUILDER_H
