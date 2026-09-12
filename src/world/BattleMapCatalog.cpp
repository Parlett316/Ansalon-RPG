#include "world/BattleMapCatalog.h"
#include "world/BattleMapLoader.h"

#include <array>
#include <stdexcept>

namespace world {

namespace {

// One row per encounter-capable terrain in world::Terrain.cpp's own kTable
// -- MUST stay in sync with that table if a terrain type is ever added,
// removed, or renamed (same cross-file coupling comment style Terrain.h
// itself uses for tools/generate_overworld.py's TERRAIN_CHARS). Ocean,
// the Blood Sea, and shallow water are deliberately absent: all three are
// impassable, so combat can never start while standing on one.
struct TerrainBattleMapEntry {
    char terrainCode;
    const char* filenameStem;
};

constexpr std::array<TerrainBattleMapEntry, 9> kEntries = {{
    {':', "glacier"},
    {'A', "mountains"},
    {'^', "hills"},
    {'%', "forest"},
    {'*', "bog"},
    {'_', "salt_flat"},
    {'"', "savannah"},
    {'.', "grassland"},
    {'#', "road"},
}};

} // namespace

BattleMapCatalog BattleMapCatalog::loadAll(const std::string& dir, int expectedWidth, int expectedHeight) {
    BattleMapCatalog catalog;
    for (const TerrainBattleMapEntry& entry : kEntries) {
        std::string path = dir + "/" + entry.filenameStem + ".txt";
        BattleMap map = BattleMapLoader::loadFromFile(path, expectedWidth, expectedHeight);
        catalog.maps_.emplace(entry.terrainCode, std::move(map));
    }
    return catalog;
}

const BattleMap* BattleMapCatalog::forTerrain(char terrainCode) const {
    auto it = maps_.find(terrainCode);
    if (it == maps_.end()) return nullptr;
    return &it->second;
}

} // namespace world
