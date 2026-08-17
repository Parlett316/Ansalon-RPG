# Ansalon: Age of Despair

A personal, non-commercial fan project: an ASCII, terminal-based RPG set on
the world of Krynn (Dragonlance) during the War of the Lance, using 2nd
Edition AD&D rules. The eventual goal is an open world where the canon
Heroes of the Lance (Tanis, Sturm, Raistlin, Caramon, Goldmoon, Riverwind,
Tasslehoff, Flint...) move through their real novel-timeline locations, so
the player can stumble into a "chance encounter" with them if they happen to
be in the same place at the same in-game time.

This is a fan project built for fun, not for profit, and is not affiliated
with or endorsed by Wizards of the Coast / the Dragonlance IP holders.

## Status

**Milestone 1: world map + travel.** Playable slice covering the opening
route of *Dragons of Autumn Twilight*: Solace → Darken Wood → (branching
toward Qualinesti/Pax Tharkas or Haven/Xak Tsaroth) → Plains of Dust →
Tarsis. No character system, combat, or canon-encounter engine yet — see
`docs/ARCHITECTURE.md` for how those are meant to attach later.

If you're picking this project up fresh (human or AI), also read
`docs/ARCHITECTURE.md` (why the code is shaped the way it is) and
`docs/GOTCHAS.md` (non-obvious traps already hit and worked around) before
making changes.

## Requirements

- CMake 3.20+
- A C++17 compiler. Developed against MSVC (Visual Studio Build Tools 2026).
  The code avoids MSVC-only extensions (`CMAKE_CXX_EXTENSIONS OFF`, no
  MSVC-only `_s` functions) so a future GCC/Clang build should need no
  source changes — untested so far, but that's the intent.

## Building (Windows / MSVC)

From a plain PowerShell (no need to manually run `vcvarsall.bat` — the
Visual Studio CMake generator locates MSVC itself):

```powershell
cmake -G "Visual Studio 18 2026" -A x64 -S . -B build
cmake --build build --config Debug
```

The built executable will be at `build\Debug\ansalon_rpg.exe`. Run it from
any directory — it locates `data/locations.txt` via an absolute path baked
in at compile time (see the comment in `CMakeLists.txt` for why, and its
limits).

If you have a different Visual Studio version installed, list available
generators with `cmake --help` and substitute the matching `-G` name.

### Alternative: Ninja from a Developer shell

If you prefer a single-config, faster incremental build, open a "Developer
PowerShell for VS 2026" (which pre-runs `vcvars64.bat` for you) and run:

```powershell
cmake -G Ninja -S . -B build
cmake --build build
```

## Playing

Run the built `ansalon_rpg.exe`. Commands:

- `look` — describe where you are and list roads onward
- `map` — show the schematic map of Ansalon
- `go <place>` — travel to a directly connected place, e.g. `go tarsis`
- `help` — list commands
- `quit` — exit

## Project layout

```
src/world/    the map data model (Location, World, WorldLoader)
src/render/   ASCII presentation (Console, MapRenderer)
src/game/     orchestration (GameState, Command, GameLoop) and main.cpp
data/         hand-authored location/road data (data/locations.txt)
docs/         ARCHITECTURE.md, GOTCHAS.md, MAP_NOTES.md
```

See `docs/ARCHITECTURE.md` for the reasoning behind this split and where
future systems (character creation, the War-of-the-Lance timeline/encounter
engine, combat) are meant to attach.
