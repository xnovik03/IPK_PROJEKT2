#include "Message.h"
#include <sys/socket.h>  
#include <iostream>

Message::Message(Type type, const std::string& content) : type(type), content(content) {}

Message Message::createAuthMessage(const std::string& username, const std::string& displayName, const std::string& secret) {
    return Message(Type::AUTH, "AUTH " + username + " AS " + displayName + " USING " + secret);
}

Message Message::createJoinMessage(const std::string& channel, const std::string& displayName) {
    return Message(Type::JOIN, "JOIN " + channel + " AS " + displayName);
}

Message Message::createByeMessage(const std::string& displayName) {
    return Message(Type::BYE, "BYE FROM " + displayName);
}

Message Message::createErrorMessage(const std::string& displayName, const std::string& errorMessage) {
    return Message(Type::ERR, "ERR FROM " + displayName + " IS " + errorMessage);
}

Message Message::createReplyMessage(const std::string& content, bool success) {
    std::string status = success ? "OK" : "NOK";
    return Message(Type::REPLY, "REPLY " + status + " IS " + content);
}

Message Message::fromBuffer(const std::string& buffer) {
    return Message(Type::MSG, buffer);
}

void Message::sendMessage(int sockfd) const {
   std::string messageWithEnd = content + "\r\n";  
    if (send(sockfd, messageWithEnd.c_str(), messageWithEnd.size(), 0) == -1) {
        std::perror("ERROR: send failed");
    }
}


std::string Message::getContent() const {
    return content;
}
