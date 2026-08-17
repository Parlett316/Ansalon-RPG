#pragma once

#include <string>

namespace world {

class World;

// Parses the plain-text location/road data format (grammar documented in
// docs/MAP_NOTES.md) and populates outWorld.
class WorldLoader {
public:
    // Throws std::runtime_error with a "file:line: message" description on
    // any malformed line or inconsistent data (e.g. a CONNECT that points at
    // a location id which was never defined). We validate everything at
    // load time, once, so a typo in the hand-edited data file fails fast
    // with a clear message at startup instead of surfacing later as a
    // confusing crash deep in gameplay -- see docs/GOTCHAS.md.
    static void loadFromFile(const std::string& path, World& outWorld);
};

} // namespace world
