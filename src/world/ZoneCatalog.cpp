#include "world/ZoneCatalog.h"
#include "world/World.h"
#include "world/ZoneLoader.h"

#include <filesystem>

namespace world {

ZoneCatalog ZoneCatalog::loadForWorld(const World& world, const std::string& zonesDir) {
    ZoneCatalog catalog;
    for (const auto& loc : world.allLocations()) {
        std::filesystem::path candidate = std::filesystem::path(zonesDir) / (loc.id + ".txt");
        if (!std::filesystem::exists(candidate)) continue;
        catalog.zones_.emplace(loc.id, ZoneLoader::loadFromFile(candidate.string()));
    }
    return catalog;
}

bool ZoneCatalog::hasZone(const std::string& locationId) const {
    return zones_.count(locationId) > 0;
}

const Zone* ZoneCatalog::getZone(const std::string& locationId) const {
    auto it = zones_.find(locationId);
    if (it == zones_.end()) return nullptr;
    return &it->second;
}

} // namespace world
