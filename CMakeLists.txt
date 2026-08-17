cmake_minimum_required(VERSION 3.20)
project(AnsalonRPG CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # avoid MSVC-only extensions, so a future GCC/Clang build has nothing to trip over

add_executable(ansalon_rpg
    src/main.cpp
    src/world/World.cpp
    src/world/WorldLoader.cpp
    src/render/Console.cpp
    src/render/MapRenderer.cpp
    src/game/Command.cpp
    src/game/GameLoop.cpp
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

# The game loads its location/road data from data/locations.txt at runtime.
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
)
