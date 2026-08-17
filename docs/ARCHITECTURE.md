# Architecture

This document explains *why* the code is organized the way it is, so a
future contributor (human or AI) doesn't have to reverse-engineer intent
from the file layout alone. If you're extending this project, read this
first — especially the "extension points" section, which exists precisely
so you don't have to restructure existing code to add the next milestone.

**Milestone 2 pivot note:** Milestone 1 was a text-adventure: named
locations connected by a road graph, moved between with typed `go <place>`
commands. The user decided that didn't feel right and asked for a Caves of
Qud–style presentation instead: a persistent, colored ASCII grid walked
across in real time with immediate keypresses. This document describes the
architecture *as of that pivot* — the graph/schematic/command-parser design
is gone, not kept alongside. If you're reading old context that mentions
`Connection`, `CONNECT`, `go <place>`, or `Command.h`, that's Milestone-1
history; it no longer exists in the code.

## Module map

```
world/   Location, World, WorldLoader,     -- overworld: named places (graph)
         Terrain, OverworldGrid               + walkable terrain (grid)
         ZoneTile, Zone, ZoneLoader,        -- interiors: hand-authored walkable
         ZoneCatalog                          scenes tied to overworld locations
render/  Console, MapRenderer               -- ASCII presentation + raw input
game/    GameState, GameLoop                -- orchestration / the actual game
main.cpp                                    -- wires the above together
```

The dependency direction is one-way: `game/` depends on `world/` and
`render/`; `render/` depends on `world/` (it needs to read terrain/location
data to draw it) and on `game/GameState` (it needs to know where the player
is) but never mutates either; `world/` depends on nothing else in the
project. This means `world/` and `render/` can each be understood, tested,
or reused in isolation without pulling in the whole game loop.

## Why the world is TWO data structures, not one

`World`/`Location` (named places: Solace, Tarsis, ...) and `OverworldGrid`
(the walkable terrain grid) are separate, independently-loaded pieces that
only meet at render/movement time — `World::locationAt(x,y)` and
`OverworldGrid::terrainCodeAt(x,y)` are both just coordinate lookups into
the same coordinate space. This split exists because the two things are
authored completely differently: locations are hand-written prose (~8
today, a few dozen at most ever), while terrain is ~150,000 tiles that
*must* come from automated classification of the reference map image (see
"Why the overworld is generated, not hand-drawn" below). Keeping them
separate means the terrain grid can be regenerated at will (different tile
scale, better classification, a different source map) without touching a
single hand-authored location description, and vice versa.

## Why `world/` doesn't know about the player

