#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <string>

class ChatClient {
public:
    virtual ~ChatClient() {}
    virtual bool connectToServer() = 0;
    virtual void run() = 0;
    virtual void sendByeMessage() = 0;
};

#endif // CHATCLIENT_H