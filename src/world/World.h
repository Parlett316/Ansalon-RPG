#pragma once

#include "world/Location.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace world {

// Owns every Location and answers graph-shaped questions about them (lookup
// by id, fuzzy lookup by name). Deliberately has no notion of "current"
// anything -- that belongs to game::GameState, not the world model, so
// World stays reusable by later systems (a timeline engine, encounter
// tables, ...) that need to query the map without touching player state.
class World {
public:
    void addLocation(Location location);

    // Exact lookup by stable id (e.g. "solace"). Returns nullptr if unknown.
    const Location* getLocation(const std::string& id) const;

    // Case-insensitive lookup by display name, so players can type
    // "go tarsis" without exact capitalization. Tries an exact name match
    // first, then falls back to "name starts with" so partial input like
    // "go pax" still resolves to "Pax Tharkas". Returns nullptr if nothing
    // matches.
    const Location* findByName(const std::string& name) const;

    const std::vector<Location>& allLocations() const { return locations_; }

private:
    std::vector<Location> locations_;
    std::unordered_map<std::string, size_t> idIndex_;
};

} // namespace world
