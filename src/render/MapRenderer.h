#pragma once

#include "game/GameState.h"
#include "world/OverworldGrid.h"
#include "world/World.h"
#include "world/Zone.h"

#include <string>

namespace render {

// Turns world data into a player-facing ASCII frame. Pure presentation:
// only ever reads its inputs and prints to stdout, never mutates anything.
class MapRenderer {
public:
    static constexpr int kViewportWidth = 78;
    static constexpr int kViewportHeight = 20;
    // Fixed default size, not queried from the actual console -- dynamic
    // resize handling is explicitly deferred (see docs/ARCHITECTURE.md).
    // Assumes a terminal at least this large; README.md documents that
    // assumption. Zones are authored to fit within this size (see
    // docs/ZONE_NOTES.md) so drawZoneFrame needs no camera/scrolling.

    // Renders one full overworld frame: a scrolling colored viewport of the
    // overworld grid centered on the player (clamped at map edges),
    // location glyphs and the player's '@' overlaid, and a status line.
    // `message` is an optional transient line shown once, below the status.
    static void drawOverworldFrame(const world::OverworldGrid& grid, const world::World& world,
                                    const game::GameState& state, const std::string& message);

    // Renders one full zone (interior) frame: the whole zone grid (no
    // camera -- see kViewportWidth/Height above), the entry/exit tile
    // marked, POI glyphs and the player's '@' overlaid, and a status line.
    static void drawZoneFrame(const world::Zone& zone, const game::GameState& state,
                               const std::string& message);
};

} // namespace render
