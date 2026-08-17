#pragma once

#include "game/GameState.h"
#include "world/World.h"

#include <string>

namespace game {

// Owns the read -> parse -> update -> render cycle. This is the one class
// that knows about World, GameState, and rendering all at once --
// everything else (World, MapRenderer, Command) is deliberately kept from
// depending on the others so they stay independently reusable once later
// milestones (character, timeline, combat) need to plug in here too.
class GameLoop {
public:
    GameLoop(const world::World& world, GameState initialState);

    void run();

private:
    void handleGo(const std::string& destinationName);
    void printHelp() const;

    const world::World& world_;
    GameState state_;
};

} // namespace game
