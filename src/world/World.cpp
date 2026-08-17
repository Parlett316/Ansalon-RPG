#include "world/World.h"

namespace world {

void World::addLocation(Location location) {
    idIndex_[location.id] = locations_.size();
    locations_.push_back(std::move(location));
}

const Location* World::getLocation(const std::string& id) const {
    auto it = idIndex_.find(id);
    if (it == idIndex_.end()) return nullptr;
    return &locations_[it->second];
}

const Location* World::locationAt(int x, int y) const {
    for (const auto& loc : locations_) {
        if (loc.x == x && loc.y == y) return &loc;
    }
    return nullptr;
}

} // namespace world
