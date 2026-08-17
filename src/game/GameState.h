#pragma once

#include <string>
#include <unordered_set>

namespace game {

// Everything about the player's current situation in the world.
// Deliberately minimal for milestone 1: dayCount is a plain counter, not a
// real in-world calendar, and there is no Character yet. Both are known
// extension points for later milestones (see docs/ARCHITECTURE.md) -- when
// they're built, add fields to THIS struct rather than introducing a
// parallel state object, so GameLoop/MapRenderer/the future timeline engine
// all keep reading from one source of truth.
struct GameState {
    std::string currentLocationId;
    int dayCount = 0;
    std::unordered_set<std::string> visitedLocations;
};

} // namespace game
