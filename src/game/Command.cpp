#include "game/Command.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace game {

namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

} // namespace

ParsedCommand parseCommand(const std::string& input) {
    std::istringstream iss(input);
    std::string verbWord;
    iss >> verbWord;
    verbWord = toLower(verbWord);

    std::string argument;
    std::getline(iss, argument);
    size_t start = argument.find_first_not_of(" \t");
    argument = (start == std::string::npos) ? "" : argument.substr(start);

    ParsedCommand cmd;
    cmd.argument = argument;

    if (verbWord == "go" || verbWord == "travel" || verbWord == "move") {
        cmd.verb = Verb::Go;
    } else if (verbWord == "look" || verbWord == "l") {
        cmd.verb = Verb::Look;
    } else if (verbWord == "map" || verbWord == "m") {
        cmd.verb = Verb::Map;
    } else if (verbWord == "help" || verbWord == "?") {
        cmd.verb = Verb::Help;
    } else if (verbWord == "quit" || verbWord == "exit" || verbWord == "q") {
        cmd.verb = Verb::Quit;
    } else {
        cmd.verb = Verb::Unknown;
    }
    return cmd;
}

} // namespace game
