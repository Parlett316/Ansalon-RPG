#pragma once

#include "game/GameState.h"
#include "world/World.h"

namespace render {

// Turns World data into player-facing ASCII output. Pure presentation: only
// ever reads World/GameState and prints to stdout, never mutates anything.
// Keeping it side-effect-free on its inputs means later additions (drawing
// NPCs on the schematic, for instance) only ever need new read-only data to
// draw, not a redesign of this interface.
class MapRenderer {
public:
    // Draws the continent schematic: every known location plotted at its
    // stylized (row, col) grid position, with '@' marking the player.
    static void drawSchematic(const world::World& world, const game::GameState& state);

    // Draws the current location's name, description, and the roads
    // leading out of it.
    static void drawLocationScene(const world::World& world, const game::GameState& state);
};

} // namespace render
