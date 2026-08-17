#pragma once

#include <string>

namespace world {

class Zone;

// Parses one data/zones/<id>.txt file (grammar in docs/ZONE_NOTES.md) into
// a Zone. Throws std::runtime_error with a "file:line: message" description
// on any malformed line, a ragged GRID block, or a GRID character that's
// neither a known world::ZoneTile code nor a declared POI -- fails fast at
// load time rather than crashing (or silently misrendering) during play.
class ZoneLoader {
public:
    static Zone loadFromFile(const std::string& path);
};

} // namespace world
