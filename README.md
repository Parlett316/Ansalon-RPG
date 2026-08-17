# Ansalon: Age of Despair

A personal, non-commercial fan project: an ASCII, terminal-based RPG set on
the world of Krynn (Dragonlance) during the War of the Lance, using 2nd
Edition AD&D rules, presented Caves-of-Qud style — a colored ASCII overworld
you walk across in real time. The eventual goal is an open world where the
canon Heroes of the Lance (Tanis, Sturm, Raistlin, Caramon, Goldmoon,
Riverwind, Tasslehoff, Flint...) move through their real novel-timeline
locations, so the player can stumble into a "chance encounter" with them if
they happen to be in the same place at the same in-game time.

This is a fan project built for fun, not for profit, and is not affiliated
with or endorsed by Wizards of the Coast / the Dragonlance IP holders.

## Status

**Milestone 4: 2nd Edition AD&D character creation.** Starting the game now
begins with an interactive text-prompt character creation wizard (roll
3d6-down-the-line ability scores, reroll as many times as you like, pick a
race, class, and alignment) before dropping you into the world with a real
character behind the `@`. The whole continent is still a 480×320 tile grid,
generated from the reference map image, walked tile-by-tile in real time;
named locations (Solace, Tarsis, Xak Tsaroth, ...) sit on that grid,
connected by roads baked into the terrain, and Solace has a walkable
interior (Enter to step in). No combat or canon-encounter engine yet — see
`docs/ARCHITECTURE.md`'s "extension points" for how those are meant to
attach later.

If you're picking this project up fresh (human or AI), read
`docs/ARCHITECTURE.md` (why the code is shaped the way it is),
`docs/GOTCHAS.md` (non-obvious traps already hit and worked around),
`docs/MAP_NOTES.md` (how the overworld map data was generated),
`docs/ZONE_NOTES.md` (how walkable interiors are authored), and
`docs/CHARACTER_NOTES.md` (which 2e rules are modeled, which are
deliberately deferred, and an important accuracy caveat) before making
changes.

## Requirements

- CMake 3.20+
- A C++17 compiler. Developed against MSVC (Visual Studio Build Tools 2026).
  The code avoids MSVC-only extensions (`CMAKE_CXX_EXTENSIONS OFF`, no
  MSVC-only `_s` functions) so a future GCC/Clang build should need no
  source changes to the game itself — untested so far, but that's the
  intent. (Raw single-keypress input currently only has a Windows
  `<conio.h>` implementation; see `docs/ARCHITECTURE.md`.)
- Python 3 + Pillow (`pip install pillow`) — **only** if you need to
  regenerate `data/overworld.grid` via `tools/generate_overworld.py`. The
  built game itself has no Python/Pillow dependency at runtime.

## Building (Windows / MSVC)

From a plain PowerShell (no need to manually run `vcvarsall.bat` — the
Visual Studio CMake generator locates MSVC itself):

```powershell
cmake -G "Visual Studio 18 2026" -A x64 -S . -B build
cmake --build build --config Debug
```

The built executable will be at `build\Debug\ansalon_rpg.exe`. Run it from
any directory — it locates `data/locations.txt` and `data/overworld.grid`
via an absolute path baked in at compile time (see the comment in
`CMakeLists.txt` for why, and its limits).

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

Run the built `ansalon_rpg.exe`. It opens with character creation (plain
typed prompts — enter a name, keep or reroll your ability scores, pick a
race/class/alignment number, confirm). Once that's done, movement is
immediate — no Enter key needed:

- **Move**: arrow keys, or `hjkl` / `wasd` for the 4 cardinal directions,
  or `y u b n` for the 4 diagonals (vi/roguelike convention: `y`=NW, `u`=NE,
  `b`=SW, `n`=SE)
- **Enter** — step into a location's walkable interior (only works where
  one exists — currently just Solace), or step back out if you're standing
  on the `>` marker inside one
- `c` — view your character sheet (any key dismisses it)
- `;` — look around (overworld: names the nearest notable place and its
  direction; inside a zone: everything is already on screen, so there's
  nothing further to reveal)
- `q` or Esc — quit

Walking into impassable terrain (open ocean, the Blood Sea, walls, trees) is
blocked with a message. Standing exactly on a named location or a point of
interest shows its description. Requires a terminal at least 78 columns ×
~24 rows (the viewport is a fixed 78×20 plus a few status lines — dynamic
resizing isn't handled yet).

## Regenerating the map

`data/overworld.grid` is generated from the reference map image by
`tools/generate_overworld.py` — see `docs/MAP_NOTES.md` for the full
process. Short version:

```powershell
pip install pillow
python tools/generate_overworld.py
```

## Project layout

```
src/world/     overworld: named places (Location, World, WorldLoader) and
               walkable terrain (Terrain, OverworldGrid); interiors:
               ZoneTile, Zone, ZoneLoader, ZoneCatalog
src/character/ 2e AD&D rules content (Race, CharClass, Ability, Alignment,
               Dice) and the interactive CharacterCreator wizard
src/render/    ASCII presentation + raw keyboard input (Console, MapRenderer)
src/game/      orchestration (GameState, GameLoop) and main.cpp
data/          locations.txt + zones/*.txt (hand-authored),
               overworld.grid (generated)
tools/         generate_overworld.py -- offline map generator, dev-only, not
               part of the shipped game
docs/          ARCHITECTURE.md, GOTCHAS.md, MAP_NOTES.md, ZONE_NOTES.md,
               CHARACTER_NOTES.md
```

See `docs/ARCHITECTURE.md` for the reasoning behind this split and where
future systems (the War-of-the-Lance timeline/encounter engine, combat) are
meant to attach.
