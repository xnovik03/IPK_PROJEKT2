#ifndef UDPCHATCLIENT_H
#define UDPCHATCLIENT_H

#include "MessageUdp.h"
#include <string>
#include <netinet/in.h>

class UdpChatClient {
public:
    UdpChatClient(const std::string& server, int port);
    ~UdpChatClient();

    bool connectToServer();

    void run();

    void printHelp();

private:
    std::string serverAddress;
    int serverPort;
    int sockfd;
    void processReplyMessage(const UdpMessage& replyMsg);
    struct sockaddr_in serverAddr;
    uint16_t nextMessageId;
    std::string displayName;
    // Pomocné metody
    bool bindSocket();
    bool resolveServerAddr();
};

#endif // UDPCHATCLIENT_H
