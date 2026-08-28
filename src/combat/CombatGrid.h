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

// max(|dx|, |dy|) -- the same "how many steps to close this gap" metric
// isAdjacent already uses at range 1, generalized to any distance.
// Milestone 117 uses this to let a monster AI pick whichever of two
// possible targets (player vs. companion) is nearer when it's adjacent to
// neither -- extracted as its own pure function for the same
// unit-testability reasoning as stepToward above.
int chebyshevDistance(GridPos a, GridPos b);

// Mirrors `from` through `target`: the cell exactly opposite `from` on the
// far side of `target` (target + (target - from)). Milestone 119's thief
// backstab uses this -- DQoK.pdf's own manual: "A thief 'back stabs' if he
// attacks a target from exactly opposite the first character to attack the
// target." Always lands on one of the 8 cells around `target` when `from`
// is itself one of them (isAdjacent(target, from) true), since the offset
// is just negated. Doesn't itself check adjacency -- callers compare the
// result against the backstabbing character's actual position.
GridPos oppositeSide(GridPos target, GridPos from);

} // namespace combat
