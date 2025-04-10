#pragma once
#include <string>
#include <optional>
#include <tuple>

struct AuthCommand {
    std::string username;
    std::string secret;
    std::string displayName;
};
struct JoinCommand {
    std::string channel;
};
class InputHandler {
public:
    static std::optional<AuthCommand> parseAuthCommand(const std::string& line);
    static std::optional<std::string> parseJoinCommand(const std::string& input);};
