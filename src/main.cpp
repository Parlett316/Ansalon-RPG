#include "character/CharacterCreator.h"
#include "character/CharClass.h"
#include "character/Race.h"
#include "combat/Monster.h"
#include "combat/MonsterLoader.h"
#include "game/GameLoop.h"
#include "game/GameState.h"
#include "game/SaveGame.h"
#include "render/Console.h"
#include "render/MapRenderer.h"
#include "timeline/Timeline.h"
#include "timeline/TimelineLoader.h"
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

#ifndef ANSALON_SAVE_FILE
// Same reasoning/fallback as ANSALON_DATA_DIR above.
#define ANSALON_SAVE_FILE "save.txt"
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

    // Sizes MapRenderer's layout to the real, currently visible console
    // window (not the scrollback buffer -- see Console::currentWindowSize
    // and docs/GOTCHAS.md) before anything renders. Must happen before any
    // GameLoop/MapRenderer usage below. Same "fail fast with a clear
    // message" convention as the starting-location checks further down --
    // not a new pattern.
    render::WindowSize windowSize = render::Console::currentWindowSize();
    if (!render::MapRenderer::configureLayout(windowSize.columns, windowSize.rows)) {
        std::cerr << "Terminal too small: detected " << windowSize.columns << "x" << windowSize.rows
                   << ", need at least " << render::MapRenderer::kAbsoluteMinColumns << "x"
                   << render::MapRenderer::kAbsoluteMinRows << ". Enlarge your terminal window and try again.\n";
        return 1;
    }

    // Resolved relative to the running executable so a distributed build
    // (see tools/package_release.ps1) finds its own data/ and save.txt
    // wherever it's unzipped, instead of the compile-time source-tree path
    // baked in below. Falls back to the compile-time ANSALON_DATA_DIR /
    // ANSALON_SAVE_FILE only if the executable's own path can't be
    // determined (non-Windows, or editor tooling that skips CMake
    // configuration) -- see docs/GOTCHAS.md.
    const std::string exeDir = render::Console::executableDirectory();
    const std::string dataDir = exeDir.empty() ? ANSALON_DATA_DIR : exeDir + "/data";
    const std::string savePath = exeDir.empty() ? ANSALON_SAVE_FILE : exeDir + "/save.txt";

    try {
        world::OverworldGrid grid =
            world::OverworldGrid::loadFromFile(dataDir + "/overworld.grid");

        world::World world;
        world::WorldLoader::loadFromFile(dataDir + "/locations.txt", world);

        // Zones are loaded after World, deliberately: ZoneCatalog matches
        // zone files to locations by id, so it needs the location list
        // first -- see docs/ARCHITECTURE.md.
        world::ZoneCatalog zones =
            world::ZoneCatalog::loadForWorld(world, dataDir + "/zones");

        // Static content, same treatment as World/OverworldGrid/ZoneCatalog
        // above -- loaded fresh every run, never touched by GameState/save
        // data. See timeline::Timeline and docs/TIMELINE_NOTES.md.
        timeline::Timeline timeline;
        timeline::TimelineLoader::loadFromFile(dataDir + "/timeline.txt", timeline);

        // Static content too, same treatment -- see combat::MonsterCatalog
        // and docs/COMBAT_NOTES.md.
        combat::MonsterCatalog monsters;
        combat::MonsterLoader::loadFromFile(dataDir + "/monsters.txt", monsters);

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

        // If a save exists, offer to continue it -- plain std::cin/std::cout,
        // same "before GameLoop's raw-keypress world starts" interaction mode
        // as CharacterCreator (see docs/ARCHITECTURE.md). Declining, or no
        // save existing, falls through to the ordinary CharacterCreator flow
        // unchanged.
        game::GameState state;
        bool loadedSave = false;
        if (game::SaveGame::exists(savePath)) {
            game::GameState loaded = game::SaveGame::load(savePath);
            if (loaded.mode == game::Mode::Zone && !zones.hasZone(loaded.currentZoneId)) {
                std::cerr << "Save file references a zone ('" << loaded.currentZoneId
                          << "') that no longer exists. Delete " << savePath
                          << " to start fresh.\n";
                return 1;
            }
            const character::Character& c = loaded.character;
            std::cout << "\nA saved character was found: " << c.name << ", level " << c.level
                       << " " << character::raceInfo(c.race).name << " "
                       << character::classInfo(c.charClass).name << " (Day "
                       << (loaded.hoursElapsed / 24) << ").\n";
            std::cout << "Continue this character? (y/n) ";
            std::string answer;
            std::getline(std::cin, answer);
            if (!answer.empty() && (answer[0] == 'y' || answer[0] == 'Y')) {
                state = std::move(loaded);
                loadedSave = true;
            }
        }

        // Character creation runs here, before the world is ever rendered:
        // it's a deliberate step-by-step wizard on plain std::cin/std::cout,
        // not part of the real-time keypress loop -- see
        // docs/ARCHITECTURE.md and character/CharacterCreator.h.
        if (!loadedSave) {
            state.character = character::CharacterCreator::run();
            state.x = start->x;
            state.y = start->y;
            state.visitedLocations.insert(start->id);
        }

        game::GameLoop loop(world, grid, zones, timeline, monsters, std::move(state), savePath);
        loop.run();
    } catch (const std::exception& ex) {
        std::cerr << "Failed to start: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
