#pragma once

namespace world {

// The fixed vocabulary of zone (interior) tile types -- a small, separate
// table from world::Terrain (see docs/ARCHITECTURE.md): overworld biomes
// and hand-authored interior scenes are different vocabularies with
// different sources (generated vs. hand-drawn), so conflating them into one
// table would just create confusing overlap (what would "mountains" mean
// inside a building?).
struct ZoneTileInfo {
    char code;             // character stored in a data/zones/*.txt GRID block
    char glyph;             // character actually drawn on screen
    const char* ansiColor; // ANSI SGR "set color" sequence -- caller must
                            // always follow with a reset; see docs/GOTCHAS.md
    bool passable;
    const char* name;      // shown in the status line, e.g. "a stand of vallenwoods"
};

// Looks up tile info for a character read from a zone's GRID block. Returns
// an impassable "unknown" entry for any character not in the table, so a
// typo in a hand-authored zone file fails safe (blocks movement) rather
// than the renderer indexing something undefined. ZoneLoader separately
// validates every GRID character is either in this table or a declared POI,
// so this fallback should never actually be reached in practice.
const ZoneTileInfo& zoneTileFor(char code);

} // namespace world
