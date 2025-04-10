#pragma once
#include <string>
#include <optional>
#include <tuple>

struct AuthCommand {
    std::string username;
    std::string secret;
    std::string displayName;
};

class InputHandler {
public:
    static std::optional<AuthCommand> parseAuthCommand(const std::string& line);
};
