#include "render/MapRenderer.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace render {

void MapRenderer::drawSchematic(const world::World& world, const game::GameState& state) {
    const auto& locations = world.allLocations();
    if (locations.empty()) {
        std::cout << "(the map is empty)\n";
        return;
    }

    int maxRow = 0;
    int maxCol = 0;
    for (const auto& loc : locations) {
        maxRow = std::max(maxRow, loc.row);
        maxCol = std::max(maxCol, loc.col);
    }
    const int rows = maxRow + 2;
    const int cols = maxCol + 3;
    std::vector<std::string> grid(static_cast<size_t>(rows), std::string(static_cast<size_t>(cols), '.'));

    // Draw roads first so location glyphs always render on top of them.
    // Only orthogonal roads (same row, or same column) are drawn as lines --
    // a general line-drawing algorithm for arbitrary diagonals wasn't worth
    // building for a schematic, hand-placed map. Roads that aren't
    // orthogonal still function for travel; they just aren't drawn here.
    // See docs/MAP_NOTES.md.
    for (const auto& loc : locations) {
        for (const auto& conn : loc.connections) {
            const world::Location* target = world.getLocation(conn.targetId);
            if (!target) continue; // WorldLoader guarantees this can't happen; guard is cheap insurance
            if (loc.row == target->row) {
                int c0 = std::min(loc.col, target->col);
                int c1 = std::max(loc.col, target->col);
                for (int c = c0 + 1; c < c1; ++c) grid[static_cast<size_t>(loc.row)][static_cast<size_t>(c)] = '-';
            } else if (loc.col == target->col) {
                int r0 = std::min(loc.row, target->row);
                int r1 = std::max(loc.row, target->row);
                for (int r = r0 + 1; r < r1; ++r) grid[static_cast<size_t>(r)][static_cast<size_t>(loc.col)] = '|';
            }
        }
    }

    for (const auto& loc : locations) {
        grid[static_cast<size_t>(loc.row)][static_cast<size_t>(loc.col)] = loc.glyph;
    }
    if (const world::Location* here = world.getLocation(state.currentLocationId)) {
        grid[static_cast<size_t>(here->row)][static_cast<size_t>(here->col)] = '@';
    }

    std::cout << "\n";
    for (const auto& rowStr : grid) {
        std::cout << rowStr << "\n";
    }

    std::cout << "\nLegend:  @ = you\n";
    for (const auto& loc : locations) {
        std::cout << "  " << loc.glyph << " = " << loc.name;
        if (loc.id == state.currentLocationId) std::cout << "  (you are here)";
        std::cout << "\n";
    }
}

void MapRenderer::drawLocationScene(const world::World& world, const game::GameState& state) {
    const world::Location* here = world.getLocation(state.currentLocationId);
    if (!here) {
        // Unreachable in practice: main.cpp validates the starting location
        // exists, and GameLoop only ever moves to locations returned by
        // World::findByName. Printed instead of asserted so a future bug
        // here fails loudly in-game rather than crashing the process.
        std::cout << "(you are nowhere -- this is a bug)\n";
        return;
    }

    std::cout << "\n== " << here->name << " (" << here->region << ") ==\n";
    std::cout << here->description << "\n";

    if (here->connections.empty()) {
        std::cout << "There are no roads leading onward from here.\n";
    } else {
        std::cout << "Roads from here:\n";
        for (const auto& conn : here->connections) {
            const world::Location* target = world.getLocation(conn.targetId);
            const std::string targetName = target ? target->name : conn.targetId;
            std::cout << "  - to " << targetName << " (" << conn.travelDays << " day"
                       << (conn.travelDays == 1 ? "" : "s") << ")\n";
        }
    }
    std::cout << "Day " << state.dayCount << ".\n";
}

} // namespace render
