#pragma once

#include "world/BattleMap.h"

#include <string>

namespace world {

// Parses one data/battlemaps/<terrain-name>.txt file -- see
// docs/COMBAT_NOTES.md's Milestone 188 section for the full grammar.
// Fails fast (file:line: message, same idiom as ZoneLoader/WorldLoader) on:
// the file not opening, a GRID row whose width doesn't match
// `expectedWidth`, a row count that doesn't match `expectedHeight`, any
// character other than '.'/'#' in the grid, and a non-fully-open row 0 or
// last row (both are combat's fixed monster/player-and-companion spawn
// rows -- see sfml_phase1/main.cpp's combatStartEncounter -- so a hand-
// authored wall placed there would silently trap a spawn).
class BattleMapLoader {
public:
    static BattleMap loadFromFile(const std::string& path, int expectedWidth, int expectedHeight);
};

} // namespace world
