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
    // Ocean is unconditionally impassable -- no general open-water sailing
    // is modeled (Milestone 36's hasBoat/crossableByBoat mechanic was
    // retired in favor of scripted point-to-point voyages; see
    // world::Zone::BoatVoyage / docs/ARCHITECTURE.md). Specific sea
    // journeys (e.g. Tarsis to Ice Wall Castle) are handled entirely as a
    // talk-triggered location jump, not by making ocean tiles walkable.
    {'~', '~', "\x1b[34m", false, 0, "the ocean", 0},
    // The Blood Sea of Istar is supernaturally hazardous in canon -- no
    // lore is invented here about ships crossing it. See docs/MAP_NOTES.md /
    // docs/TIMELINE_NOTES.md.
    {'!', '~', "\x1b[31m", false, 0, "the Blood Sea", 0},
    // Impassable, matching ocean immediately above -- shallow water is no
    // longer a foot-fordable shortcut (it was, briefly: Milestone 84 zeroed
    // its encounter chance and Milestone 87 leaned on its passability to
    // avoid inventing sea-lane roads, both superseded here). The generator's
    // shrink_river_to_coastal_fringe() pass (tools/generate_overworld.py)
    // now also keeps only a real coastal fringe of this code in the grid --
    // the rest of what used to be classified 'river' is 'ocean' -- so what's
    // left is a color-only distinction from ocean (a lighter, "shallow"
    // cyan), same as the Blood Sea's color-only distinction from ocean below.
    // See docs/MAP_NOTES.md's "Shallow water pass" section.
    {'r', '~', "\x1b[36m", false, 0, "shallow water", 0},
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
