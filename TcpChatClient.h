#pragma once
#include <string>
#include "Message.h"

class TcpChatClient {
public:
    TcpChatClient(const std::string& host, int port);
    ~TcpChatClient();

    bool connectToServer();
    void run();
    void printHelp();
    void sendByeMessage();
    void process_reply(const Message& reply);

private:
    std::string server;
    std::string displayName; 
    int port;
    int sockfd;

    void receiveServerResponse();
    Message parseMessage(const std::string& buffer); 
};
