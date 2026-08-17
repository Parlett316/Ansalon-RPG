#pragma once

#include <string>
#include <unordered_set>

namespace game {

// The player's position and progress in the world. x/y are overworld tile
// coordinates (same space as world::OverworldGrid). hoursElapsed is the
// single source of truth for in-game time -- day/hour are derived from it
// (hoursElapsed/24, hoursElapsed%24) wherever displayed, rather than
// tracked as separate fields, so they can never drift out of sync.
struct GameState {
    int x = 0;
    int y = 0;
    long long hoursElapsed = 0;
    std::unordered_set<std::string> visitedLocations;
};

} // namespace game
