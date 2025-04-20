#pragma once
#include <string>
#include <optional>
#include <tuple>

// Structure to store the authentication command details
struct AuthCommand {
    std::string username;    // Username for authentication
    std::string secret;      // Secret (password) for authentication
    std::string displayName; // Display name to be used after authentication
};

// Structure to store the join command details
struct JoinCommand {
    std::string channel; // Channel to join
};

class InputHandler {
public:
    // Parses the "/join" command from the input string and returns the channel if valid
    // If the input format is invalid, returns an empty optional
    static std::optional<std::string> parseJoinCommand(const std::string& input);

    // Parses the "/auth" command from the input string and returns an AuthCommand object if valid
    // If the input format is invalid, returns an empty optional
    static std::optional<AuthCommand> parseAuthCommand(const std::string& input);
};
