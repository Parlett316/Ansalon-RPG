# Architecture

This document explains *why* the code is organized the way it is, so a
future contributor (human or AI) doesn't have to reverse-engineer intent
from the file layout alone. If you're extending this project, read this
first — especially the "extension points" section, which exists precisely
so you don't have to restructure existing code to add the next milestone.

## Module map

```
world/   Location, World, WorldLoader   -- the map data model
render/  Console, MapRenderer           -- ASCII presentation
game/    GameState, Command, GameLoop   -- orchestration / the actual game
main.cpp                                -- wires the above together
```

The dependency direction is one-way: `game/` depends on `world/` and
`render/`; `render/` depends on `world/` (it needs to read Location data to
draw it) and on `game/GameState` (it needs to know where the player is) but
never mutates either; `world/` depends on nothing else in the project. This
means `world/` and `render/` can each be understood, tested, or reused in
isolation without pulling in the whole game loop.

## Why `world/` doesn't know about the player

`World` and `Location` model the map itself — names, descriptions, roads —
with no concept of "current location" or "day count" anywhere in them.
Player-specific state lives entirely in `game::GameState`. This split
matters because it's not just `GameLoop` that will eventually need to ask
"where is the player, and what's around them" — a future timeline/encounter
engine will need to ask the same question about *canon characters* (e.g.
"is Tanis's party in Tarsis on day 14?"), and it should be able to query the
same `World` without needing a player to exist at all. If location data and
player position were tangled together in one class, that reuse wouldn't be
possible without a rewrite.

## Why `Console` is the only platform-specific file

All Windows API usage (`<windows.h>`, `SetConsoleMode`,
`ENABLE_VIRTUAL_TERMINAL_PROCESSING`) is isolated inside
`render/Console.cpp`, guarded by `#ifdef _WIN32`. Every other file is
portable standard C++17. The reasoning: real terminals on Linux/macOS
already interpret ANSI escape codes without any setup, so a future port to
those platforms should only ever need an alternate (likely much simpler, or
even empty) implementation of `Console` — never a change to `World`,
`MapRenderer`, or `GameLoop`.

## Why location/road data is a hand-rolled text format, not JSON

`WorldLoader` parses a small custom block format (grammar in
`MAP_NOTES.md`) instead of using a JSON library. This was a deliberate
tradeoff, made with the project owner: no external dependency to vendor or
fetch (no package manager is set up for this project), a parser small
enough (`WorldLoader.cpp`) that a future maintainer can read the entire
thing in a couple of minutes, and a data format readable/editable without
knowing any particular library's conventions. The cost is that the format
is bespoke — if the project ever wants richer data (nested structures,
arrays of arrays, etc.), revisit this and consider vendoring a header-only
JSON library like `nlohmann/json` instead of extending the custom parser
indefinitely.

## Why the schematic map is a hand-placed graph, not a tile grid

Ansalon (per the reference map) is enormous and detailed. A tile-accurate
biome grid covering the whole continent is a much bigger effort than
"chance encounter" gameplay actually needs — that gameplay only cares
*which named location* the player and a canon character are at on *which
day*, not what's between them. So the world is a **graph**: named locations
connected by roads with travel times, each location given a stylized
`(row, col)` position for the ASCII schematic view. This keeps map-authoring
proportional to how many named places matter for the story, and defers a
true tile-based continent map to a possible future stretch goal — it is not
planned for the near term.

## Extension points for later milestones

These are the seams intentionally left in milestone 1's code so later
systems can attach without reworking it:

- **Character creation** (2nd Ed. AD&D rules): add a `Character` type and a
  `Character* character` (or similar) field to `game::GameState`. Nothing
  in `world/` or `render/` needs to change; `GameLoop`/`Command` gain new
  verbs (`stats`, `inventory`, ...).
- **War-of-the-Lance timeline / chance-encounter engine**: `GameState`
  already tracks `currentLocationId` and `dayCount` — a future
  `timeline::Timeline` (or similar) can be queried each time the player
  arrives at a location (`GameLoop::handleGo`, after the state update) with
  "who else is here on this day?" using the same `region`/`id` fields
  `Location` already carries. `Location::region` exists today specifically
  so region-scoped canon events have something to match against later, even
  though nothing reads it yet.
- **Combat**: would plug into `GameLoop` as a new mode/state entered when an
  encounter triggers, reusing `Console`/rendering primitives already in
  place.
- **Save/load**: `GameState` is a small, flat, serializable struct on
  purpose — a save format only needs to round-trip that struct (plus,
  later, `Character`), not touch `World` (which is static, loaded fresh
  from `data/locations.txt` every run).

## What's deliberately NOT abstracted yet

Following a "don't build for hypothetical requirements" principle: there's
no plugin system, no generic "event" bus, no data-driven scripting layer.
Those would be premature — the timeline/combat/character systems don't
exist yet, so there's nothing concrete to generalize from. When those
milestones are actually built, revisit this document and update it to
describe what shipped, not just what was planned.
