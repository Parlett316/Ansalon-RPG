#pragma once

#include "world/Location.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace world {

// Owns every Location and answers questions about them (lookup by id,
// lookup by tile position). Deliberately has no notion of "current"
// anything -- that belongs to game::GameState, not the world model, so
// World stays reusable by later systems (a timeline engine, encounter
// tables, ...) that need to query the map without touching player state.
class World {
public:
    void addLocation(Location location);

    // Exact lookup by stable id (e.g. "solace"). Returns nullptr if unknown.
    const Location* getLocation(const std::string& id) const;

    // Returns the location exactly at (x, y), or nullptr if none is there.
    // Used by rendering and by GameLoop to detect arrival. A linear scan is
    // fine at the current location count; revisit with a coordinate index
    // if that grows into the hundreds.
    const Location* locationAt(int x, int y) const;

    const std::vector<Location>& allLocations() const { return locations_; }

private:
    std::vector<Location> locations_;
    std::unordered_map<std::string, size_t> idIndex_;
};

} // namespace world
