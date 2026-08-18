#include "world/ZoneCatalog.h"
#include "world/World.h"
#include "world/ZoneLoader.h"

#include <filesystem>
#include <stdexcept>
#include <vector>

namespace world {

ZoneCatalog ZoneCatalog::loadForWorld(const World& world, const std::string& zonesDir) {
    ZoneCatalog catalog;
    std::vector<std::string> toFollow;
    for (const auto& loc : world.allLocations()) {
        std::filesystem::path candidate = std::filesystem::path(zonesDir) / (loc.id + ".txt");
        if (!std::filesystem::exists(candidate)) continue;
        catalog.zones_.emplace(loc.id, ZoneLoader::loadFromFile(candidate.string()));
        toFollow.push_back(loc.id);
    }
    // A zone reached only via another zone's PORTAL (e.g. the Inn of the
    // Last Home, entered from inside Solace's town square) has no matching
    // overworld Location of its own, so the loop above wouldn't find it --
    // follow PORTAL links (transitively) to pick those up too.
    for (size_t i = 0; i < toFollow.size(); ++i) {
        for (const auto& [code, targetId] : catalog.zones_.at(toFollow[i]).portals()) {
            if (catalog.zones_.count(targetId) > 0) continue;
            std::filesystem::path candidate = std::filesystem::path(zonesDir) / (targetId + ".txt");
            if (!std::filesystem::exists(candidate)) {
                throw std::runtime_error("zone '" + toFollow[i] + "' has a PORTAL '" +
                                          std::string(1, code) + "' to '" + targetId + "' but " +
                                          candidate.string() + " does not exist");
            }
            catalog.zones_.emplace(targetId, ZoneLoader::loadFromFile(candidate.string()));
            toFollow.push_back(targetId);
        }
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
