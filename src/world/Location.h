#pragma once

#include <string>
#include <vector>

namespace world {

// A single road leading from one Location to another. Roads are declared on
// both ends in data/locations.txt (see docs/MAP_NOTES.md) -- the format does
// not infer a reverse connection automatically, so a one-way road is
// possible simply by omitting the CONNECT line on the far end.
struct Connection {
    std::string targetId;
    int travelDays = 0;
    std::string roadDescription;
};

// A single named place on the map. Plain data -- all behavior (loading,
// rendering, travel) lives in WorldLoader/World/MapRenderer/GameLoop so this
// struct stays easy to extend (e.g. adding shops, NPCs) without touching
// unrelated code.
struct Location {
    std::string id;           // stable key, e.g. "solace" -- used in CONNECT and GameState
    std::string name;         // display name, e.g. "Solace"
    std::string region;       // e.g. "Abanasinia" -- unused by logic today, reserved for the
                               // future timeline engine to scope canon events by region
    std::string terrain;      // free-form tag, e.g. "forest-town"
    char glyph = '?';         // single ASCII character drawn on the schematic map
    int row = 0;               // schematic (stylized, not geographic) grid position
    int col = 0;
    std::string description;
    std::vector<Connection> connections;
};

} // namespace world
