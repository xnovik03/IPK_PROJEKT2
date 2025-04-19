#ifndef UDPCHATCLIENT_H
#define UDPCHATCLIENT_H

#include "MessageUdp.h"
#include "ChatClient.h"  
#include <string>
#include <netinet/in.h>
#include <unordered_set>

class UdpChatClient : public ChatClient {
public:
    UdpChatClient(const std::string& server, int port);
    ~UdpChatClient();

    bool connectToServer();
    void run();

    // Funkce pro příkazy
    void printHelp(); 
    void handleCommand(const std::string& input);
    void handleAuthCommand(const std::string& input);
    void handleJoinCommand(const std::string& input);
    void handleRenameCommand(const std::string& input);
    void sendMessage(const std::string& message);

    void sendByeMessage();
  
private:
    std::string serverAddress;
    int serverPort;
    int sockfd;
    void processReplyMessage(const UdpMessage& replyMsg);
    void processErrMessage(const UdpMessage& errMsg);
    void receiveServerResponseUDP();
    void processConfirmMessage(const UdpMessage& confirmMsg); 
    void processMsgMessage(const UdpMessage& msgMsg);
    struct sockaddr_in serverAddr;
    uint16_t nextMessageId;
    std::string displayName;
    std::unordered_set<uint16_t> confirmedMessageIds;

    // Pomocné metody
    bool bindSocket();
    bool resolveServerAddr();
};

#endif // UDPCHATCLIENT_H
