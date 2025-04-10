
#include "InputHandler.h"
#include <sstream>

std::optional<AuthCommand> InputHandler::parseAuthCommand(const std::string& input) {
    std::istringstream iss(input);
    std::string cmd, username, secret, displayName;
    iss >> cmd >> username >> secret >> displayName;

    if (cmd != "/auth" || username.empty() || secret.empty() || displayName.empty()) {
        return std::nullopt;
    }

    return AuthCommand{username, secret, displayName};
}
std::optional<std::string> InputHandler::parseJoinCommand(const std::string& line) {
    std::istringstream iss(line);
    std::string cmd, channel;
    
    iss >> cmd; // Přečte "/join"
    
    if (cmd == "/join") {
        iss >> channel;  // Načte název kanálu
        
        if (!channel.empty()) {
            return channel;  // Pokud je název kanálu validní, vrátíme ho jako std::optional
        }
    }
    
    return std::nullopt;  // Pokud je formát špatný, vrátíme std::nullopt
}


