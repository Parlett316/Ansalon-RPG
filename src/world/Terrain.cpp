#include "world/Terrain.h"

#include <array>

namespace world {

namespace {

// hoursToCross values are invented for gameplay pacing, not derived from
// any canon source (same spirit as Milestone 1's road travel-times -- see
// docs/MAP_NOTES.md). Roads are always 1 hour regardless of the terrain
// they're drawn over, which is what makes following one actually pay off
// when it cuts through forest/hills. encounterChancePercent is likewise
// invented, tuned (not sourced) risk -- roads safest, forest/mountains
// riskiest -- see docs/COMBAT_NOTES.md's "Encounters" section.
constexpr std::array<TerrainInfo, 12> kTable = {{
    // Ocean's hoursToCross/encounterChancePercent were irrelevant while it
    // was unconditionally impassable; now that GameState::hasBoat can cross
    // it (Milestone 36), it gets real (still invented-for-pacing) values.
    // encounterChancePercent stays 0 -- no sea monsters exist in the
    // monster roster yet, so a random encounter here would draw a land
    // creature into open water; revisit if sea monsters are ever added.
    {'~', '~', "\x1b[34m", false, 2, "the ocean", 0, true},
    // The Blood Sea stays impassable even with a boat -- not an oversight,
    // a sourced restraint: the Blood Sea of Istar is supernaturally
    // hazardous in canon, and no lore is invented here about ships
    // crossing it. See docs/MAP_NOTES.md / docs/TIMELINE_NOTES.md.
    {'!', '~', "\x1b[31m", false, 0, "the Blood Sea", 0},
    {'r', '~', "\x1b[36m", true, 2, "shallow water", 5},
    {':', '.', "\x1b[97m", true, 4, "glacier", 6},
    {'A', '^', "\x1b[90m", true, 6, "mountains", 12},
    {'^', '^', "\x1b[33m", true, 3, "hills", 8},
    {'%', '%', "\x1b[32m", true, 2, "forest", 11},
    {'*', '"', "\x1b[35m", true, 4, "bog", 9},
    {'_', '.', "\x1b[93m", true, 2, "salt flat", 4},
    {'"', '"', "\x1b[93m", true, 1, "savannah", 5},
    {'.', '.', "\x1b[92m", true, 1, "grassland", 5},
    {'#', '#', "\x1b[37m", true, 1, "road", 2},
}};

constexpr TerrainInfo kUncharted{'?', '?', "\x1b[0m", false, 0, "uncharted territory", 0};

} // namespace

const TerrainInfo& terrainFor(char code) {
    for (const auto& terrain : kTable) {
        if (terrain.code == code) return terrain;
    }
    return kUncharted;
}

} // namespace world
