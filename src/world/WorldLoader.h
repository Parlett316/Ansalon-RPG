#pragma once

#include <string>

namespace world {

class World;

// Parses the plain-text location data format (grammar documented in
// docs/MAP_NOTES.md) and populates outWorld. Note this loader knows nothing
// about OverworldGrid -- deliberately decoupled (see docs/ARCHITECTURE.md)
// -- so it does not validate that a location's POS falls inside the grid;
// that check happens in main.cpp, the one place both are loaded together.
class WorldLoader {
public:
    // Throws std::runtime_error with a "file:line: message" description on
    // any malformed line, so a typo in the hand-edited data file fails fast
    // with a clear message at startup instead of surfacing later as a
    // confusing bug deep in gameplay -- see docs/GOTCHAS.md.
    static void loadFromFile(const std::string& path, World& outWorld);
};

} // namespace world
