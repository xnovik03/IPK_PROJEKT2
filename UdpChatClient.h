#ifndef UDPCHATCLIENT_H
#define UDPCHATCLIENT_H

#include <string>
#include <netinet/in.h>

class UdpChatClient {
public:
    UdpChatClient(const std::string& server, int port);
    ~UdpChatClient();

    bool connectToServer();

    void run();

private:
    std::string serverAddress;
    int serverPort;
    int sockfd;
    struct sockaddr_in serverAddr;
    uint16_t nextMessageId;
    // Pomocné metody
    bool bindSocket();
    bool resolveServerAddr();
};

#endif // UDPCHATCLIENT_H
