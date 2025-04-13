#include "MessageTcp.h"
#include <iostream>
#include <sys/socket.h>  // For send()
#include <sstream>

Message::Message(Type type, const std::string& content) : type(type), content(content) {}

Message Message::createAuthMessage(const std::string& username, const std::string& displayName, const std::string& secret) {
    return Message(Type::AUTH, "AUTH " + username + " AS " + displayName + " USING " + secret);
}

Message Message::createJoinMessage(const std::string& channel, const std::string& displayName) {
    return Message(Type::JOIN, "JOIN " + channel + " AS " + displayName);
}

Message Message::createByeMessage(const std::string& displayName) {
    return Message(Message::Type::BYE, "BYE FROM " + displayName);
}

Message Message::createErrorMessage(const std::string& displayName, const std::string& errorMessage) {
    return Message(Type::ERR, "ERR FROM " + displayName + " IS " + errorMessage);
}

Message Message::createReplyMessage(const std::string& content, bool success) {
    std::string status = success ? "OK" : "NOK";
    return Message(Type::REPLY, "REPLY " + status + " IS " + content);
}

// Pomocná funkce pro odstranění "\r\n" na konci řetězce
static std::string removeCRLF(const std::string& s) {
    std::string result = s;
    if (result.size() >= 2 && result.substr(result.size()-2) == "\r\n") {
        result.erase(result.size()-2);
    }
    return result;
}

Message Message::fromBuffer(const std::string& rawBuffer)
{
    std::string buffer = removeCRLF(rawBuffer);
    std::istringstream iss(buffer);
    std::string firstWord;
    iss >> firstWord;  // např. AUTH, JOIN, REPLY, BYE, MSG, ERR

    if (firstWord == "REPLY") {
        std::string status; // OK nebo NOK
        iss >> status;
        std::string isLiteral;
        iss >> isLiteral;  // očekává se "IS"

        // Zbytek řádku je text zprávy
        std::string replyContent;
        std::getline(iss, replyContent);
        if (!replyContent.empty() && replyContent.front() == ' ')
            replyContent.erase(replyContent.begin());

        // Spojíme status a text zprávy do jediného řetězce
        return Message(Message::Type::REPLY, status + " " + replyContent);
    }
    else if (firstWord == "BYE") {
        // Očekává se formát: "BYE FROM <DisplayName>"
        std::string fromLiteral;
        iss >> fromLiteral; // "FROM"
        std::string dname;
        std::getline(iss, dname);
        if (!dname.empty() && dname.front() == ' ')
            dname.erase(dname.begin());
        return Message(Message::Type::BYE, dname);
    }
    else if (firstWord == "ERR") {
      
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
        // V případě neznámého typu nastavíme typ jako MSG
        return Message(Message::Type::MSG, buffer);
    }
}
void Message::sendMessage(int sockfd) const {
    std::string messageWithEnd = content + "\r\n";  
    std::cout << "Sending message: " << messageWithEnd << std::endl;  // Ladicí výpis
    if (send(sockfd, messageWithEnd.c_str(), messageWithEnd.size(), 0) == -1) {
        std::perror("ERROR: send failed");
    }
}

std::string Message::getContent() const {
    return content;
}

Message::Type Message::getType() const {
    return type;
}
