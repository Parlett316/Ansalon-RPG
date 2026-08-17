#pragma once

#include <string>

namespace game {

enum class Verb {
    Look,
    Map,
    Go,
    Help,
    Quit,
    Unknown,
};

struct ParsedCommand {
    Verb verb = Verb::Unknown;
    std::string argument; // e.g. the destination name for Go; unused otherwise
};

// Parses one raw line of player input into a verb + argument. Pure parsing
// -- no World/GameState access, no I/O -- which keeps it trivial to extend
// with new verbs later (talk, fight, inventory, ...) without touching
// GameLoop's execution logic, and trivial to unit test in isolation.
ParsedCommand parseCommand(const std::string& input);

} // namespace game
