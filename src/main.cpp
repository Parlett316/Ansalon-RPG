#include "game/GameLoop.h"
#include "game/GameState.h"
#include "render/Console.h"
#include "world/World.h"
#include "world/WorldLoader.h"

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

    world::World world;
    try {
        world::WorldLoader::loadFromFile(std::string(ANSALON_DATA_DIR) + "/locations.txt", world);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to load world data: " << ex.what() << "\n";
        return 1;
    }

    if (!world.getLocation(kStartingLocationId)) {
        std::cerr << "World data does not define the starting location '" << kStartingLocationId << "'.\n";
        return 1;
    }

    game::GameState state;
    state.currentLocationId = kStartingLocationId;
    state.visitedLocations.insert(kStartingLocationId);

    render::Console::clearScreen();
    std::cout << "=== Ansalon: Age of Despair ===\n";
    std::cout << "A War of the Lance chronicle.\n";

    game::GameLoop loop(world, std::move(state));
    loop.run();

    return 0;
}
