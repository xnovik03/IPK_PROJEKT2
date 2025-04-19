#pragma once
#include <string>
#include "MessageTcp.h"
#include "ChatClient.h"

class TcpChatClient : public ChatClient {
public:
    TcpChatClient(const std::string& host, int port);
    ~TcpChatClient();

    bool connectToServer();
    void run();
    void printHelp();
    void sendByeMessage();
    void process_reply(const Message& reply);

   void processInvalidMessage(const std::string& invalidMessage);
private:
    std::string server;
    std::string displayName; 
    int port;
    int sockfd;

    void receiveServerResponse();
    Message parseMessage(const std::string& buffer); 
};
