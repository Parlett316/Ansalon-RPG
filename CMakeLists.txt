cmake_minimum_required(VERSION 3.20)
project(AnsalonRPG CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # avoid MSVC-only extensions, so a future GCC/Clang build has nothing to trip over

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
    src/combat/Monster.cpp
    src/combat/MonsterLoader.cpp
    src/combat/Combat.cpp
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

# The game loads data/locations.txt, data/overworld.grid, data/zones/*.txt,
# data/timeline.txt, and data/monsters.txt at runtime.
# Resolving that path relative to the *running executable* would need
# platform-specific APIs (GetModuleFileName on Windows, /proc/self/exe on
# Linux) that would leak platform code outside render/Console.cpp. Instead,
# for this solo/dev-only project, we bake the absolute source-tree data
# directory in at compile time: the built exe then finds its data regardless
# of the caller's working directory. This is NOT how a distributed/installed
# build should locate data -- revisit if this project is ever packaged for
# someone else to run outside this exact source tree.
target_compile_definitions(ansalon_rpg PRIVATE
    ANSALON_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
    # Same reasoning/limitation as ANSALON_DATA_DIR above -- a single,
    # baked-path savegame file, not relative to the executable. See
    # docs/GOTCHAS.md.
    ANSALON_SAVE_FILE="${CMAKE_SOURCE_DIR}/save.txt"
)
