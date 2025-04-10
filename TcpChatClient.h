#ifndef TCP_CHAT_CLIENT_H
#define TCP_CHAT_CLIENT_H

#include <string>

class TcpChatClient {
public:
    TcpChatClient(const std::string& server, int port);
    bool connectToServer();

private:
    std::string server;
    int port;
    int sockfd;
};

#endif
