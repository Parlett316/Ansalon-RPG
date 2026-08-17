#include "render/MapRenderer.h"
#include "world/Terrain.h"
#include "world/ZoneTile.h"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace render {

void MapRenderer::drawOverworldFrame(const world::OverworldGrid& grid, const world::World& world,
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
    out << "Move: arrows/hjkl/yubn/wasd   ;=look around   Enter=step in   q=quit\n";

    // The whole frame is built as one string and written in a single
    // flush -- this matters far more here than in Milestone 1, since a
    // redraw now happens on every single keystroke instead of every typed
    // command. Many small writes would flicker visibly. See docs/GOTCHAS.md.
    std::cout << out.str();
}

void MapRenderer::drawZoneFrame(const world::Zone& zone, const game::GameState& state,
                                 const std::string& message) {
    std::ostringstream out;
    out << "\x1b[2J\x1b[H";

    for (int y = 0; y < zone.height(); ++y) {
        for (int x = 0; x < zone.width(); ++x) {
            char displayChar;
            const char* color;
            if (x == state.zoneX && y == state.zoneY) {
                displayChar = '@';
                color = "\x1b[97m";
            } else if (const world::PointOfInterest* poi = zone.poiAt(x, y)) {
                displayChar = poi->code;
                color = "\x1b[93m"; // bright yellow, matching overworld location markers
            } else if (x == zone.entryX() && y == zone.entryY()) {
                displayChar = '>';
                color = "\x1b[96m"; // bright cyan -- the way back out
            } else {
                const world::ZoneTileInfo& tile = world::zoneTileFor(zone.tileCodeAt(x, y));
                displayChar = tile.glyph;
                color = tile.ansiColor;
            }
            out << color << displayChar << "\x1b[0m";
        }
        out << "\n";
    }

    out << "\n== " << zone.name() << " ==\n";
    if (const world::PointOfInterest* here = zone.poiAt(state.zoneX, state.zoneY)) {
        out << here->name << ": " << here->description << "\n";
    } else if (state.zoneX == zone.entryX() && state.zoneY == zone.entryY()) {
        out << "You stand at the way back out.\n";
    } else {
        out << "\n";
    }
    if (!message.empty()) {
        out << message << "\n";
    }
    out << "Move: arrows/hjkl/yubn/wasd   ;=look   Enter=leave (from the '>' marker)   q=quit\n";

    std::cout << out.str();
}

} // namespace render
