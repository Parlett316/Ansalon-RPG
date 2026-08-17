#include "game/GameLoop.h"
#include "render/Console.h"
#include "render/MapRenderer.h"
#include "world/Terrain.h"

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

GameLoop::GameLoop(const world::World& world, const world::OverworldGrid& grid, GameState initialState)
    : world_(world), grid_(grid), state_(std::move(initialState)) {}

void GameLoop::run() {
    for (;;) {
        render::MapRenderer::drawFrame(grid_, world_, state_, message_);
        message_.clear();

        switch (render::Console::readKey()) {
            case render::Key::North:     tryMove(0, -1); break;
            case render::Key::South:     tryMove(0, 1);  break;
            case render::Key::East:      tryMove(1, 0);  break;
            case render::Key::West:      tryMove(-1, 0); break;
            case render::Key::NorthEast: tryMove(1, -1); break;
            case render::Key::NorthWest: tryMove(-1, -1); break;
            case render::Key::SouthEast: tryMove(1, 1);  break;
            case render::Key::SouthWest: tryMove(-1, 1); break;
            case render::Key::Look:      look(); break;
            case render::Key::Quit:      return;
            case render::Key::Unknown:   break;
        }
    }
}

void GameLoop::tryMove(int dx, int dy) {
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

void GameLoop::look() {
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

} // namespace game