Neither `World`/`Location` nor `OverworldGrid` has any concept of "current
position" or "elapsed time" — that's entirely `game::GameState`. This
matters because it's not just `GameLoop` that will eventually need to ask
"where is X, and what's around them" — a future timeline/encounter engine
will need to ask the same question about *canon characters* (e.g. "is
Tanis's party in Tarsis on day 14?"), and it should be able to query the
same `World`/`OverworldGrid` without a player needing to exist at all.

## Why `Console` is the only platform-specific file

All Windows API usage (`<windows.h>`, `<conio.h>`, `SetConsoleMode`,
`_getch`/`_kbhit`) is isolated inside `render/Console.cpp`, guarded by
`#ifdef _WIN32`. Every other file is portable standard C++17. The
reasoning: real terminals on Linux/macOS already interpret ANSI escape
codes without setup, and raw single-keypress input there is a POSIX
`termios` problem, not a `conio.h` one — so a future port should only ever
need an alternate implementation of `Console`, never a change to `World`,
`OverworldGrid`, `MapRenderer`, or `GameLoop`. (As of Milestone 2, the
non-Windows branch of `Console::readKey` is a line-based fallback, not a
real raw-input implementation — see `docs/GOTCHAS.md`.)

## Why location data is a hand-rolled text format, not JSON

Unchanged reasoning from Milestone 1: no external dependency to vendor for
the *shipped game* (the offline map generator using Python+Pillow is a
separate, dev-only tool — see below), a parser small enough
(`WorldLoader.cpp`) to read in a couple of minutes, and a format editable
without knowing any library's conventions.

## Why the overworld is generated, not hand-drawn

A tile grid covering all of Ansalon at walkable resolution is ~150,000
tiles (480×320) — far too many to hand-author, but exactly the kind of
thing the reference map image already encodes visually. So
`tools/generate_overworld.py` (Python + Pillow, **not part of the shipped
C++ game** — the game only ever reads the plain-text `data/overworld.grid`
it produces) downsamples and color-classifies the reference image into
terrain tiles, then bakes roads between named locations into the same grid.
This is a genuinely different kind of "data" than `locations.txt`: it's
machine-generated with a human-reviewed classification step in the middle
(see `docs/MAP_NOTES.md`), not hand-written, and treated that way — hand
edits to `data/overworld.grid` are tolerated but will be silently
overwritten if the generator is rerun.

Terrain *types* themselves (`world::Terrain.h/.cpp`) are code, not data:
the set of kinds (ocean, forest, mountains, ...) is small, fixed, and
tightly coupled to rendering/passability rules, which is exactly the case
where a data file would be pure indirection with nothing gained.

## Walkable interiors ("zones"): a second, smaller world, tied to the first

Milestone 3 added `Zone`/`ZoneLoader`/`ZoneCatalog` — hand-authored walkable
interiors (see `docs/ZONE_NOTES.md` for the file grammar and rationale for
why they're hand-drawn rather than generated, and why they're capped to fit
the viewport with no camera/scrolling). The key design choice: a zone is
tied to an overworld `Location` **only by filename matching id**
(`data/zones/<id>.txt` ↔ `LOCATION <id>`), checked once at startup by
`ZoneCatalog::loadForWorld` — not a field on `Location` itself. This keeps
`World`/`Location` completely unaware that zones exist at all, the same
separation-of-concerns reasoning as "why `world/` doesn't know about the
player" above: `Location` stays a single, simple, reusable concept whether
or not a walkable interior happens to exist for it.

`GameState.mode` (`Mode::Overworld` / `Mode::Zone`) is the single source of
truth for which "world" is currently active; `GameLoop` dispatches
movement/look to one of two parallel code paths
(`tryMoveOverworld`/`tryMoveZone`, `lookOverworld`/`lookZone`) based on it,
and `MapRenderer` gained a matching second render method (`drawZoneFrame`)
rather than one method trying to handle both — the two "worlds" have
different data sources (`OverworldGrid` vs `Zone`), different overlay
concepts (named `Location`s vs. `PointOfInterest`s), and different camera
behavior (scrolling vs. none), so sharing one code path would have meant
branching throughout instead of once at the top.

## Extension points for later milestones

These are the seams intentionally left in the code so later systems can
attach without reworking it:

- **Character creation** (2nd Ed. AD&D rules): add a `Character` type and a
  `Character*` (or similar) field to `game::GameState`. Nothing in `world/`
  or `render/` needs to change; `GameLoop` gains new key bindings.
- **War-of-the-Lance timeline / chance-encounter engine**: `GameState`
  already tracks `x`, `y`, and `hoursElapsed` — a future `timeline::Timeline`
  can be queried each time the player's overworld position changes
  (`GameLoop::tryMoveOverworld`, after the state update) with "who else is
  here right now?" using the same `region`/`id` fields `Location` already
  carries. `Location::region` exists today specifically so region-scoped
  canon events have something to match against later, even though nothing
  reads it yet.
- **Combat**: would plug into `GameLoop` as a new mode entered when an
  encounter triggers, reusing `Console`/`MapRenderer` primitives already in
  place.
- **Save/load**: `GameState` stays a small, flat, serializable struct on
  purpose — a save format only needs to round-trip that struct (plus,
  later, `Character`), not touch `World`/`OverworldGrid` (both static,
  reloaded fresh from `data/` every run).

## What's deliberately NOT abstracted yet

No plugin system, no generic "event" bus, no data-driven scripting layer,
no dynamic console-resize handling (the viewport is a fixed default size —
see `docs/GOTCHAS.md`). Those would be premature — the systems that would
need them don't exist yet. When a milestone that actually needs one of
these is built, revisit this document and update it to describe what
shipped, not just what was planned.
