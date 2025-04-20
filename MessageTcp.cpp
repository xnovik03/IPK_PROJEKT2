#include "MessageTcp.h"
#include <iostream>
#include <sys/socket.h>  // For send()
#include <sstream>

// Constructor: initialize message with type and content
Message::Message(Type type, const std::string& content) : type(type), content(content) {}

// Factory method to create an AUTH message
Message Message::createAuthMessage(const std::string& username, const std::string& displayName, const std::string& secret) {
    return Message(Type::AUTH, "AUTH " + username + " AS " + displayName + " USING " + secret);
}

// Factory method to create a JOIN message
Message Message::createJoinMessage(const std::string& channel, const std::string& displayName) {
    return Message(Type::JOIN, "JOIN " + channel + " AS " + displayName);
}

// Factory method to create a BYE message
Message Message::createByeMessage(const std::string& displayName) {
    return Message(Message::Type::BYE, "BYE FROM " + displayName);
}

// Factory method to create an ERR message
Message Message::createErrorMessage(const std::string& displayName, const std::string& errorMessage) {
    return Message(Type::ERR, "ERR FROM " + displayName + " IS " + errorMessage);
}

// Factory method to create a REPLY message
Message Message::createReplyMessage(const std::string& content, bool success) {
    std::string status = success ? "OK" : "NOK";
    return Message(Type::REPLY, "REPLY " + status + " IS " + content);
}

// Helper function to remove trailing "\r\n" from input
static std::string removeCRLF(const std::string& s) {
    std::string result = s;
    if (result.size() >= 2 && result.substr(result.size()-2) == "\r\n") {
        result.erase(result.size()-2);
    }
    return result;
}

// Parses a raw message string into a Message object with proper type
Message Message::fromBuffer(const std::string& rawBuffer)
{
    std::string buffer = removeCRLF(rawBuffer);
    std::istringstream iss(buffer);
    std::string firstWord;
    iss >> firstWord;  // e.g., AUTH, JOIN, REPLY, BYE, MSG, ERR

    if (firstWord == "REPLY") {
        // Parse format: REPLY OK|NOK IS <content>
        std::string status;
        iss >> status;
        std::string isLiteral;
        iss >> isLiteral;

        std::string replyContent;
        std::getline(iss, replyContent);
        if (!replyContent.empty() && replyContent.front() == ' ')
            replyContent.erase(replyContent.begin());

        return Message(Message::Type::REPLY, status + " " + replyContent);
    }
    else if (firstWord == "BYE") {
        // Parse format: BYE FROM <DisplayName>
        std::string fromLiteral;
        iss >> fromLiteral;
        std::string dname;
        std::getline(iss, dname);
        if (!dname.empty() && dname.front() == ' ')
            dname.erase(dname.begin());
        return Message(Message::Type::BYE, dname);
    }
    else if (firstWord == "ERR") {
        // ERR is stored in full, handled separately
        return Message(Message::Type::ERR, buffer);
    }
    else if (firstWord == "AUTH") {
        return Message(Message::Type::AUTH, buffer);
    }
    else if (firstWord == "JOIN") {
        return Message(Message::Type::JOIN, buffer);
    }
    else if (firstWord == "MSG") {
        return Message(Message::Type::MSG, buffer);
    }
    else {
        // Default to MSG type if unknown
        return Message(Message::Type::MSG, buffer);
    }
}

// Sends the message content to the server via socket
void Message::sendMessage(int sockfd) const {
    std::string messageWithEnd = content + "\r\n";  
    std::cerr << "Sending message: " << messageWithEnd << std::endl;  // Debug output
    if (send(sockfd, messageWithEnd.c_str(), messageWithEnd.size(), 0) == -1) {
        std::perror("ERROR: send failed");
    }
}

// Getter for the content string
std::string Message::getContent() const {
    return content;
}

// Getter for the message type
Message::Type Message::getType() const {
    return type;
}
