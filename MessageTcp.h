#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

// This class represents a message used in the chat protocol.
// It supports different message types and provides factory methods
// for constructing standard messages according to the protocol.
class Message {
public:
    // Enum for the supported message types
    enum Type { AUTH, JOIN, BYE, ERR, REPLY, MSG };

    // Constructor
    Message(Type type, const std::string& content);

    // Factory methods to create various message types
    static Message createAuthMessage(const std::string& username, const std::string& displayName, const std::string& secret);
    static Message createJoinMessage(const std::string& channel, const std::string& displayName);
    static Message createByeMessage(const std::string& displayName);
    static Message createErrorMessage(const std::string& displayName, const std::string& errorMessage);
    static Message createReplyMessage(const std::string& content, bool success);

    // Parses a raw buffer and creates a Message object
    static Message fromBuffer(const std::string& buffer);

    // Sends this message to the specified socket
    void sendMessage(int sockfd) const;

    // Returns the content of the message
    std::string getContent() const;

    // Returns the type of the message
    Type getType() const;

private:
    Type type;               // Type of the message (e.g., AUTH, JOIN, etc.)
    std::string content;     // The actual message string content
};

#endif // MESSAGE_H
