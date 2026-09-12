#pragma once

#include "world/BattleMap.h"

#include <string>
#include <unordered_map>

namespace world {

// Loads every data/battlemaps/*.txt file at startup and looks one up by the
// overworld world::TerrainInfo::code the fight started on (see
// docs/COMBAT_NOTES.md's Milestone 188 section). Only the terrains that are
// ever both passable and have a nonzero encounterChancePercent (see
// Terrain.cpp) get a battlemap -- ocean/Blood Sea/shallow water never
// trigger combat, so they're not in this table.
class BattleMapCatalog {
public:
    // Loads all known terrain battlemaps from `dir`, each validated to be
    // exactly expectedWidth x expectedHeight (sfml_phase1/main.cpp's own
    // kCombatGridWidth/kCombatGridHeight) -- fails fast (throws
    // std::runtime_error) if any file is missing or malformed, same
    // "let main()'s outer try/catch turn this into a FATAL EXCEPTION"
    // precedent every other startup catalog already relies on.
    static BattleMapCatalog loadAll(const std::string& dir, int expectedWidth, int expectedHeight);

    // Returns the battlemap for this terrain code, or nullptr if none is
    // registered for it (defensive only -- in practice combat only ever
    // starts on one of the codes this catalog covers, the same guarantee
    // combat::MonsterCatalog::randomMonster already relies on).
    const BattleMap* forTerrain(char terrainCode) const;

private:
    std::unordered_map<char, BattleMap> maps_;
};

} // namespace world
