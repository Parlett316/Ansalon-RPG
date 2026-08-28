#pragma once

#include <vector>

namespace combat {

// One cell on the small tactical grid GameLoop::runCombat lays out for
// Milestone 114's positional combat -- see docs/COMBAT_NOTES.md's
// "Positional combat grid" section. Purely local math, no dependency on
// character::/world:: -- same independence as Monster.h/Combat.h.
struct GridPos {
    int x = 0;
    int y = 0;
};

// Chebyshev distance <= 1 (the 8 surrounding cells) -- a position is never
// "adjacent" to itself. This is the sourced adjacency rule DQoK.pdf's own
// manual describes for melee range and ranged-weapon lockout.
bool isAdjacent(GridPos a, GridPos b);

// One greedy step from `from` toward `to`, preferring whichever axis has
// the larger remaining gap; falls back to the other axis if the preferred
// step would leave the grid (`width`/`height`) or land on a `blocked`
// cell, and stands still (returns `from` unchanged) if both are blocked.
// Extracted as its own function (rather than left inline in
// GameLoop::runCombat's monster-AI lambda) specifically so it's
// unit-testable, same reasoning as combat::rollGroupSize.
GridPos stepToward(GridPos from, GridPos to, int width, int height, const std::vector<GridPos>& blocked);

} // namespace combat
