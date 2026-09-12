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

bool hasLineOfSight(GridPos a, GridPos b, const std::vector<GridPos>& walls) {
    auto isWall = [&](GridPos p) {
        for (const GridPos& w : walls) {
            if (w.x == p.x && w.y == p.y) return true;
        }
        return false;
    };

    int x0 = a.x, y0 = a.y;
    const int x1 = b.x, y1 = b.y;
    const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        bool isEndpoint = (x0 == a.x && y0 == a.y) || (x0 == x1 && y0 == y1);
        if (!isEndpoint && isWall({x0, y0})) return false;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return true;
}

GridPos oppositeSide(GridPos target, GridPos from) {
    return {2 * target.x - from.x, 2 * target.y - from.y};
}

std::vector<GridPos> footprintCells(GridPos anchor, int width, int height) {
    std::vector<GridPos> cells;
    cells.reserve(static_cast<size_t>(width) * static_cast<size_t>(height));
    for (int dy = 0; dy < height; ++dy) {
        for (int dx = 0; dx < width; ++dx) {
            cells.push_back({anchor.x + dx, anchor.y + dy});
        }
    }
    return cells;
}

bool isAdjacentToFootprint(GridPos point, GridPos anchor, int width, int height) {
    for (const GridPos& cell : footprintCells(anchor, width, height)) {
        if (isAdjacent(point, cell)) return true;
    }
    return false;
}

bool footprintFits(GridPos anchor, int width, int height, int gridWidth, int gridHeight,
                    const std::vector<GridPos>& blocked) {
    for (const GridPos& cell : footprintCells(anchor, width, height)) {
        if (cell.x < 0 || cell.x >= gridWidth || cell.y < 0 || cell.y >= gridHeight) return false;
        for (const GridPos& b : blocked) {
            if (b.x == cell.x && b.y == cell.y) return false;
        }
    }
    return true;
}

GridPos nearestFootprintCell(GridPos from, GridPos anchor, int width, int height) {
    GridPos best = anchor;
    int bestDist = -1;
    for (const GridPos& cell : footprintCells(anchor, width, height)) {
        int d = chebyshevDistance(from, cell);
        if (bestDist == -1 || d < bestDist) {
            bestDist = d;
            best = cell;
        }
    }
    return best;
}

GridPos stepFootprintToward(GridPos anchor, GridPos to, int width, int height, int gridWidth, int gridHeight,
                             const std::vector<GridPos>& blocked) {
    auto tryStep = [&](int dx, int dy) -> bool {
        GridPos candidate{anchor.x + dx, anchor.y + dy};
        return footprintFits(candidate, width, height, gridWidth, gridHeight, blocked);
    };

    int dx = to.x - anchor.x;
    int dy = to.y - anchor.y;
    int stepX = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
    int stepY = dy > 0 ? 1 : (dy < 0 ? -1 : 0);

    bool xFirst = std::abs(dx) >= std::abs(dy);
    if (xFirst) {
        if (stepX != 0 && tryStep(stepX, 0)) return {anchor.x + stepX, anchor.y};
        if (stepY != 0 && tryStep(0, stepY)) return {anchor.x, anchor.y + stepY};
    } else {
        if (stepY != 0 && tryStep(0, stepY)) return {anchor.x, anchor.y + stepY};
        if (stepX != 0 && tryStep(stepX, 0)) return {anchor.x + stepX, anchor.y};
    }
    return anchor; // fully blocked, or already at the target's own cell
}

} // namespace combat
