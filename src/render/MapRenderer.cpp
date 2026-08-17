#include "render/MapRenderer.h"
#include "world/Terrain.h"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace render {

void MapRenderer::drawFrame(const world::OverworldGrid& grid, const world::World& world,
                             const game::GameState& state, const std::string& message) {
    std::ostringstream out;

    // Center the viewport on the player, then clamp so it never scrolls
    // past the grid edge (which would show wasted blank space) -- the
    // camera simply stops following the player once they're within half a
    // viewport of a border.
    int left = std::clamp(state.x - kViewportWidth / 2, 0, std::max(0, grid.width() - kViewportWidth));
    int top = std::clamp(state.y - kViewportHeight / 2, 0, std::max(0, grid.height() - kViewportHeight));

    out << "\x1b[2J\x1b[H"; // clear + cursor home (see Console::clearScreen -- same VT100 sequence)

    for (int row = 0; row < kViewportHeight; ++row) {
        int gy = top + row;
        for (int col = 0; col < kViewportWidth; ++col) {
            int gx = left + col;

            char displayChar;
            const char* color;
            if (gx == state.x && gy == state.y) {
                displayChar = '@';
                color = "\x1b[97m"; // bright white -- the player must always read clearly against any terrain
            } else if (const world::Location* loc = world.locationAt(gx, gy)) {
                displayChar = loc->glyph;
                color = "\x1b[93m"; // bright yellow -- locations stand out from raw terrain
            } else {
                const world::TerrainInfo& terrain = world::terrainFor(grid.terrainCodeAt(gx, gy));
                displayChar = terrain.glyph;
                color = terrain.ansiColor;
            }

            // Every color-set code is paired with a reset immediately
            // after -- see docs/GOTCHAS.md on why (color bleed).
            out << color << displayChar << "\x1b[0m";
        }
        out << "\n";
    }

    out << "\n";
    if (const world::Location* here = world.locationAt(state.x, state.y)) {
        out << "== " << here->name << " (" << here->region << ") ==\n" << here->description << "\n";
    } else {
        const world::TerrainInfo& underfoot = world::terrainFor(grid.terrainCodeAt(state.x, state.y));
        out << "You are in " << underfoot.name << ".\n";
    }
    out << "Day " << (state.hoursElapsed / 24) << ", hour " << (state.hoursElapsed % 24) << ".";
    if (!message.empty()) {
        out << "  " << message;
    }
    out << "\n";
    out << "Move: arrows/hjkl/yubn/wasd   ;=look around   q=quit\n";

    // The whole frame is built as one string and written in a single
    // flush -- this matters far more here than in Milestone 1, since a
    // redraw now happens on every single keystroke instead of every typed
    // command. Many small writes would flicker visibly. See docs/GOTCHAS.md.
    std::cout << out.str();
}

} // namespace render
