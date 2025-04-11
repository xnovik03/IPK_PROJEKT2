#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

class Message {
public:
    enum class Type { AUTH, JOIN, MSG, REPLY, ERR, BYE };

    Message(Type type, const std::string& content);
    static Message createAuthMessage(const std::string& username, const std::string& displayName, const std::string& secret);
    static Message createJoinMessage(const std::string& channel, const std::string& displayName);
    static Message createByeMessage(const std::string& displayName);
    static Message createErrorMessage(const std::string& displayName, const std::string& errorMessage);
    static Message createReplyMessage(const std::string& content, bool success);
    static Message fromBuffer(const std::string& buffer);
    
    void sendMessage(int sockfd) const;
    std::string getContent() const;


private:
    Type type;
    std::string content;
};

#endif
