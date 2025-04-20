#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <string>

// Abstract base class representing a generic chat client.
// This allows using either TCP or UDP clients with a common interface.
class ChatClient {
public:
    virtual ~ChatClient() {}

    // Establish connection to the server.
    virtual bool connectToServer() = 0;

    // Start the main loop for user interaction and message handling.
    virtual void run() = 0;

    // Send a BYE message before exiting.
    virtual void sendByeMessage() = 0;
};

#endif // CHATCLIENT_H
