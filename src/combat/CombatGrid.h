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

// A real shortest-path step toward `to`, avoiding every cell in `blocked`
// (occupied cells and wall cells, merged by the caller into one list) --
// Milestone 188's wall-aware replacement for stepToward's greedy approach,
// needed once real wall geometry exists on the grid (data/battlemaps/*.txt,
// see docs/COMBAT_NOTES.md): stepToward's single-axis heuristic can dead-end
// against a wall corner that a real path would simply go around. Computes a
// full BFS distance field outward from `to` (cardinal neighbors only,
// matching stepToward's own existing cardinal-only movement -- this stays a
// pathing-quality fix, not a new AI-diagonal-movement feature), then steps
// `from` toward whichever of its own unblocked cardinal neighbors has the
// smallest distance to `to` (ties broken by whichever is closer to `to` in
// a straight line). Same "stands still" contract as stepToward: returns
// `from` unchanged if no neighbor is both unblocked and able to reach `to`
// at all. Doesn't require `to` itself to be unblocked -- like stepToward,
// callers routinely include a live target's own occupied cell in `blocked`.
// SFML-only caller (sfml_phase1/main.cpp's combatMonstersAct/
// combatCompanionActs) -- stepToward itself is untouched and still used by
// the console build, which has no walls to route around.
GridPos stepTowardBfs(GridPos from, GridPos to, int width, int height, const std::vector<GridPos>& blocked);

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
