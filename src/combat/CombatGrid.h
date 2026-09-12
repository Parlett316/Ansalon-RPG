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

// True if a straight line from `a` to `b` isn't blocked by any wall cell in
// `walls` -- Milestone 189's gate for ranged-weapon and spell targeting now
// that real battlemap walls exist (see docs/COMBAT_NOTES.md's "Line of
// sight" section). A standard integer Bresenham line walk; the two
// endpoints themselves are never checked against `walls` -- `a`/`b` are
// always the shooter's/caster's own occupied cell and a live target's own
// occupied cell, neither of which can be a wall. `walls` takes the same
// flattened-list shape stepToward/stepTowardBfs's own `blocked` parameter
// already does, so a fight's existing CombatSession::wallPositions plugs
// straight in with no new per-call cost. Deliberately a plain sightline
// check -- it does NOT apply the movement-only corner-cutting nuance
// stepToward/stepTowardBfs's callers layer on separately for diagonal
// steps; a line that grazes between two diagonal wall cells is not treated
// specially here.
bool hasLineOfSight(GridPos a, GridPos b, const std::vector<GridPos>& walls);

// Mirrors `from` through `target`: the cell exactly opposite `from` on the
// far side of `target` (target + (target - from)). Milestone 119's thief
// backstab uses this -- DQoK.pdf's own manual: "A thief 'back stabs' if he
// attacks a target from exactly opposite the first character to attack the
// target." Always lands on one of the 8 cells around `target` when `from`
// is itself one of them (isAdjacent(target, from) true), since the offset
// is just negated. Doesn't itself check adjacency -- callers compare the
// result against the backstabbing character's actual position.
GridPos oppositeSide(GridPos target, GridPos from);

// Milestone 190: multi-square creatures (Ogre/Troll 1x2, Griffon 2x1, the
// Blue Dragon 2x2 -- see combat::Monster::footprintWidth/Height and
// docs/COMBAT_NOTES.md). `anchor` is the footprint's top-left cell; a
// footprint occupies [anchor.x, anchor.x+width) x [anchor.y, anchor.y+height).
// A 1x1 footprint (every monster before this milestone) degenerates to
// exactly the single-cell behavior every other function above already has.
// SFML-only callers -- ansalon_rpg's console combat has no footprint
// concept and never calls these.

// Every cell the footprint anchored at `anchor` occupies.
std::vector<GridPos> footprintCells(GridPos anchor, int width, int height);

// True if `point` is Chebyshev-adjacent (isAdjacent) to ANY cell of the
// footprint anchored at `anchor` -- the footprint-aware generalization of
// isAdjacent for melee/ranged eligibility, opportunity attacks, and a
// multi-cell monster's own "am I next to my target" check.
bool isAdjacentToFootprint(GridPos point, GridPos anchor, int width, int height);

// True if every cell of the footprint anchored at `anchor` is in bounds
// ([0,gridWidth) x [0,gridHeight)) and absent from `blocked` (walls and/or
// occupied cells, merged by the caller same as stepToward/stepTowardBfs's
// own `blocked`) -- the core primitive spawn placement and multi-cell
// movement validation both build on.
bool footprintFits(GridPos anchor, int width, int height, int gridWidth, int gridHeight,
                    const std::vector<GridPos>& blocked);

// Whichever cell of the footprint anchored at `anchor` is closest (Chebyshev)
// to `from` -- Milestone 189's hasLineOfSight needs a single endpoint, and
// the nearest footprint cell (not the bare anchor) is the correct one: a
// shooter peeking around a corner can see the near edge of a big creature
// even if its far edge is behind a wall.
GridPos nearestFootprintCell(GridPos from, GridPos anchor, int width, int height);

// One greedy step for the footprint anchored at `anchor` toward `to`, same
// "prefer the larger remaining gap, fall back to the other axis, stand
// still if both are blocked" heuristic as stepToward -- but a candidate
// step is only taken if footprintFits holds for the WHOLE footprint at the
// destination, not just its anchor cell. Deliberately not a footprint-aware
// stepTowardBfs: a real multi-cell BFS (a distance field over footprint
// *placements*, not single cells) is a non-trivial generalization this
// milestone doesn't attempt -- every multi-cell creature in this roster is
// solo (MonsterLoader enforces this), so getting stuck on a wall corner a
// smarter path would avoid is an accepted, documented restraint, not an
// oversight. See docs/COMBAT_NOTES.md.
GridPos stepFootprintToward(GridPos anchor, GridPos to, int width, int height, int gridWidth, int gridHeight,
                             const std::vector<GridPos>& blocked);

} // namespace combat
