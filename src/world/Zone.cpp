#include "world/Zone.h"

namespace world {

Zone::Zone(std::string name, std::vector<std::string> rows, int entryX, int entryY,
           std::unordered_map<char, PointOfInterest> pois)
    : name_(std::move(name)),
      width_(rows.empty() ? 0 : static_cast<int>(rows.front().size())),
      height_(static_cast<int>(rows.size())),
      entryX_(entryX),
      entryY_(entryY),
      rows_(std::move(rows)),
      pois_(std::move(pois)) {}

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

} // namespace world
