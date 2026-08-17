#include "character/CharacterCreator.h"
#include "game/GameLoop.h"
#include "game/GameState.h"
#include "render/Console.h"
#include "world/OverworldGrid.h"
#include "world/World.h"
#include "world/WorldLoader.h"
#include "world/ZoneCatalog.h"

#include <iostream>

#ifndef ANSALON_DATA_DIR
// CMakeLists.txt always supplies this via target_compile_definitions. The
// fallback here only exists so editor/IDE tooling that parses this file
// without running through CMake configuration still sees something valid.
#define ANSALON_DATA_DIR "data"
#endif

namespace {
constexpr const char* kStartingLocationId = "solace";
} // namespace

int main() {
    // Console's constructor enables ANSI/VT100 escape processing for the
    // lifetime of this scope (Windows only -- see render/Console.cpp) and
    // restores the original console mode when it goes out of scope at the
    // end of main().
    render::Console console;

    try {
        world::OverworldGrid grid =
            world::OverworldGrid::loadFromFile(std::string(ANSALON_DATA_DIR) + "/overworld.grid");

        world::World world;
        world::WorldLoader::loadFromFile(std::string(ANSALON_DATA_DIR) + "/locations.txt", world);

        // Zones are loaded after World, deliberately: ZoneCatalog matches
        // zone files to locations by id, so it needs the location list
        // first -- see docs/ARCHITECTURE.md.
        world::ZoneCatalog zones =
            world::ZoneCatalog::loadForWorld(world, std::string(ANSALON_DATA_DIR) + "/zones");

        const world::Location* start = world.getLocation(kStartingLocationId);
        if (!start) {
            std::cerr << "World data does not define the starting location '" << kStartingLocationId << "'.\n";
            return 1;
        }
        // Checked here, not in WorldLoader: WorldLoader has no knowledge of
        // OverworldGrid (deliberately decoupled -- see
        // docs/ARCHITECTURE.md), so grid-bounds validation for location
        // coordinates happens at this, the one place both are loaded together.
        if (start->x < 0 || start->y < 0 || start->x >= grid.width() || start->y >= grid.height()) {
            std::cerr << "Starting location '" << kStartingLocationId << "' is outside the overworld grid.\n";
            return 1;
        }

        // Character creation runs here, before the world is ever rendered:
        // it's a deliberate step-by-step wizard on plain std::cin/std::cout,
        // not part of the real-time keypress loop -- see
        // docs/ARCHITECTURE.md and character/CharacterCreator.h.
        game::GameState state;
        state.character = character::CharacterCreator::run();
        state.x = start->x;
        state.y = start->y;
        state.visitedLocations.insert(start->id);

        game::GameLoop loop(world, grid, zones, std::move(state));
        loop.run();
    } catch (const std::exception& ex) {
        std::cerr << "Failed to start: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
