#ifndef UDPCHATCLIENT_H
#define UDPCHATCLIENT_H

#include "MessageUdp.h"
#include "ChatClient.h"  
#include <string>
#include <netinet/in.h>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <unordered_map>
struct SentMessageInfo {
    std::vector<uint8_t> data;
    uint16_t messageId;
    std::chrono::steady_clock::time_point timestamp;
};

std::unordered_map<uint16_t, SentMessageInfo> sentMessages;

// Class to handle UDP chat client functionalities.

class UdpChatClient : public ChatClient {
public:
    UdpChatClient(const std::string& server, int port);
    ~UdpChatClient();

    bool connectToServer();
    void run();

    // Functions for handling commands    
    void printHelp(); 
    void handleCommand(const std::string& input);
    void handleAuthCommand(const std::string& input);
    void handleJoinCommand(const std::string& input);
    void handleRenameCommand(const std::string& input);
    void sendMessage(const std::string& message);
    void processByeMessage(const UdpMessage& byeMsg);
     
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
    void sendPingMessage();
    void processPingMessage(const UdpMessage& pingMsg);
    void checkRetransmissions();
    void sendRawUdpMessage(const UdpMessage& msg); 
    struct sockaddr_in serverAddr;
    uint16_t nextMessageId;
    std::string displayName;
    std::unordered_set<uint16_t> confirmedMessageIds;
    std::thread receiverThread;
    std::atomic<bool> running = true;
    std::thread retransmissionThread;
    std::unordered_set<uint16_t> receivedMsgIds; 
    void backgroundReceiverLoop();
    // Helper methods
    bool bindSocket();
    bool resolveServerAddr();
};

#endif // UDPCHATCLIENT_H
