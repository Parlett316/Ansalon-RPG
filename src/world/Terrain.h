#pragma once

namespace world {

// The fixed vocabulary of overworld terrain types. Deliberately code, not
// data: unlike location content (endlessly variable, hand-authored), the
// set of terrain kinds is small and tightly coupled to rendering/movement
// rules -- a data file here would just be indirection with no real
// flexibility gained. The single-character `code` values MUST match
// TERRAIN_CHARS in tools/generate_overworld.py; that script is what writes
// those codes into data/overworld.grid.
struct TerrainInfo {
    char code;             // character stored in data/overworld.grid
    char glyph;            // character actually drawn on screen
    const char* ansiColor; // ANSI SGR "set color" sequence -- callers must
                            // always follow it with a reset ("\x1b[0m"), or
                            // color bleeds into later output; see docs/GOTCHAS.md
    bool passable;
    int hoursToCross;      // in-game hours consumed by stepping onto this tile
    const char* name;      // shown in the status line, e.g. "forest"
    int encounterChancePercent; // risk of a random encounter per move onto
                                 // this tile -- see docs/COMBAT_NOTES.md
};

// Looks up terrain info for a character read from data/overworld.grid.
// Returns an "uncharted" entry (impassable) for any character not in the
// table, so a corrupted/hand-edited data file fails safe (blocks movement)
// instead of the renderer indexing something undefined.
const TerrainInfo& terrainFor(char code);

} // namespace world
