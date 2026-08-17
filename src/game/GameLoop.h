#pragma once

#include "game/GameState.h"
#include "world/OverworldGrid.h"
#include "world/World.h"

#include <string>

namespace game {

// Owns the render -> read-key -> update cycle. Movement is dispatched
// directly from render::Key -- there is no verb/command parser anymore
// (Command.h/.cpp was deleted in Milestone 2): a per-keystroke walking game
// doesn't fit a "type a word, press enter" model. See docs/ARCHITECTURE.md.
class GameLoop {
public:
    GameLoop(const world::World& world, const world::OverworldGrid& grid, GameState initialState);

    void run();

private:
    void tryMove(int dx, int dy);
    void look();

    const world::World& world_;
    const world::OverworldGrid& grid_;
    GameState state_;
    std::string message_; // transient, shown for one frame then cleared
};

} // namespace game
