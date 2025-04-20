#include "InputHandler.h"
#include <sstream>
#include <optional>

// Parses the /auth command from user input.
// Expected format: /auth <username> <secret> <displayName>
std::optional<AuthCommand> InputHandler::parseAuthCommand(const std::string& input) {
    std::istringstream iss(input);
    std::string cmd, username, secret, displayName;

    // Read command and parameters from the input string
    iss >> cmd >> username >> secret >> displayName;

    // Check if input matches expected format
    if (cmd != "/auth" || username.empty() || secret.empty() || displayName.empty()) {
        return std::nullopt; // Return no result if format is invalid
    }

    // Return the parsed command as an AuthCommand object
    return AuthCommand{username, secret, displayName};
}

// Parses the /join command from user input.
// Expected format: /join <channel>
std::optional<std::string> InputHandler::parseJoinCommand(const std::string& line) {
    std::istringstream iss(line);
    std::string cmd, channel;

    // Read the command keyword (should be "/join")
    iss >> cmd;

    if (cmd == "/join") {
        // Try to read the channel name
        iss >> channel;

        // Return the channel name if it's not empty
        if (!channel.empty()) {
            return channel;
        }
    }

    return std::nullopt; // Return no result if command is invalid or empty
}
