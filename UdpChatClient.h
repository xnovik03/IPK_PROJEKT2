#ifndef UDPCHATCLIENT_H
#define UDPCHATCLIENT_H

#include "MessageUdp.h"
#include <string>
#include <netinet/in.h>
#include <unordered_set>
   
class UdpChatClient {
public:
    UdpChatClient(const std::string& server, int port);
    ~UdpChatClient();

    bool connectToServer();

    void run();

    void printHelp();

    void sendByeMessage();

private:
    std::string serverAddress;
    int serverPort;
    int sockfd;
    void processReplyMessage(const UdpMessage& replyMsg);
    void processErrMessage(const UdpMessage& errMsg);
    void receiveServerResponseUDP();
    struct sockaddr_in serverAddr;
    uint16_t nextMessageId;
    std::string displayName;
    std::unordered_set<uint16_t> confirmedMessageIds;
    // Pomocné metody
    bool bindSocket();
    bool resolveServerAddr();
};

#endif // UDPCHATCLIENT_H
