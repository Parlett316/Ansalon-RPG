#include "world/Zone.h"

namespace world {

Zone::Zone(std::string name, std::vector<std::string> rows, int entryX, int entryY,
           std::unordered_map<char, PointOfInterest> pois,
           std::unordered_map<char, std::string> portals, char timelineAnchorPoi,
           std::string timelineLocationId, std::unordered_map<char, std::string> quests,
           std::unordered_map<char, BoatVoyage> boatVoyages,
           std::unordered_map<char, std::string> shopLocks,
           std::unordered_map<char, std::string> recruits)
    : name_(std::move(name)),
      width_(rows.empty() ? 0 : static_cast<int>(rows.front().size())),
      height_(static_cast<int>(rows.size())),
      entryX_(entryX),
      entryY_(entryY),
      rows_(std::move(rows)),
      pois_(std::move(pois)),
      portals_(std::move(portals)),
      timelineAnchorPoi_(timelineAnchorPoi),
      timelineLocationId_(std::move(timelineLocationId)),
      quests_(std::move(quests)),
      boatVoyages_(std::move(boatVoyages)),
      shopLocks_(std::move(shopLocks)),
      recruits_(std::move(recruits)) {}

char Zone::tileCodeAt(int x, int y) const {
    if (x < 0 || y < 0 || y >= height_ || x >= width_) return '#';
    return rows_[static_cast<size_t>(y)][static_cast<size_t>(x)];
}

const PointOfInterest* Zone::poiAt(int x, int y) const {
    if (x < 0 || y < 0 || y >= height_ || x >= width_) return nullptr;
    char code = rows_[static_cast<size_t>(y)][static_cast<size_t>(x)];
    auto it = pois_.find(code);
    if (it == pois_.end()) return nullptr;
    return &it->second;
}

const std::string* Zone::portalAt(int x, int y) const {
    if (x < 0 || y < 0 || y >= height_ || x >= width_) return nullptr;
    char code = rows_[static_cast<size_t>(y)][static_cast<size_t>(x)];
    auto it = portals_.find(code);
    if (it == portals_.end()) return nullptr;
    return &it->second;
}

const std::string* Zone::questAt(int x, int y) const {
    if (x < 0 || y < 0 || y >= height_ || x >= width_) return nullptr;
    char code = rows_[static_cast<size_t>(y)][static_cast<size_t>(x)];
    auto it = quests_.find(code);
    if (it == quests_.end()) return nullptr;
    return &it->second;
}

const std::string* Zone::shopLockAt(int x, int y) const {
    if (x < 0 || y < 0 || y >= height_ || x >= width_) return nullptr;
    char code = rows_[static_cast<size_t>(y)][static_cast<size_t>(x)];
    auto it = shopLocks_.find(code);
    if (it == shopLocks_.end()) return nullptr;
    return &it->second;
}

const BoatVoyage* Zone::boatAt(int x, int y) const {
    if (x < 0 || y < 0 || y >= height_ || x >= width_) return nullptr;
    char code = rows_[static_cast<size_t>(y)][static_cast<size_t>(x)];
    auto it = boatVoyages_.find(code);
    if (it == boatVoyages_.end()) return nullptr;
    return &it->second;
}

} // namespace world
