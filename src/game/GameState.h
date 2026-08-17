#pragma once

#include "character/Character.h"

#include <string>
#include <unordered_set>

namespace game {

enum class Mode {
    Overworld,
    Zone,
};

// The player's position and progress in the world. x/y are overworld tile
// coordinates (same space as world::OverworldGrid), always kept valid even
// while inside a zone -- entering a zone doesn't move the overworld
// position, it just changes what's rendered/walked on, so exiting returns
// the player to exactly where they were. zoneX/zoneY/currentZoneId are only
// meaningful when mode == Mode::Zone. hoursElapsed is the single source of
// truth for in-game time -- day/hour are derived from it
// (hoursElapsed/24, hoursElapsed%24) wherever displayed, rather than
// tracked as separate fields, so they can never drift out of sync. Walking
// inside a zone does not advance hoursElapsed (see docs/ARCHITECTURE.md) --
// only overworld travel does.
struct GameState {
    character::Character character; // produced once by CharacterCreator before the loop starts

    Mode mode = Mode::Overworld;
    int x = 0;
    int y = 0;
    long long hoursElapsed = 0;
    std::unordered_set<std::string> visitedLocations;

    std::string currentZoneId;
    int zoneX = 0;
    int zoneY = 0;
};

} // namespace game
