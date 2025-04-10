#pragma once
#include <string>

class TcpChatClient {
public:
    TcpChatClient(const std::string& host, int port);
    ~TcpChatClient();

    bool connectToServer();
    void run();
    void printHelp();

private:
    std::string server;
    std::string displayName; 
    int port;
    int sockfd;

    void receiveServerResponse(); 
};
