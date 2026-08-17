#include "world/World.h"

#include <algorithm>
#include <cctype>

namespace world {

namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

} // namespace

void World::addLocation(Location location) {
    idIndex_[location.id] = locations_.size();
    locations_.push_back(std::move(location));
}

const Location* World::getLocation(const std::string& id) const {
    auto it = idIndex_.find(id);
    if (it == idIndex_.end()) return nullptr;
    return &locations_[it->second];
}

const Location* World::findByName(const std::string& name) const {
    const std::string needle = toLower(name);
    if (needle.empty()) return nullptr;

    for (const auto& loc : locations_) {
        if (toLower(loc.name) == needle) return &loc;
    }
    for (const auto& loc : locations_) {
        const std::string locName = toLower(loc.name);
        if (locName.rfind(needle, 0) == 0) return &loc; // starts-with
    }
    return nullptr;
}

} // namespace world
