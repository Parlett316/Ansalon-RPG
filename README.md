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

Starting the game begins with a saved-character continue prompt (if
`save.txt` exists) or, failing that, an interactive text-prompt character
creation wizard (roll 3d6-down-the-line ability scores, reroll as many
times as you like, pick a race — Elf and Dwarf prompt a Dragonlance
subrace — class, and alignment; qualifying Fighters can join the Knights
of Solamnia, and Gnomes are always Tinkers) before dropping you into the
world with a real character behind the `@`. Progress autosaves
continuously. The whole continent is a 480×320 tile grid, generated from
the reference map image, walked tile-by-tile in real time; named locations
(Solace, Tarsis, Xak Tsaroth, ...) sit on that grid, connected by roads
baked into the terrain, and every one of them now has a walkable interior
(Enter to step in) — including the Inn of the Last Home inside Solace,
and Qualinost, the elven capital, inside Qualinesti. Standing at a location can also
reveal canon Heroes of the Lance passing through on their own schedule —
the beginnings of the "chance encounter" engine described above (all
eight Heroes of the Lance now, at six of the eight locations, including
an alternate path through Darken Wood and the climactic siege of Pax
Tharkas — see `docs/TIMELINE_NOTES.md`), and stepping inside carries the
encounter through too: find them gathered at the Inn's fireplace, Haven's
market, Xak Tsaroth's old well, Qualinost's Hall of the Sky, Darken
Wood's faded trail, or the Tharkadan mine entrance at Pax Tharkas, not
just standing on the overworld tile. You can actually talk to them (and to NPCs
inside zones, like the Inn's Otik and Tika, Haven's Seeker Guard, and Darken
Wood's Forestmaster) — press `t`, grounded in the original DL1-3 adventure
modules and, for the Heroes of the Lance, the Chronicles/Legends novels:
real reactive dialogue based on your own race/class/alignment, branching
topics to ask about, and real memory of whether you've spoken before, not
just one static line forever — carried whether you met them out in the
open or found them indoors. Traveling the wilds now risks a random encounter — goblins, kobolds,
hobgoblins, wolves, giant spiders, bugbears, ogres, or Baaz/Kapak
draconians, all sourced from a real 2e Monster Manual and, for the
Krynn-specific draconians, Dragonlance Adventures (no orcs, since Krynn
has none) — the chance of one varies by terrain, roads safest and forest/
mountains riskiest — resolved with real 2e attack/damage math (Enter to attack, `f` to flee, and a Mage or
Cleric can also `m` to cast their one known spell — Magic Missile or Cure
Light Wounds, real PHB slots-per-day by level); losing just knocks you out
and sends you back to Solace, it isn't permadeath — see
`docs/COMBAT_NOTES.md`. Winning fights earns Steel Pieces (Krynn's own
post-Cataclysm currency, not gold) and experience, and that steel now has
somewhere to go — press `p` at a shop (Solace's General Store, Haven's
Market Stalls, or Tarsis's Old Sailor — all three sell the same catalog)
to buy real armor (Leather/Chain Mail/Splint Mail) and a weapon upgrade
(Mages and Tinkers can't wear armor at all, per the PHB's own rule), or
press `i` inside the shop to switch to selling gear back for half its
price. Purchases land in a real carried inventory rather than being worn
automatically — press `i` outside a shop to see what you're carrying and
equip it, which actually changes your AC and damage in the next fight,
swapping whatever you had on back into your pack rather than losing it —
see `docs/CHARACTER_NOTES.md`'s "Equipment" section. Enough experience
means
real leveling — more hit points, a better THAC0, better saving throws, all
sourced from the PHB's level-by-level tables (a Knight of the Crown gets a
nod toward the Order of the Sword at 3rd level, and a Mage actually
undergoes the Test of High Sorcery and is Robed by alignment) — see
`docs/CHARACTER_NOTES.md`.

If you're picking this project up fresh (human or AI), read
`docs/ARCHITECTURE.md` (why the code is shaped the way it is),
`docs/GOTCHAS.md` (non-obvious traps already hit and worked around),
`docs/MAP_NOTES.md` (how the overworld map data was generated),
`docs/ZONE_NOTES.md` (how walkable interiors are authored),
`docs/CHARACTER_NOTES.md` (which 2e rules are modeled, which are
deliberately deferred, and an important accuracy caveat),
`docs/TIMELINE_NOTES.md` (how the chance-encounter schedule is authored),
and `docs/COMBAT_NOTES.md` (attack/damage math, and what's invented vs.
sourced) before making changes.

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

Run the built `ansalon_rpg.exe`. If a saved character exists (`save.txt`,
autosaved continuously during play — see `docs/ARCHITECTURE.md`), it asks
whether to continue that character before anything else. Otherwise (or if
you decline), it opens with character creation (plain typed prompts — enter
a name, keep or reroll your ability scores, pick a race/class/alignment
number, confirm). Once that's done, movement is immediate — no Enter key
needed:

- **Move**: arrow keys, or `hjkl` / `wasd` for the 4 cardinal directions,
  or `y u b n` for the 4 diagonals (vi/roguelike convention: `y`=NW, `u`=NE,
  `b`=SW, `n`=SE)
- **Enter** — step into a location's walkable interior (every location has
  one now), or step back out if you're standing on the `>` marker inside
  one
