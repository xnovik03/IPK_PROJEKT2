#pragma once
#include <string>
#include "MessageTcp.h"
#include "ChatClient.h"

#define DEFAULT_PORT 4567
// This class represents a TCP chat client that connects to a server and sends/receives messages.
class TcpChatClient : public ChatClient {
public:
    TcpChatClient(const std::string& host, int port);
    ~TcpChatClient();

    bool connectToServer();
    void run();
    void printHelp();
    void sendByeMessage();
    void process_reply(const Message& reply);
    void sendChannelJoinConfirmation();
   void processInvalidMessage(const std::string& invalidMessage);
private:
    std::string server;
    std::string displayName; 
    int port;
    int sockfd;
    bool authenticated; 
    void receiveServerResponse();
    Message parseMessage(const std::string& buffer); 
};
