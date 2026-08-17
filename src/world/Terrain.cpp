#include "world/Terrain.h"

#include <array>

namespace world {

namespace {

// hoursToCross values are invented for gameplay pacing, not derived from
// any canon source (same spirit as Milestone 1's road travel-times -- see
// docs/MAP_NOTES.md). Roads are always 1 hour regardless of the terrain
// they're drawn over, which is what makes following one actually pay off
// when it cuts through forest/hills.
constexpr std::array<TerrainInfo, 12> kTable = {{
    {'~', '~', "\x1b[34m", false, 0, "the ocean"},
    {'!', '~', "\x1b[31m", false, 0, "the Blood Sea"},
    {'r', '~', "\x1b[36m", true, 2, "shallow water"},
    {':', '.', "\x1b[97m", true, 4, "glacier"},
    {'A', '^', "\x1b[90m", true, 6, "mountains"},
    {'^', '^', "\x1b[33m", true, 3, "hills"},
    {'%', '%', "\x1b[32m", true, 2, "forest"},
    {'*', '"', "\x1b[35m", true, 4, "bog"},
    {'_', '.', "\x1b[93m", true, 2, "salt flat"},
    {'"', '"', "\x1b[93m", true, 1, "savannah"},
    {'.', '.', "\x1b[92m", true, 1, "grassland"},
    {'#', '#', "\x1b[37m", true, 1, "road"},
}};

constexpr TerrainInfo kUncharted{'?', '?', "\x1b[0m", false, 0, "uncharted territory"};

} // namespace

const TerrainInfo& terrainFor(char code) {
    for (const auto& terrain : kTable) {
        if (terrain.code == code) return terrain;
    }
    return kUncharted;
}

} // namespace world
