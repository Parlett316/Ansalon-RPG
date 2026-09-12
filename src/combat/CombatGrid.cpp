#include "combat/CombatGrid.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace combat {

bool isAdjacent(GridPos a, GridPos b) {
    if (a.x == b.x && a.y == b.y) return false;
    return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y)) <= 1;
}

GridPos stepToward(GridPos from, GridPos to, int width, int height, const std::vector<GridPos>& blocked) {
    auto inBounds = [&](GridPos p) { return p.x >= 0 && p.x < width && p.y >= 0 && p.y < height; };
    auto isBlocked = [&](GridPos p) {
        for (const GridPos& b : blocked) {
            if (b.x == p.x && b.y == p.y) return true;
        }
        return false;
    };
    auto tryStep = [&](int dx, int dy) -> bool {
        GridPos candidate{from.x + dx, from.y + dy};
        return inBounds(candidate) && !isBlocked(candidate);
    };

    int dx = to.x - from.x;
    int dy = to.y - from.y;
    int stepX = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
    int stepY = dy > 0 ? 1 : (dy < 0 ? -1 : 0);

    // Prefer whichever axis currently has the larger gap to close.
    bool xFirst = std::abs(dx) >= std::abs(dy);
    if (xFirst) {
        if (stepX != 0 && tryStep(stepX, 0)) return {from.x + stepX, from.y};
        if (stepY != 0 && tryStep(0, stepY)) return {from.x, from.y + stepY};
    } else {
        if (stepY != 0 && tryStep(0, stepY)) return {from.x, from.y + stepY};
        if (stepX != 0 && tryStep(stepX, 0)) return {from.x + stepX, from.y};
    }
    return from; // fully blocked, or already at the target's own cell
}

GridPos stepTowardBfs(GridPos from, GridPos to, int width, int height, const std::vector<GridPos>& blocked) {
    auto inBounds = [&](GridPos p) { return p.x >= 0 && p.x < width && p.y >= 0 && p.y < height; };
    if (width <= 0 || height <= 0 || !inBounds(from) || !inBounds(to)) return from;
    if (from.x == to.x && from.y == to.y) return from;

    const size_t cellCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    auto index = [&](GridPos p) {
        return static_cast<size_t>(p.y) * static_cast<size_t>(width) + static_cast<size_t>(p.x);
    };

    std::vector<bool> isBlocked(cellCount, false);
    for (const GridPos& b : blocked) {
        if (inBounds(b)) isBlocked[index(b)] = true;
    }

    // BFS outward from `to`, cardinal directions only -- builds a full
    // shortest-distance field in one pass rather than repeatedly probing
    // single candidate steps the way stepToward's greedy approach does, so
    // it can't get fooled by a wall corner a real path would go around.
    // Seeded directly at distance 0 regardless of isBlocked[to] -- `to` is
    // reachable by definition even when it's itself a live target's own
    // occupied cell (see this function's own doc comment); only cells
    // discovered via expansion from a neighbor are ever rejected for being
    // blocked.
    std::vector<int> distance(cellCount, -1);
    std::queue<GridPos> frontier;
    distance[index(to)] = 0;
    frontier.push(to);
    static constexpr int kDx[4] = {1, -1, 0, 0};
    static constexpr int kDy[4] = {0, 0, 1, -1};
    while (!frontier.empty()) {
        GridPos cur = frontier.front();
        frontier.pop();
        int curDist = distance[index(cur)];
        for (int dir = 0; dir < 4; ++dir) {
            GridPos next{cur.x + kDx[dir], cur.y + kDy[dir]};
            if (!inBounds(next) || distance[index(next)] != -1 || isBlocked[index(next)]) continue;
            distance[index(next)] = curDist + 1;
            frontier.push(next);
        }
    }

    // Step to whichever of from's own unblocked cardinal neighbors has the
    // smallest distance-to-`to`; ties broken by straight-line closeness to
    // `to` (a stand-in for stepToward's own "prefer the larger remaining
    // gap" tie-break, in a form that generalizes cleanly to a distance
    // field instead of a single dx/dy pair).
    GridPos best = from;
    int bestDist = -1;
    int bestTie = -1;
    for (int dir = 0; dir < 4; ++dir) {
        GridPos candidate{from.x + kDx[dir], from.y + kDy[dir]};
        if (!inBounds(candidate) || isBlocked[index(candidate)]) continue;
        int d = distance[index(candidate)];
        if (d == -1) continue; // unreachable given the current walls/occupants
        int tie = std::abs(to.x - candidate.x) + std::abs(to.y - candidate.y);
        if (bestDist == -1 || d < bestDist || (d == bestDist && tie < bestTie)) {
            bestDist = d;
            bestTie = tie;
            best = candidate;
        }
    }
    return best;
}

int chebyshevDistance(GridPos a, GridPos b) {
    return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

GridPos oppositeSide(GridPos target, GridPos from) {
    return {2 * target.x - from.x, 2 * target.y - from.y};
}

} // namespace combat
