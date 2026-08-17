#pragma once

#include "game/GameState.h"
#include "world/OverworldGrid.h"
#include "world/World.h"
#include "world/ZoneCatalog.h"

#include <string>

namespace game {

// Owns the render -> read-key -> update cycle. Movement is dispatched
// directly from render::Key -- there is no verb/command parser (see
// docs/ARCHITECTURE.md). As of Milestone 3, the loop has two modes
// (game::Mode) -- overworld and zone (interior) -- and routes movement/look
// to whichever is active; GameState.mode is the single source of truth for
// which one that is.
class GameLoop {
public:
    GameLoop(const world::World& world, const world::OverworldGrid& grid,
              const world::ZoneCatalog& zones, GameState initialState);

    void run();

private:
    void tryMoveOverworld(int dx, int dy);
    void tryMoveZone(int dx, int dy);
    void lookOverworld();
    void lookZone();
    void handleEnter();

    const world::World& world_;
    const world::OverworldGrid& grid_;
    const world::ZoneCatalog& zones_;
    GameState state_;
    std::string message_; // transient, shown for one frame then cleared
};

} // namespace game
