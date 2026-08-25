#include "world/Terrain.h"

#include <array>

namespace world {

namespace {

// minutesToCross values are invented for gameplay pacing, not derived from
// any canon source (same spirit as Milestone 1's road travel-times -- see
// docs/MAP_NOTES.md). They're a straight x15 scaling of this project's
// original flat-hour costs (1-6 hours -> 15-90 minutes, Milestone 67) --
// same relative tuning between terrain types, just finer-grained; see
// docs/MAP_NOTES.md's "Movement granularity" section for why. Roads are
// always the cheapest regardless of the terrain they're drawn over, which
// is what makes following one actually pay off when it cuts through
// forest/hills. encounterChancePercent is likewise invented, tuned (not
// sourced) risk -- roads safest, forest/mountains riskiest -- see
// docs/COMBAT_NOTES.md's "Encounters" section.
constexpr std::array<TerrainInfo, 12> kTable = {{
    // Ocean's minutesToCross/encounterChancePercent were irrelevant while it
    // was unconditionally impassable; now that GameState::hasBoat can cross
    // it (Milestone 36), it gets real (still invented-for-pacing) values.
    // encounterChancePercent stays 0 -- no sea monsters exist in the
    // monster roster yet, so a random encounter here would draw a land
    // creature into open water; revisit if sea monsters are ever added.
    {'~', '~', "\x1b[34m", false, 30, "the ocean", 0, true},
    // The Blood Sea stays impassable even with a boat -- not an oversight,
    // a sourced restraint: the Blood Sea of Istar is supernaturally
    // hazardous in canon, and no lore is invented here about ships
    // crossing it. See docs/MAP_NOTES.md / docs/TIMELINE_NOTES.md.
    {'!', '~', "\x1b[31m", false, 0, "the Blood Sea", 0},
    // Zeroed (Milestone 84) for the same reason ocean/Blood Sea are 0%: no
    // sea monsters exist in the roster, and per docs/MAP_NOTES.md this code
    // is essentially always coastal water in the generated grid (real river
    // fords never survived downsampling), so it's crossed constantly while
    // sailing -- a nonzero chance here meant land monsters ambushing the
    // player mid-voyage. Was 5 until this milestone.
    {'r', '~', "\x1b[36m", true, 30, "shallow water", 0},
    {':', '.', "\x1b[97m", true, 60, "glacier", 6},
    {'A', '^', "\x1b[90m", true, 90, "mountains", 12},
    {'^', '^', "\x1b[33m", true, 45, "hills", 8},
    {'%', '%', "\x1b[32m", true, 30, "forest", 11},
    {'*', '"', "\x1b[35m", true, 60, "bog", 9},
    {'_', '.', "\x1b[93m", true, 30, "salt flat", 4},
    {'"', '"', "\x1b[93m", true, 15, "savannah", 5},
    {'.', '.', "\x1b[92m", true, 15, "grassland", 5},
    {'#', '#', "\x1b[37m", true, 15, "road", 2},
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
