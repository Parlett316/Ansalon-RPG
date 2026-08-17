#include "game/GameLoop.h"
#include "render/Console.h"
#include "render/MapRenderer.h"
#include "world/Terrain.h"
#include "world/ZoneTile.h"

#include <cmath>
#include <sstream>

namespace game {

namespace {

// Grid y grows downward (row 0 is the top), so "north" is negative dy --
// easy to get backwards, worth calling out.
const char* compassDirection(int dx, int dy) {
    constexpr double kPi = 3.14159265358979323846;
    double angle = std::atan2(static_cast<double>(-dy), static_cast<double>(dx)); // 0 = east, increases counter-clockwise
    static const char* kDirs[8] = {"east", "northeast", "north", "northwest",
                                    "west", "southwest", "south", "southeast"};
    int index = static_cast<int>(std::lround(angle / (kPi / 4.0))) & 7;
    return kDirs[index];
}

} // namespace

GameLoop::GameLoop(const world::World& world, const world::OverworldGrid& grid,
                    const world::ZoneCatalog& zones, GameState initialState)
    : world_(world), grid_(grid), zones_(zones), state_(std::move(initialState)) {}

void GameLoop::run() {
    for (;;) {
        if (state_.mode == Mode::Overworld) {
            render::MapRenderer::drawOverworldFrame(grid_, world_, state_, message_);
        } else {
            const world::Zone* zone = zones_.getZone(state_.currentZoneId);
            render::MapRenderer::drawZoneFrame(*zone, state_, message_);
        }
        message_.clear();

        render::Key key = render::Console::readKey();
        bool inZone = state_.mode == Mode::Zone;

        switch (key) {
            case render::Key::North:     inZone ? tryMoveZone(0, -1) : tryMoveOverworld(0, -1); break;
            case render::Key::South:     inZone ? tryMoveZone(0, 1)  : tryMoveOverworld(0, 1);  break;
            case render::Key::East:      inZone ? tryMoveZone(1, 0)  : tryMoveOverworld(1, 0);  break;
            case render::Key::West:      inZone ? tryMoveZone(-1, 0) : tryMoveOverworld(-1, 0); break;
            case render::Key::NorthEast: inZone ? tryMoveZone(1, -1) : tryMoveOverworld(1, -1); break;
            case render::Key::NorthWest: inZone ? tryMoveZone(-1, -1): tryMoveOverworld(-1, -1);break;
            case render::Key::SouthEast: inZone ? tryMoveZone(1, 1)  : tryMoveOverworld(1, 1);  break;
            case render::Key::SouthWest: inZone ? tryMoveZone(-1, 1) : tryMoveOverworld(-1, 1); break;
            case render::Key::Look:      inZone ? lookZone() : lookOverworld(); break;
            case render::Key::Enter:     handleEnter(); break;
            case render::Key::Quit:      return;
            case render::Key::Unknown:   break;
        }
    }
}

void GameLoop::tryMoveOverworld(int dx, int dy) {
    int nx = state_.x + dx;
    int ny = state_.y + dy;
    const world::TerrainInfo& terrain = world::terrainFor(grid_.terrainCodeAt(nx, ny));
    if (!terrain.passable) {
        message_ = "You cannot cross " + std::string(terrain.name) + " on foot.";
        return;
    }
    state_.x = nx;
    state_.y = ny;
    state_.hoursElapsed += terrain.hoursToCross;
    if (const world::Location* here = world_.locationAt(nx, ny)) {
        state_.visitedLocations.insert(here->id);
    }
}

void GameLoop::tryMoveZone(int dx, int dy) {
    const world::Zone* zone = zones_.getZone(state_.currentZoneId);
    int nx = state_.zoneX + dx;
    int ny = state_.zoneY + dy;
    const world::ZoneTileInfo& tile = world::zoneTileFor(zone->tileCodeAt(nx, ny));
    if (!tile.passable) {
        message_ = "You can't walk through " + std::string(tile.name) + ".";
        return;
    }
    // Deliberately does not touch hoursElapsed -- indoor shuffling isn't
    // meaningful travel time; only overworld movement advances the clock.
    state_.zoneX = nx;
    state_.zoneY = ny;
}

void GameLoop::lookOverworld() {
    const world::Location* nearest = nullptr;
    int nearestDistSq = 0;
    for (const auto& loc : world_.allLocations()) {
        if (loc.x == state_.x && loc.y == state_.y) continue; // already described by the status line
        int dx = loc.x - state_.x;
        int dy = loc.y - state_.y;
        int distSq = dx * dx + dy * dy;
        if (nearest == nullptr || distSq < nearestDistSq) {
            nearest = &loc;
            nearestDistSq = distSq;
        }
    }
    if (nearest == nullptr) {
        message_ = "Nothing notable stands out on the horizon.";
        return;
    }
    const char* dir = compassDirection(nearest->x - state_.x, nearest->y - state_.y);
    std::ostringstream oss;
    oss << "You reckon " << nearest->name << " lies to the " << dir << ".";
    message_ = oss.str();
}

void GameLoop::lookZone() {
    // Deliberately minimal: unlike the overworld (a scrolling camera that
    // hides everything outside the viewport, so "look around" has real work
    // to do finding the nearest hidden landmark), a zone is always rendered
    // in full -- every POI is already visible on screen. There's nothing
    // for a "look" action to reveal that isn't already there.
    message_ = "Nothing else catches your eye here.";
}

void GameLoop::handleEnter() {
    if (state_.mode == Mode::Overworld) {
        const world::Location* here = world_.locationAt(state_.x, state_.y);
        if (here == nullptr) {
            message_ = "There is nothing here to step into.";
            return;
        }
        const world::Zone* zone = zones_.getZone(here->id);
        if (zone == nullptr) {
            message_ = "There is nothing to explore inside " + here->name + " yet.";
            return;
        }
        state_.mode = Mode::Zone;
        state_.currentZoneId = here->id;
        state_.zoneX = zone->entryX();
        state_.zoneY = zone->entryY();
        message_ = "You step into " + here->name + ".";
        return;
    }

    // Mode::Zone
    const world::Zone* zone = zones_.getZone(state_.currentZoneId);
    if (state_.zoneX != zone->entryX() || state_.zoneY != zone->entryY()) {
        message_ = "You need to be at the entrance (marked '>') to leave.";
        return;
    }
    const world::Location* here = world_.getLocation(state_.currentZoneId);
    message_ = "You step back out into " + (here != nullptr ? here->name : "the world") + ".";
    state_.mode = Mode::Overworld;
    state_.currentZoneId.clear();
}

} // namespace game
