cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW) # must be set before project() for CMAKE_MSVC_RUNTIME_LIBRARY below to take effect
project(AnsalonRPG CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # avoid MSVC-only extensions, so a future GCC/Clang build has nothing to trip over

# Statically link the MSVC runtime so a distributed exe (see
# tools/package_release.ps1) doesn't require the recipient to separately
# install the Visual C++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(ansalon_rpg
    src/main.cpp
    src/world/World.cpp
    src/world/WorldLoader.cpp
    src/world/Terrain.cpp
    src/world/OverworldGrid.cpp
    src/world/ZoneTile.cpp
    src/world/Zone.cpp
    src/world/ZoneLoader.cpp
    src/world/ZoneCatalog.cpp
    src/timeline/Timeline.cpp
    src/timeline/TimelineLoader.cpp
    src/quest/Quest.cpp
    src/quest/QuestLoader.cpp
    src/combat/Monster.cpp
    src/combat/MonsterLoader.cpp
    src/combat/Combat.cpp
    src/combat/CombatGrid.cpp
    src/character/Dice.cpp
    src/character/Ability.cpp
    src/character/Race.cpp
    src/character/CharClass.cpp
    src/character/Alignment.cpp
    src/character/Knighthood.cpp
    src/character/WizardOrder.cpp
    src/character/Leveling.cpp
    src/character/Spellcasting.cpp
    src/character/Equipment.cpp
    src/character/Companion.cpp
    src/character/CharacterCreator.cpp
    src/render/Console.cpp
    src/render/MapRenderer.cpp
    src/game/GameLoop.cpp
    src/game/SaveGame.cpp
)
# Source files are listed explicitly rather than globbed on purpose: adding a
# new .cpp is then a visible, reviewable line in this file instead of a
# silent pickup (and avoids CMake's well-known stale-glob-cache surprises).

target_include_directories(ansalon_rpg PRIVATE src)

if (MSVC)
    target_compile_options(ansalon_rpg PRIVATE /W4 /permissive-)
else()
    target_compile_options(ansalon_rpg PRIVATE -Wall -Wextra)
endif()
# Both branches are written now even though only MSVC is in use today, so
# this file is already ready for a future GCC/Clang build.

# The game loads data/locations.txt, data/overworld.grid,
# data/overworld_regions.grid, data/zones/*.txt, data/timeline.txt,
# data/monsters.txt, and data/quests.txt at runtime, plus
# up to 3 save-slot files (save1.txt/save2.txt/save3.txt -- see
# docs/ARCHITECTURE.md's "Save/load").
# main.cpp resolves both primarily via render::Console::executableDirectory()
# (GetModuleFileName on Windows, the one platform-specific API this needs --
# kept inside render/Console.cpp per CLAUDE.md) so a distributed build (see
# tools/package_release.ps1) finds its own data next to itself wherever it's
# unzipped. The definitions below are only the fallback for when that can't
# be determined (non-Windows, or editor tooling that skips CMake
# configuration) -- see docs/GOTCHAS.md.
target_compile_definitions(ansalon_rpg PRIVATE
    ANSALON_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
    ANSALON_SAVE_FILE_BASE="${CMAKE_SOURCE_DIR}/save"
)

# Keeps the dev workflow (running straight from build\<Config>\) working:
# copies data/ next to the built exe after every build, in every config, so
# the executable-relative resolution above finds it without a separate
# install/package step during normal development.
add_custom_command(TARGET ansalon_rpg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:ansalon_rpg>/data"
)

# ---------------------------------------------------------------------------
# ansalon_sfml_trial -- round 3 de-risking spike (see docs/CURRENT_WORK.md
# and the sprites-deferred history it links to): an isolated, throwaway
# target proving the real map image (References/dragonlancemap2.png, used
# with the artist's permission -- see docs/MAP_NOTES.md) can be rendered
# directly as the overworld, with a scrolling camera and location markers,
# instead of a hand-drawn tile atlas. Purely additive -- never touches the
# ansalon_rpg target above. Reuses the real world::OverworldGrid/World/
# WorldLoader loading code directly rather than reimplementing it. Reads
# References/dragonlancemap2.png and data/*.txt straight from the source
# tree at run time (no post-build copy step -- never intended to leave this
# repo).
include(FetchContent)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
    GIT_SHALLOW ON
)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

add_executable(ansalon_sfml_trial
    sfml_trial/main.cpp
    src/world/OverworldGrid.cpp
    src/world/World.cpp
    src/world/WorldLoader.cpp
    src/world/Terrain.cpp
)
target_include_directories(ansalon_sfml_trial PRIVATE src)
target_compile_features(ansalon_sfml_trial PRIVATE cxx_std_17)
target_link_libraries(ansalon_sfml_trial PRIVATE SFML::Graphics SFML::Window SFML::System)
if (MSVC)
    target_compile_options(ansalon_sfml_trial PRIVATE /W4 /permissive-)
else()
    target_compile_options(ansalon_sfml_trial PRIVATE -Wall -Wextra)
endif()
