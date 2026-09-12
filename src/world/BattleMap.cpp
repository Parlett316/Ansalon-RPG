#include "world/BattleMap.h"

#include <utility>

namespace world {

BattleMap::BattleMap(std::vector<std::string> rows, int width, int height)
    : rows_(std::move(rows)), width_(width), height_(height) {}

bool BattleMap::isWall(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return true;
    return rows_[static_cast<size_t>(y)][static_cast<size_t>(x)] == '#';
}

} // namespace world
