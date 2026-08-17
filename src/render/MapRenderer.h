#pragma once

#include "game/GameState.h"
#include "world/OverworldGrid.h"
#include "world/World.h"

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
    // assumption.

    // Renders one full frame: a scrolling colored viewport of the
    // overworld grid centered on the player (clamped at map edges),
    // location glyphs and the player's '@' overlaid, and a status line.
    // `message` is an optional transient line (e.g. "You cannot cross the
    // ocean here.") shown once, below the status line.
    static void drawFrame(const world::OverworldGrid& grid, const world::World& world,
                           const game::GameState& state, const std::string& message);
};

} // namespace render