- `t` — talk to whoever's here (a canon character the timeline places at
  your current location today, or a talkable NPC inside a zone — any key
  dismisses the reply). If more than one character is present at once,
  `t` first asks who — up/down selects, Enter talks, `q`/Esc cancels.
  Characters remember whether you've spoken before: the first
  conversation is their real line, every one after is a real second line
  where one's been written (all 8 Heroes of the Lance and every zone NPC
  have one), or a quick nod of recognition otherwise. At Solace, the 8
  Heroes may also react differently depending on your character's own
  race/class/alignment, and offer a follow-up topic to ask about — up/
  down selects, Enter asks, `q`/Esc leaves the conversation.
- `p` — browse/buy at a shop POI (Solace's General Store, Haven's Market
  Stalls, or Tarsis's Old Sailor) — up/down selects an item, Enter buys
  it, `i` switches to selling gear from your inventory back for half its
  price, `q`/Esc leaves the shop. Purchases go to your carried inventory,
  not straight onto your body.
- `i` — outside a shop: view your carried items and equip one — up/down
  selects, Enter equips (swapping in whatever you were wearing before),
  `q`/Esc leaves
- `c` — view your character sheet (any key dismisses it)
- `;` — look around (overworld: names the nearest notable place and its
  direction; inside a zone: everything is already on screen, so there's
  nothing further to reveal)
- `q` or Esc — quit

Walking into impassable terrain (open ocean, the Blood Sea, walls, trees) is
blocked with a message. Standing exactly on a named location or a point of
interest shows its description — and, if a canon character's schedule
places them there on the current in-game day, a line describing them too.
Traveling away from named locations carries a chance of a random encounter,
which takes over the screen: **Enter** attacks, **`m`** casts your one
known spell if you're a Mage or Cleric (and you have a spell slot left
today), **`f`** flees. Losing a fight knocks you out (HP capped at 1) and
sends you back to Solace rather than ending the run. Requires a terminal at least 78 columns × ~24 rows
(the viewport is a fixed 78×20 plus a few status lines — dynamic resizing
isn't handled yet).

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
src/timeline/  canon character schedule for chance encounters (Timeline,
               TimelineLoader)
src/character/ 2e AD&D rules content (Race, CharClass, Ability, Alignment,
               Knighthood, WizardOrder, Leveling, Spellcasting, Equipment,
               Dice) and the interactive CharacterCreator wizard
src/combat/    monster roster + attack/damage/initiative math (Monster,
               MonsterCatalog, MonsterLoader, Combat)
src/render/    ASCII presentation + raw keyboard input (Console, MapRenderer)
src/game/      orchestration (GameState, GameLoop, SaveGame) and main.cpp
data/          locations.txt + zones/*.txt + timeline.txt + monsters.txt
               (hand-authored), overworld.grid (generated)
tools/         generate_overworld.py -- offline map generator, dev-only, not
               part of the shipped game
docs/          ARCHITECTURE.md, GOTCHAS.md, MAP_NOTES.md, ZONE_NOTES.md,
               CHARACTER_NOTES.md
```

See `docs/ARCHITECTURE.md` for the reasoning behind this split and where
future systems (the War-of-the-Lance timeline/encounter engine, combat) are
meant to attach.
