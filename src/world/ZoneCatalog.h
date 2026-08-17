#pragma once

#include "world/Zone.h"

#include <string>
#include <unordered_map>

namespace world {

class World;

// Loads every walkable interior available in the world. For each Location
// in the given World, checks whether "<zonesDir>/<id>.txt" exists and loads
// it if so -- the filename stem IS the zone id, matching the LOCATION id
// exactly, so "does this place have a walkable interior" is answered by a
// simple existence check rather than a second hand-maintained list (see
// docs/ARCHITECTURE.md).
class ZoneCatalog {
public:
    static ZoneCatalog loadForWorld(const World& world, const std::string& zonesDir);

    bool hasZone(const std::string& locationId) const;
    const Zone* getZone(const std::string& locationId) const;

private:
    std::unordered_map<std::string, Zone> zones_;
};

} // namespace world
