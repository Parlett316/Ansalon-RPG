#include "combat/CombatGrid.h"

#include <algorithm>
#include <cmath>

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

int chebyshevDistance(GridPos a, GridPos b) {
    return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

} // namespace combat
