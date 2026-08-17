#include "world/ZoneTile.h"

#include <array>

namespace world {

namespace {

constexpr std::array<ZoneTileInfo, 5> kTable = {{
    {'.', '.', "\x1b[92m", true, "open ground"},
    {'%', '%', "\x1b[32m", false, "a vallenwood tree"},
    {'#', '#', "\x1b[90m", false, "a wall"},
    {'~', '~', "\x1b[34m", false, "water"},
    {'+', '+', "\x1b[33m", true, "a doorway"},
}};

constexpr ZoneTileInfo kUnknown{'?', '?', "\x1b[0m", false, "something indistinct"};

} // namespace

const ZoneTileInfo& zoneTileFor(char code) {
    for (const auto& tile : kTable) {
        if (tile.code == code) return tile;
    }
    return kUnknown;
}

} // namespace world
