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
world/     Location, World, WorldLoader,     -- overworld: named places (graph)
           Terrain, OverworldGrid               + walkable terrain (grid)
           ZoneTile, Zone, ZoneLoader,        -- interiors: hand-authored walkable
           ZoneCatalog                          scenes tied to overworld locations
timeline/  Timeline, TimelineLoader           -- canon character schedule, queried
                                                  by location id + in-game day
quest/     Quest, QuestCatalog,               -- hand-authored quests: offer/
           QuestLoader                          accept/turn-in text, objectives,
                                                  rewards (see docs/QUEST_NOTES.md)
character/ Dice, Ability, Race, CharClass,    -- 2e AD&D rules content, the
           Alignment, Knighthood,                interactive creation wizard,
           WizardOrder, Leveling,                level-up math, spellcasting,
           Spellcasting, Equipment,              and buyable armor/weapons
           Character, CharacterCreator
combat/    Monster, MonsterCatalog,           -- monster roster + attack/damage/
           MonsterLoader, Combat                initiative resolution math
render/    Console, MapRenderer               -- ASCII presentation + raw input
game/      GameState, GameLoop, SaveGame      -- orchestration / the actual game
main.cpp                                      -- wires the above together
```

The dependency direction is one-way: `game/` depends on `world/`,
`timeline/`, `quest/`, `character/`, `combat/`, and `render/`; `render/`
depends on `world/` (it needs to read terrain/location data to draw it),
`timeline/` (to query who's present), `combat/` (`drawCombatFrame` takes a
`Monster`), and `game/GameState` (it needs to know where the player is)
but never mutates any of them; `combat/` depends only on `character/`
(`Dice` for rolls, `Character`/`Ability` for the player side of an
attack); `world/`, `timeline/`, `quest/`, and `character/` depend on
nothing else in the project. This means those four, `combat/`, and
`render/` can each be understood, tested, or reused in isolation without
pulling in the whole game loop. Notably, `render/` does **not** depend on
`quest/` — `MapRenderer::drawJournalFrame` takes a fully-resolved
`JournalEntry` (title, complete flag, pre-marked objective-line strings)
built by `game::GameLoop`, the same "presentation stays ignorant of the
domain type" split `drawDialogueFrame` already established for
`timeline::PresenceWindow`/`world::PointOfInterest`.

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

`Console::readLine` (the "ask about..." free-text prompt — see
`docs/TIMELINE_NOTES.md`'s "Ask about anything") is the one other input
primitive in `Console`, added alongside `readKey` rather than reusing
`CharacterCreator`'s plain `std::cin`/`getline` approach: it stays inside
the same raw-`_getch()` input model `readKey` already uses (reading one
character at a time and echoing manually) specifically to avoid mixing two
different console input APIs while `GameLoop` is live — a combination this
codebase has never needed and hasn't tested (see `docs/GOTCHAS.md`).
Consequence: like the rest of `GameLoop`'s input, it can't be driven by a
piped/redirected script either.

**As of Milestone 72**, the adapter that turns a `timeline::PresenceWindow`
into a `game::Speech` (`speechFromWindow`, in `GameLoop.cpp`, called from
both the overworld and `TIMELINE_ANCHOR` talk paths in `handleTalk`) now
calls `timeline::Timeline::subjectsFor`/`subjectUnknownFor` instead of
reading `window.subjects`/`window.subjectUnknown` directly, so a
character's day-gated subject pool layers in underneath that window's own
`SUBJECT` entries. Nothing downstream of `Speech` changed: `talkTo`,
`TalkCandidate`, `pickAndTalk`, `tokenizeAskInput`, and `matchSubject` are
all untouched — the same "the executor was always source-agnostic; only
the loader needed to learn the new grammar" precedent Milestones 26 and 71
both already established.

**Flagged future direction, not a commitment:** if real cross-platform
builds or sprite rendering ever become an actual goal, this isolation is
exactly what would make an SFML-backed `Console`/renderer a contained
swap rather than a rewrite — SFML was chosen over SDL2 for fitting this
codebase's existing modern-C++ (RAII) style more closely. It would touch
only `render/Console.cpp`, `render/MapRenderer.cpp`, and the input-polling
call sites in `game/GameLoop.cpp`; `World`, `ZoneCatalog`, `Timeline`,
`MonsterCatalog`, and every data loader are untouched either way, since
none of them depend on `render/`. The one real cost: this project
currently has zero external dependencies, and SFML would be the first —
bringing in vcpkg or `FetchContent` is a bigger step than it sounds for a
repo this deliberately minimal. See `docs/MILESTONES.md`'s "NEXT UP".

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

### Zones can nest, via portals

Added when the Inn of the Last Home got a real interior: a `PORTAL <char>
<zone-id>` line in a zone file marks a tile as a doorway into *another*
zone (see `docs/ZONE_NOTES.md` for the grammar). `GameLoop::handleEnter`
checks `Zone::portalAt` before its existing entry/exit logic — stepping
onto a portal tile pushes the current zone id and position onto
`GameState::zoneStack` and switches to the target zone; leaving via that
zone's own `ENTRY` tile pops the stack and returns to the exact spot the
player portaled in from, falling back to the pre-existing overworld-exit
behavior only once the stack is empty. This is a small, generic stack, not
a single-level special case, so it supports arbitrary nesting depth (a
shop's back room, a keep's tower) without further changes.

The one piece of plumbing this required in `ZoneCatalog`: a zone reached
*only* via a portal (like the Inn's interior) has no matching overworld
`Location`, so it wouldn't be found by `loadForWorld`'s normal
location-id-matching pass. A second pass follows every loaded zone's
`PORTAL` targets transitively, loading anything not already found — so
`data/zones/solace_inn.txt` is discovered purely because
`data/zones/solace.txt` references it, with nothing in
`data/locations.txt` needing to know it exists.

`tryMoveZone`, `lookZone`, and `drawZoneFrame` needed **no changes** to
support this — they already operate generically on whatever zone
`zones_.getZone(state_.currentZoneId)` returns, nested or not.

## Character creation: a fourth module, and a different interaction mode

Milestone 4 added `src/character/` (`Dice`, `Ability`, `Race`, `CharClass`,
`Alignment`, `Character`, `CharacterCreator`) — a new module sibling to
`world/`, `render/`, `game/`, depending on nothing else in the project (same
independence as `world/`). `game/GameState` depends on it (a
`character::Character character;` field), and `render/MapRenderer` depends
on it (`drawCharacterSheet`) — the same one-way dependency shape as
everything else in the project.

The interesting design choice here isn't the data model (it's plain structs
and fixed lookup tables, same pattern as `Terrain`/`ZoneTile`) — it's that
`CharacterCreator::run()` deliberately does **not** use
`render::Console::readKey()`. Every other piece of player interaction in
this game is single-keypress, real-time, no Enter required — but character
creation is a one-time, deliberate, step-by-step wizard (name a character,
roll and maybe reroll ability scores, pick from a numbered list), which is
a fundamentally different interaction shape than "move one tile per
keypress." Rather than force it through the movement input model, it's
plain blocking `std::cin`/`std::cout` prompts, run once in `main.cpp`
*before* `GameLoop` (and its raw-keypress world) ever starts. This also
means, usefully, that character creation is the one part of the game that
**can** be driven by piped/redirected input for testing — see
`docs/GOTCHAS.md`.

`docs/CHARACTER_NOTES.md` documents the actual 2e ruleset scope (which
mechanics are modeled, which are deliberately deferred, and an explicit
"these numbers are from memory, spot-check them" note).

## Save/load: GameState was already shaped for this

`game::SaveGame` (`src/game/SaveGame.h/.cpp`) is a small module sibling to
`GameState`/`GameLoop`, depending on nothing but `GameState` itself (and
transitively, `character::Character`). This was cheap to add precisely
because `GameState` had already been kept flat and serializable from the
start (see the old note in "Extension points," now shipped) — there was no
restructuring needed, just a format and a place to call it from.
`SaveGame::save`/`load` take an arbitrary `path`, which is what let
Milestone 89's multi-slot system below be built with zero changes to this
module.

**Format**: a hand-rolled keyword-per-line text file, following the same
`trim`/`splitKeyword`/fail-fast idioms as `WorldLoader`/`ZoneLoader`.
`character::RaceId`/`ClassId`/`Alignment` are stored as raw enum ints
rather than names — see `docs/GOTCHAS.md` for the fragility that trades
off against.

**When it saves**: `GameLoop::run()` calls `SaveGame::save` once per loop
iteration, unconditionally, right after the input switch (including on
`Quit`, which now sets a local flag and falls through to the save instead of
returning directly) — see the loop in `GameLoop.cpp`. Simpler than tracking
"did this specific key mutate state," and cheap enough not to matter: a
crash or an ungraceful close loses at most the single most recent keypress.
`GameLoop` is handed whichever save-slot path was chosen at startup (below)
and knows nothing about slots itself — it just autosaves to that one path.

**3 save slots, resolved and picked in `main.cpp` (Milestone 89)**:
`save1.txt`/`save2.txt`/`save3.txt`, resolved next to the running
executable, same pattern as `data/` — see `docs/GOTCHAS.md`. Fixed at 3,
not configurable — the ask was "at least 3," and a generic N-slot system
wasn't needed for it. Before the menu is built, a pre-Milestone-89 single
`save.txt` is migrated to `save1.txt` if no `save1.txt` exists yet
(`std::filesystem::rename`, non-fatal on failure), so an in-progress
character from before this milestone is carried forward automatically.

For each of the 3 slots, a local `describeSlot` helper (`main.cpp`) tries
`SaveGame::load` plus the same "does the saved `ZONE` still exist"
cross-check the single-save flow used to do inline — `SaveGame` doesn't
know about `ZoneCatalog`, so `main.cpp` is where a save referencing a
since-removed zone gets caught, same reasoning as `WorldLoader` not
validating a location's `POS` against the grid. Unlike the pre-Milestone-89
behavior, a slot that fails to load no longer aborts the whole program — it
just shows as unreadable in the menu, isolated from the other two slots.

The slot-picker menu itself is plain `std::cin`/`std::cout`, the same
interaction mode `CharacterCreator` uses (see above), with its own small
local `promptLine`/`promptSlotChoice`/`promptYesNo` (mirrors
`CharacterCreator.cpp`'s file-local reprompt-on-garbage/throw-on-EOF
idiom — kept as its own copy here rather than shared across translation
units, same "each file owns its own tiny copy" precedent as
`trim`/`splitKeyword` across the various `*Loader.cpp` files). Picking an
empty slot goes straight to `CharacterCreator::run()`; picking an occupied
slot offers "Continue this character?", and declining asks a second,
explicit confirmation before a fresh character is allowed to overwrite it
on the next autosave.

**Explicit delete (Milestone 94)**: the menu also accepts `d1`/`d2`/`d3`
(`main.cpp`'s `SlotChoice`, an extension of `promptSlotChoice`) to delete
a slot's save file immediately, after its own `y/n` confirmation --
distinct from the overwrite-on-next-autosave flow above, which only
replaces the file once the player actually plays and an autosave fires.
Deletion itself is `game::SaveGame::remove` (a thin, non-throwing
`std::filesystem::remove` wrapper next to `exists()`); the affected
`SlotInfo` is then refreshed via the same `describeSlot` helper so it
comes back showing `(empty)` without any special-case reset logic.

## Timeline / chance-encounter engine

The core original pitch for this whole project (see `README.md`), and the
reason `GameState` was kept tracking `x`, `y`, and `hoursElapsed` from
Milestone 2 onward. `src/timeline/` (`Timeline`, `TimelineLoader`) is a new
module sibling to `world/`/`character/`/`render/`/`game/`, depending on
nothing else in the project — same independence as `world/`.

**Data model**: `Timeline` owns a list of `CanonCharacter`, each with a
`schedule` of `PresenceWindow { locationId, dayStart, dayEnd, flavorText }`.
`Timeline::presentAt(locationId, day)` returns every character whose
schedule places them there on that day (inclusive range), paired with the
specific window that matched (a `Presence{character, window}` struct) so
callers get the flavor text without re-scanning. Loaded once at startup by
`TimelineLoader::loadFromFile` from `data/timeline.txt` — same
hand-rolled-text, `trim`/`splitKeyword`/fail-fast pattern as
`WorldLoader`/`ZoneLoader` (see `docs/TIMELINE_NOTES.md` for the grammar).
**As of Milestone 72**, `CanonCharacter` also carries a character-level
`subjects` pool (`std::vector<CharacterSubject>`, each day-range-gated) and
an optional character-level `subjectUnknown` — the free-text "ask about
anything" content that used to live only on `PresenceWindow`, now
available at every one of a character's windows instead of needing to be
copy-pasted into each. Two pure queries, `subjectsFor`/`subjectUnknownFor`,
resolve a window's own subjects plus the character's day-filtered pool
into the list a talk actually uses — see `docs/TIMELINE_NOTES.md`'s
"Character-level subject pools" for the resolution order and why `timeline/`
still returns its own `Subject` type rather than `game::Speech::SubjectEntry`
(same module-independence reasoning as everything else in this section).

**Static content, not player state**: like `World`/`OverworldGrid`, a
`Timeline` is loaded fresh every run and never mutated during play — only
`GameState.hoursElapsed`/position (both already saved) determine what's
currently showing, so nothing in `game::SaveGame` needed to change.

**The query hook**: `MapRenderer::drawOverworldFrame` gained a `const
Timeline&` parameter (`GameLoop` holds one alongside `World`/`OverworldGrid`
/`ZoneCatalog`, passed through from `main.cpp`). When the player is
standing on a `Location`, it calls `timeline.presentAt(here->id,
state.hoursElapsed / 24)` and prints one line per result, right after that
location's own description — a **persistent** line (redrawn every frame
while the window is active), not a one-shot toast, so leaving and
returning during the same window still shows the encounter. Deliberately
scoped to the overworld only for now; showing presence inside a zone
(actually finding someone *inside* the Inn of the Last Home, say) is real
future work — see `docs/TIMELINE_NOTES.md`.

`data/timeline.txt` currently has two characters (Tanis Half-Elven,
Raistlin Majere) across four of the eight locations — a first content
pass, meant to grow the same incremental way `data/zones/*.txt` did.

## Combat

New module `src/combat/` (`Monster`, `MonsterCatalog`, `MonsterLoader`,
`Combat`), depending only on `character/` (for `Dice`, `Ability`,
`Character`) — same independence pattern as `timeline/`. Full accuracy/
design writeup in `docs/COMBAT_NOTES.md`; the two decisions most worth
knowing before touching this code:

**Not a `GameState.mode`.** Unlike `Mode::Overworld`/`Mode::Zone`, an
encounter is a nested loop inside `GameLoop` (`runCombat`) — it takes over
rendering/input in its own loop, same shape as `showCharacterSheet`'s
existing one-keypress block, then returns control when the fight ends.
Combat state (monster HP, the log) is local to that loop, never touches
`GameState` or `game::SaveGame` — a crash mid-fight just loses the
encounter, an accepted trade-off given the death rule below.

**Knocked out, not killed.** A direct user decision, chosen over
permadeath: HP reaching 0 fully heals the player and warps them to the
**nearest town** (`GameLoop::nearestTown()`, straight-line distance to
whichever `world::Location::isTown` entry is closest — see
`docs/COMBAT_NOTES.md`'s "Death: knocked out, not killed" for the town
list and why it's a curated subset, not every named location). This is
*why* combat could stay out of the save format at all — if death were
permanent (or even just persistent partial damage across a crash
mattered), losing an in-progress fight to a crash would be a much bigger
deal.

`MapRenderer` gained a third per-context draw method, `drawCombatFrame`,
alongside `drawOverworldFrame`/`drawZoneFrame` — same "one method per
mode" pattern, even though combat isn't technically a `Mode`.
`render::Key` gained `Flee` (`'f'`/`'F'`); adding it required a new case
in the main loop's `switch` in `GameLoop::run()` too, since that switch
relies on exhaustive `Key` coverage rather than a `default:` label.

## Leveling

New `character/Leveling.h/.cpp` and `character/WizardOrder.h/.cpp`,
called from `GameLoop::runCombat` right after a monster's XP award (the
only new call site — everything else is `character/`-internal). Full
sourcing and scope cuts in `docs/CHARACTER_NOTES.md`'s "Leveling /
experience" section; the two things most worth knowing before touching
this code:

**One function does the whole level-up.**
`character::applyPendingLevelUps(Character&, messages)` loops one level
at a time (not jumping straight to the target level from a big XP award),
recomputing HP/THAC0/saves via small lookup tables (PHB Tables 53/60,
transcribed the same "terrain-as-code" way as `world::Terrain`) and
re-applying `applyRacialSavingThrowBonus` after each level's save-table
lookup, the same as character creation does once. Looping level-by-level
isn't just tidiness — it's what makes the level-3 flavor checks (Knight of
the Crown → Sword notice, Mage → Test of High Sorcery) reliably fire even
if a single large XP award crosses several levels at once.

**Mage's Test of High Sorcery is real, not just flavor.** Unlike the
Knight-of-the-Crown-at-3rd-level line (pure flavor text, no state change —
Sword itself isn't modeled), reaching level 3 as a Mage actually sets
`Character::robeColor` via `character::robeForAlignment` — the
alignment-to-Robe mapping was already fully sourced in Milestone 7's
Dragonlance research and needed no quest-like gate to apply, unlike Sword
Knighthood's "witnessed quest of heroism" requirement.

## Spellcasting

New `character/Spellcasting.h/.cpp`, depending only on `character/`
(`Dice` for rolls, `Race` for `effectiveCanBeMage`) — same independence
pattern as `Leveling`/`WizardOrder`. Full sourcing and scope cuts in
`docs/CHARACTER_NOTES.md`'s "Spellcasting" section; the shape most worth
knowing before touching this code:

**One known spell per casting class, real per-day slot counts.** Mage
knows Magic Missile, Cleric knows Cure Light Wounds — no spellbook, no
selection, same "one sourced thing, not a whole subsystem" precedent as
`ClassInfo`'s placeholder weapon. `maxSpellSlotsPerDay` and
`hasSpellSlotAvailable` are the only two functions `GameLoop::runCombat`
needs to call before letting a cast happen; `castSpell` resolves the
one known spell's effect and consumes a slot.

**Slots refill via day rollover, not a rest command.**
`Character::spellsCastToday`/`spellsCastDay` reset themselves the moment
`hasSpellSlotAvailable` is checked against a new `hoursElapsed / 24` value
— the same day convention `timeline::Timeline` already uses. No explicit
"memorize spells" or "pray" action exists.

**A previously flavor-only restriction now has teeth.** Kender and the
three Mage-blocked subraces (Kagonesti Elf, Hill/Mountain Dwarf) always
get `maxSpellSlotsPerDay() == 0` for a Mage, via the same
`effectiveCanBeMage` check `CharacterCreator` already used to print a
non-blocking warning. Character creation itself is unchanged — you can
still create one of these as a Mage — but casting now actually fails in
combat instead of silently working.

## NPC interaction: a real "talk" action

`t` (`render::Key::Talk`) is a new normal-mode action, alongside `Look`/
`Enter`/`Sheet` (a real case in `GameLoop::run()`'s switch, not
combat-only like `Flee`/`Cast`), dispatching to a new
`GameLoop::handleTalk()`. Full grammar/content notes in
`docs/ZONE_NOTES.md` ("NPCs: POIs you can talk to") and
`docs/TIMELINE_NOTES.md` ("Talking to a canon character"); the shape most
worth knowing before touching this code:

**Two data sources, one action -- three since Milestone 23.**
`handleTalk()` builds a `std::vector<TalkCandidate>` (`{id, name, Speech}`,
`GameLoop.h`) from whichever of these apply to the player's current tile:
in a zone, `Zone::poiAt` (a `POI` gains a `dialogue` field via an
optional `TALK` line in the zone file); on the overworld,
`Timeline::presentAt` on the player's location and current day (a
`PresenceWindow` gains a `dialogue` field via an optional `SAY` line
following its `PRESENCE`); and, as of Milestone 23, `Timeline::presentAt`
*again* when standing on a zone's `TIMELINE_ANCHOR` tile, checking the
zone's effective timeline location (its own catalog id by default, or an
explicit `TIMELINE_LOCATION` override — see `docs/TIMELINE_NOTES.md`
for why `solace_inn.txt` needs one and the other zones don't). All three
reuse queries `drawZoneFrame`/`drawOverworldFrame` already make for their
existing passive descriptions — talking adds a second, deliberate read of
the same data, it doesn't replace the passive line.

**Real "have I met them" tracking, as of Milestone 18.**
`GameState::metCharacters` (a set of ids, saved via `SaveGame` the same
way as the pre-existing `visitedLocations`) records who the player has
talked to. Timeline characters use their existing stable id, on the
overworld *and* at a `TIMELINE_ANCHOR` tile — the same id either way, so
"met on the overworld" and "met at the Inn's fireplace" are the same
fact, not two. Zone-native NPCs (Otik, Tika, etc.), which have no stable
id of their own, get a synthesized `"<zoneId>:<char>"` key instead.

**A shared `pickAndTalk` helper for when several candidates share a
spot.** Now that all 8 Heroes of the Lance can be present at once
(Milestone 17) — or a zone's `TALK` POI and a `TIMELINE_ANCHOR` character
could both be candidates on the same tile (Milestone 23) —
`GameLoop::pickAndTalk(candidates)` is the one place "0 -> nothing to
talk to; 1 -> talk directly; 2+ -> ask which one" lives, via a
`drawPickerFrame` cursor picker (same nested-loop, locally-reinterpreted-
`Key::North`/`South`/`Enter`/`Quit` shape `handleShop` already
established). Both `handleTalk()` branches (overworld and zone) build
their own `TalkCandidate` list and hand off to this one helper, rather
than each keeping its own copy of the picker loop.

**Reactive dialogue, real repeat-visit lines, and branching topics, as of
Milestone 19.** `game::Speech` (`GameLoop.h`) is the adapter shape
`GameLoop::talkTo(id, name, const Speech&)` works with regardless of
source (built from either a `timeline::PresenceWindow` or a
`world::PointOfInterest`, which stay fully decoupled from
`character::Character` — see "Module map" above): a default
greeting, an ordered list of `(condition, text)` reactive variants, an
optional repeat-visit line, and an ordered list of `(label, text)`
topics. `talkTo` is the single place all of this logic lives, so every
talk path (zone POI, single-character overworld, post-picker overworld)
behaves identically:
- **First visit**: the first `conditional` entry whose condition matches
  the player's character (`game::conditionMatches`, a free function
  specifically so it's unit-testable without constructing a whole
  `GameLoop`) wins over the plain greeting.
- **Every visit after**: the authored repeat-visit line if one exists,
  else the Milestone 18 generic recognition fallback.
- **If `Speech::topics` is non-empty**, a topic-picker loop follows the
  greeting (same `drawPickerFrame`), looping back after each topic so a
  player can ask about more than one thing per visit, until "Nothing,
  thanks" or `Key::Quit`.

`MapRenderer::drawDialogueFrame` (the existing fifth per-context draw
method) takes a list of `{speaker, text}` pairs — `talkTo` always passes
exactly one now — and blocks for one keypress to dismiss, same shape as
`drawCharacterSheet`. `drawPickerFrame` (generalized from a
Milestone-18-only "talk to whom?" picker into a reusable
title/items/footer picker, now also used for topic menus) is a seventh
per-context draw method (after `drawShopFrame`'s sixth, see "Equipment"
above).

**As of Milestone 43, Talk and Look are two separate reads of the same
"who's present" data, not one.** Talk (`t`) still works exactly as
described above. Look (`;`) gained its own, read-only
`pickAndLook`/`LookCandidate` pair (see "Frameless overworld/zone layout
+ NPC 'Look'" below) that surfaces an NPC's full description on demand —
the passive arrival log now only ever prints their name.

## Equipment

New `character/Equipment.h/.cpp`, depending only on `character/`
(`Ability` for `acAdjustmentForDexterity`) — same independence pattern as
`Spellcasting`/`Leveling`. Full sourcing and scope cuts in
`docs/CHARACTER_NOTES.md`'s "Equipment" section; the shape most worth
knowing before touching this code:

**Equipped fields vs. the carried inventory.** `Character` carries its own
`equippedArmor`/`hasShield`/`weaponName`/`weaponDamageSides`/
`weaponDamageBonus` — "what's worn right now" — seeded from `ClassInfo` at
creation (`CharacterCreator.cpp`). `combat::resolvePlayerAttack` reads
these fields directly; `ClassInfo`'s weapon fields are only ever a
starting point. Separately, `Character::inventory`
(`std::vector<character::InventoryItem>`, Milestone 21) holds carried-but-
not-worn gear. `Equipment::purchaseItem` only ever appends to `inventory`
now — it never touches the equipped fields directly. The only function
that moves an item between the two is `equipInventoryItem`, which swaps a
carried item into its slot and pushes whatever was equipped there back
into `inventory`.

**A zone-grammar addition, not a new subsystem.** `SHOP <char>` (see
`docs/ZONE_NOTES.md`) marks an existing POI as browsable — same "TALK
layers an ability onto a POI" pattern used for NPC dialogue.
`GameLoop::handleShop()` takes over input in its own nested loop, same
architectural shape `runCombat` already established for a self-contained
input mode: it reinterprets `Key::North`/`South`/`Enter`/`Quit` locally
(cursor move, buy, exit) rather than adding new `Key` values for item
selection, and `Key::Quit` inside that loop means "leave the shop," not
"quit the game" — the same local-reinterpretation trick `runCombat` uses
for `Key::Flee`. `GameLoop::handleInventory()` ('i', Milestone 21) is the
same nested-loop shape again, but unlike `handleShop` it's not gated on
standing at any particular POI — carried gear travels with the player.

**`availableShopItems`/`purchaseItem`/`equipInventoryItem` decide what's
on offer and whether an action is legal, `GameLoop`/`MapRenderer` just
render and select.** Same split of responsibility `Spellcasting`'s
`castSpell` already established between rules logic and presentation — a
Mage's armor restriction, insufficient steel, and "already owned" (checked
against both the equipped slot and inventory contents as of Milestone 21)
are all decided in `Equipment.cpp`, never in `GameLoop.cpp`.
`MapRenderer::drawInventoryFrame` is an eighth per-context draw method
(after `drawPickerFrame`'s seventh, see "NPC interaction" above).

## Presentation: a wide HUD + side-by-side log panel (Milestone 29)

Prompted by a Caves of Qud reference screenshot the user dropped into
`References/CavesofQud.jpg` and asked to match. Qud's screen has three
things the original `MapRenderer` (Milestone 2) didn't: a persistent
scrolling event log rather than one transient line, a compact top HUD
(HP bar, stats) instead of prose below the map, and a wide side-by-side
layout — map left, log panel right — instead of one narrow column.

**Still a fixed frame size, just a wider one.** `kViewportWidth`/
`kViewportHeight` (78×20, the map itself) are unchanged — zones are still
authored to fit them. `MapRenderer` gained `kLogPanelWidth`/
`kLogPanelGap` (40/2), so the real frame is now 120 columns wide, and
taller (~30 rows) to fit the new HUD lines. This is the same "not queried
from the actual console" tradeoff `kViewportWidth`/`Height` already made
(see "What's deliberately NOT abstracted yet" below) — **dynamic
terminal-resize handling is still not implemented**, this milestone just
widens the fixed assumption. `README.md`'s stated minimum terminal size
moved from 78×~24 to 120×30 accordingly (chosen to match Windows
Terminal/console host's own common 120-column default buffer, so most
users shouldn't need to resize at all).

**`GameLoop::message_` (a transient `std::string`, set then cleared every
frame) became `log_` (`std::vector<std::string>`, capped at 300 entries
via a new `pushLog` helper) — a real, persistent history**, matching
Qud's own ":: you take X" scrolling log. Every existing `message_ = ...`
call site became `pushLog(...)` (mechanical, no behavior change to *what*
gets logged, just that it now accumulates instead of being overwritten
each frame). `runCombat` also pushes a short one-line summary to `log_`
at its start and each of its three exit points (victory/flee/knockout) so
the exploration log has continuity after a fight — this is separate from,
and doesn't duplicate, the full attack-by-attack local `log` vector
`drawCombatFrame` already shows on its own dedicated screen.

**`MapRenderer::buildLogPanel`** (a new private helper) wraps every raw
`log_` entry to `kLogPanelWidth` (greedy word wrap via `wrapText`), takes
the tail `kViewportHeight` physical lines (oldest scrolls off the top
once the panel fills — most recent is always visible at the bottom), and
pads the *end* with blank lines while there isn't yet a full panel's
worth of content, so a fresh session's log anchors at the top and grows
downward rather than jumping around.

**Only `drawOverworldFrame` and `drawZoneFrame` got this treatment** —
matching exactly what the reference screenshot depicts (walking around),
not `drawCombatFrame`/`drawShopFrame`/`drawInventoryFrame`/
`drawCharacterSheet`/`drawDialogueFrame`/`drawPickerFrame`, which keep
their existing simple full-screen text. Consistent with "restraint over
completeness" — revisit if a future milestone wants those widened too.

**`drawZoneFrame` now always renders the full `kViewportWidth`/`Height`**
(previously it looped `zone.width()`/`height()`, whatever size that
specific zone happened to be) so the log panel's left edge sits at a
stable screen column regardless of which zone is showing. Safe because
`Zone::tileCodeAt`/`poiAt` already return `'#'`/`nullptr` for any
out-of-bounds coordinate (a pre-existing guarantee, not something added
for this) — a zone smaller than 78×20 now wall-pads out to the full frame
size, arguably a nicer look (framed by walls) than the old blank space.
See `docs/GOTCHAS.md` and `docs/ZONE_NOTES.md`.

## The "standing here" text moved into the log too (Milestone 30)

Milestone 29 left one thing behind: the location/POI description block
(a Location's `== Name (Region) ==` heading + description, or a zone
POI's name + description, plus any canon-character presence/flavor
lines) still printed full-width below the map every single frame,
recomputed fresh each time by `MapRenderer` regardless of how the player
got there. Milestone 30 folds this into the log too, matching how Caves
of Qud actually works — it has no separate persistent description box,
place flavor is just another log entry.

**`GameLoop` gained two "announce" helpers** — `announceOverworldTile()`
and `announceZoneTile()` — that push the same content `MapRenderer` used
to print, but as `pushLog(...)` calls instead, called once per arrival:
from `run()` before the loop starts (parity with the very first frame),
from `tryMoveOverworld`/`tryMoveZone` after a successful step, and from
all four of `handleEnter`'s arrival points (entering a zone, a portal
into a nested zone, popping back to a parent zone, and exiting to the
overworld). `MapRenderer::drawOverworldFrame`/`drawZoneFrame` lost the
`timeline::Timeline` parameter entirely — they no longer need to know
about it, since that lookup now happens in `GameLoop` at announce time.

**Two deliberate omissions, not oversights**: plain overworld terrain
(previously "You are in grassland.", redrawn every frame) gets no log
entry at all — logging every wilderness step would flood the panel and
bury real events; the map glyph already conveys terrain. And a zone's
own `== Name ==` heading (also previously redrawn every frame,
regardless of what tile the player stood on) isn't ported anywhere —
it's already implied by the "You step into X."/"You step back out into
X." log lines `handleEnter` pushes on the same transition. Both are
minor, visible changes (no persistent on-screen reminder of which
zone/terrain the player is in beyond the map itself) accepted as the
point of the change rather than compensated for with new HUD content.

## A taller live panel and a dedicated log-history screen (Milestone 31)

Milestones 29-30 gave `log_` real content, but the live side panel
(`MapRenderer::buildLogPanel`) is a tail view only — once a wrapped line
scrolls off the top of its fixed `kViewportHeight`-row window, there was
no way to get back to it. Two fixes, both requested together:

**`kViewportHeight` went from 20 to 30.** Since the map render loop, the
live log panel, and zone wall-padding (Milestone 29) all already key off
this one constant, this was a one-line change — no restructuring. Side
effect: the overworld camera and zone view both show 10 more rows too,
a harmless bonus. `README.md`'s stated minimum terminal size moved from
120×30 to **120×36** (2 HUD + 1 blank + 30 map/log + 1 blank + 1
controls = 35, +1 buffer).

**A new dedicated, pageable full-history screen**, bound to `v`/`V`
(`l`/`L` was already taken by East, the vi `hjkl` convention). Same
architectural shape as `handleShop`/`handleInventory`: a new
`render::Key::Log` enum value, `GameLoop::handleLog()` (a nested loop
that reinterprets `North`/`South` locally as "scroll by
`kLogScrollStep` (10) wrapped lines" rather than one line at a time — a
long session's `log_` can wrap to hundreds of physical lines, so a
single-line-per-keypress pager would feel unusably slow), and a new pure
`MapRenderer::drawLogFrame(log, scrollOffset)`. Unlike the live panel,
`drawLogFrame` wraps *every* entry (not just the tail) to a wider
`kLogFrameWidth` (110, vs. the live panel's 40 — this screen has the
whole frame to itself) and returns the actual clamped offset it used, so
`handleLog`'s loop can keep adjusting a plain `int` across calls without
duplicating the wrap/line-count math itself. A negative `scrollOffset`
means "start at the bottom" (most recent), matching the live panel's own
default-to-recent convention.

**The one thing that had to be touched everywhere**: `docs/GOTCHAS.md`
already documents that `GameLoop::run()`'s main `switch` relies on
*exhaustive* `Key` coverage (no `default:` label), so adding
`Key::Log` required a matching `case render::Key::Log: handleLog();
break;` there too — the same rule Milestone 9's `Key::Flee` addition
already hit.

## A plain-ASCII window border on every screen (Milestone 32)

Asked to make the game feel like a contained app rather than raw text
floating in a console. Unicode box-drawing (`─│┌┐` etc.) was considered
and rejected — a real functional risk, not a style choice, since
`Console.cpp` deliberately never enables UTF-8 console output (see
`docs/GOTCHAS.md`), so those glyphs would garble on plenty of Windows
consoles. Landed on a plain `+`/`-`/`|` border instead, applied to
*every* `draw*Frame` function, each sized to hug its own content rather
than all forcing themselves into one giant fixed window.

**Entirely contained inside `MapRenderer.cpp`** — every `draw*Frame`
function kept its exact existing signature, so `GameLoop` needed zero
changes; it has no idea its frames are bordered now.

**Two families, because of ANSI color codes.** A border requires every
row between the two `|` characters to be *exactly* the box's inner
width — including the map+log rows (`drawOverworldFrame`/
`drawZoneFrame`), which carry a `\x1b[...m`/`\x1b[0m` pair around every
single glyph. Measuring "visible width" through those escape codes with
a generic `std::string::size()`-based helper would silently miscount, so:
- **"Fixed" screens** (`drawOverworldFrame`, `drawZoneFrame`,
  `drawLogFrame`) already build their content at a known, exact width by
  construction — the map+log rows are already exactly `kViewportWidth +
  kLogPanelGap + kLogPanelWidth` (120) visible columns, and
  `drawLogFrame`'s wrapped lines are plain text padded to
  `kLogFrameWidth` (110). These build their own exact-width line vector
  (HUD/blank/footer lines padded via `padPlain`; map+log rows trusted
  as-is, never re-measured) and hand it straight to the shared
  `writeBorder` primitive.
- **"Organic" screens** (`drawCharacterSheet`, `drawCombatFrame`,
  `drawDialogueFrame`, `drawPickerFrame`, `drawShopFrame`,
  `drawInventoryFrame`) have no ANSI, but unbounded line lengths
  (dialogue text, a long carried-inventory listing). These go through
  `writeBoxed`, which wraps any line longer than `kProseWrapWidth` (76)
  via the existing `wrapText` helper (reused unchanged), computes box
  width as the longest resulting line (or the title's own minimum width,
  whichever is bigger) clamped to `[kSecondaryBoxMinWidth=20,
  kSecondaryBoxMaxWidth=100]`, pads every line to that width via
  `padPlain`, then also calls `writeBorder`.

**`writeBorder(out, title, width, exactWidthLines)`** is the one place
the actual `+`/`-`/`|` characters get printed — both families funnel
into it, so there's exactly one border implementation, not two.

**Titles replace the old `"=== X ==="` content lines.** Every screen
already printed something like `"=== Combat ==="` or `c.name` as its
first line — these fold into the border's top edge as a title bar
instead (`"+-- Combat ---...--+"`). Footer/controls hint lines moved
from "printed after the box" to "the last content line inside it" —
everything about a screen now lives inside its own window.

**New frame sizes**: the main map/log screen is 120×35 content → 124×37
bordered (2 border rows, 4 border/padding columns);
`README.md`'s minimum terminal size moved to 124×38. "Organic" screens
vary per their own content and never exceed 100 columns wide
(`kSecondaryBoxMaxWidth`), comfortably under the main screen's 120.

**Superseded by Milestone 33 below** — those fixed 124×38/100-column
numbers stopped being fixed almost immediately once someone actually ran
the game in a real (smaller) terminal.

## Adaptive layout sized to the real console window (Milestone 33)

Every one of Milestones 29-32 grew the frame without checking it against
a real terminal (78×24 → 120×30 → 120×36 → 124×38) — the user hit the
predictable consequence: the frame no longer fit their actual console
window, so they had to scroll to see it, defeating the entire point of a
full-screen redraw. Fixed by querying the real console size at startup
and sizing the frame to it, rather than continuing to guess a fixed
number.

**Key finding that shaped the whole design**: measuring every zone's
actual authored `GRID` size (`data/zones/*.txt`) showed the largest is
`solace_inn.txt` at **44×16** — every other zone is 40×16 or smaller.
The 78×30 map viewport was never a real content requirement, just a
generous default with a lot of unused headroom to shrink into before
anything would actually clip.

**`Console::currentWindowSize()`** (new static method, same
static-only-public-API shape as `readKey()`/`clearScreen()`) queries
`GetConsoleScreenBufferInfo` and reads **`srWindow`** (the visible
window rectangle) — deliberately **not** `dwSize` (the scrollback
buffer size, which can be much taller than what's actually on screen).
Using `dwSize` by mistake here would silently reintroduce the exact bug
this method exists to fix — see `docs/GOTCHAS.md`. Falls back to a
conservative `{80, 24}` if the query fails (stdout redirected — every
piped smoke test and the throwaway self-test pattern hit this path) or
on non-Windows, where no real implementation exists yet.

**`MapRenderer::configureLayout(columns, rows)`** turns
`kViewportWidth`/`kViewportHeight`/`kLogPanelWidth`/`kLogFrameWidth`
from `static constexpr int` into plain `static int`, default-initialized
to their old preferred values (so a throwaway self-test that never calls
`configureLayout` behaves exactly like the old fixed-size build).
Priority order: map width gets the 78-preferred size unless that would
leave the log panel below its own 20-column floor, in which case map
width shrinks — but never below `kMinViewportWidth` (44, the measured
zone floor above) — just enough to guarantee the log panel its minimum;
the log panel then takes whatever's left, clamped to `[20, 60]` (a huge
terminal doesn't need an absurdly wide log column — the unused width
past 60 is just left as slack rather than forcing the frame to fill the
whole screen). Map height is `min(30, available)`. Returns `false`
(layout left unchanged) below `kAbsoluteMinColumns`/`Rows` (70×23,
derived from the same constants, not a separately-chosen number) —
`main.cpp` prints a clear error and exits rather than attempting to
render something broken, the same fail-fast convention already used for
a missing starting location or an out-of-bounds `POS`. The two "organic
screen" internals (`kProseWrapWidth`, `kSecondaryBoxMaxWidth` in
`MapRenderer.cpp`'s anonymous namespace) get the same treatment, so a
narrow-but-still-valid terminal can't overflow on a long dialogue line
even though the main frame fits fine.

**Zone-authoring consequence**: `docs/ZONE_NOTES.md`'s "must fit inside
the viewport" constraint now means the guaranteed floor
(`kMinViewportWidth`/`kMinViewportHeight`, 44×16), not whatever a given
run's larger/preferred layout happens to be — a zone has to work on the
smallest terminal this game will still run on, not just the author's own
big monitor. Every currently-authored zone already fits (44×16 is
exactly the measured maximum), so this is a documentation correction,
not a required content change.

**Scope boundary, stated plainly**: this adapts **once, at process
startup** — not continuously. A terminal resized mid-session won't
reflow until the game restarts; `docs/GOTCHAS.md`'s and this document's
older "dynamic resize is deferred" framing still holds for that
narrower, harder problem. Solving the reported bug (frame bigger than
the window at launch) didn't require solving live resize too, and
taking on that scope wasn't asked for.

## Sea travel: a scripted one-time voyage (Milestone 36, reworked Milestone 88, extended Milestone 91, decline option + third leg Milestone 92, fourth leg + repoint Milestone 95)

Some locations (Ice Wall Castle, Sancrist Isle) are sea-locked — confirmed
by direct inspection of the reference map, no land route exists — so
reaching them needs some way past ocean, which `world::Terrain` marks
hard-`passable = false`, same as the Blood Sea.

**Milestone 36's original shape** (kept here for history, superseded
below): ocean tiles gained a `crossableByBoat` terrain flag and
`GameState` gained a permanent `bool hasBoat`, so `tryMoveOverworld` let
the player cross *any* ocean tile anywhere on the continent forever, once
granted. Milestone 88 found this didn't match the source material (the
novels describe one specific ship's route, never general open-world
sailing) and that it let the player treat the entire ocean as a shortcut
regardless of where they stood — so it was replaced outright.

**Current shape: a talk-triggered location jump, not a passability rule.**
Ocean and the Blood Sea are simply impassable, exactly as they were before
Milestone 36 — `world::TerrainInfo` carries no boat-related field at all
now. A zone `POI` can instead carry `BOAT <char> <destination-location-id>
<hours>` (`ZoneLoader`, same must-already-have-a-`TALK`-line validation as
`SAY_IF`/`TOPIC` — see `docs/ZONE_NOTES.md`). This carries an id payload
needing cross-file validation (the destination must be a real
`world::Location`), so — like `PORTAL`'s target zone id or `QUEST`'s quest
id — it lives in its own `Zone`-level map (`world::Zone::boatVoyages_`,
exposed via `boatAt`/`boatVoyages()`), not on `PointOfInterest`, and is
validated where a loaded `World` exists: `ZoneCatalog::loadForWorld`,
which throws if the destination id doesn't resolve to a real location
(same spot/style as its existing `PORTAL` target check).

`TalkCandidate` (`GameLoop.h`) carries `boatDestinationId`/`boatHours`
instead of a bool. `data/zones/tarsis.txt`'s Knight's Runner grants
`BOAT R ice_wall 48` (a ~2-day voyage, invented for pacing like every
`minutesToCross` value already is, not a sourced figure). Milestone 91
added a second leg on the same mechanism: `data/zones/ice_wall.txt`'s
"An Ice Barbarian Guide" (`B`) grants `BOAT B sancrist_isle 48` — this one
*is* a sourced figure (`.research/dwn_full.txt` lines 5919-5920: "if the
winds held, they might make Sancrist in two days"), a coincidence with the
first leg's invented number, not a copy of it. Milestone 92 added a third:
`data/zones/sancrist_isle.txt`'s "An Embarkation Officer" (`E`) grants
`BOAT E palanthas 96`, sourced from the same novel's account of Sturm's
army sailing from Sancrist to defend Palanthas and the High Clerist's
Tower (`.research/dwn_full.txt` lines ~10628-10731) — no explicit
day-count exists for this leg, so 96 hours is invented-for-pacing, chosen
longer than the two 48-hour legs since it's a materially longer crossing.

**Milestone 95 repointed the second leg and added a fourth.** Fresh
sourcing found the Ice Wall -> Sancrist crossing doesn't arrive safely in
the source text — a white dragon wrecks the ship on Southern Ergoth's
coast partway there (`.research/dwn_full.txt` lines 6095-6224, continuous
with the same voyage Milestone 91 already cited). `data/zones/
ice_wall.txt`'s "An Ice Barbarian Guide" (`B`) now grants `BOAT B
southern_ergoth 48` instead of `BOAT B sancrist_isle 48` — same POI char,
so `voyagesTaken`'s `"ice_wall:B"` key and any save that already recorded
it as taken are unaffected; only the destination changed. A new fourth
leg, `data/zones/southern_ergoth.txt`'s "A Silvanesti Sentry" (`S`),
grants `BOAT S sancrist_isle 60`, closing the route back to the
already-shipped Sancrist Isle content. The chain is now five POIs across
four locations: Tarsis -> Ice Wall -> Southern Ergoth -> Sancrist Isle ->
Palanthas. See `docs/TIMELINE_NOTES.md`'s "Southern Ergoth" section for
the full sourcing and `docs/ZONE_NOTES.md`'s "Boats" section for the
POI-level detail.

**Decline option (Milestone 92).** The original shape above executed the
voyage unconditionally on first talk — no way to say no, despite the
Runner's own `TALK`/`TALK_AGAIN` lines in `data/zones/tarsis.txt` clearly
being written to expect a choice ("waiting on your answer" / "if you've
changed your mind"). Gating this on `GameState::alreadyMet` couldn't
support a real decline: that set is inserted into on *every* talk
(accepted or not), so a declined offer would immediately read as
`alreadyMet` on the next visit and could never be re-offered. Fixed with
its own state, `GameState::voyagesTaken` (same `"<zoneId>:<POI char>"` id
shape as `metCharacters`, persisted as a new `VOYAGED <count> <id>...`
save line, same shape/position as `VISITED`/`MET`) — a POI is offered
until its id is actually in `voyagesTaken`, not until it's merely been
talked to. `talkTo` now shows a `drawPickerFrame` Board/"Not yet" choice
(mirroring `offerOrTurnInQuest`'s Accept/Decline picker), gated on
`voyagesTaken` instead of `alreadyMet`. Boarding inserts into
`voyagesTaken` and runs the jump exactly as before; declining does nothing
and falls through to the ordinary topics/`SUBJECT` picker below instead of
returning — this is what finally makes the Runner's already-written
`derek,knights`/`dragons,ship` `SUBJECT` lines reachable (they were dead
content before, since the unconditional jump never let execution reach
that code).

**Direction fix (Milestone 92).** The travel log line hardcoded "you board
the ship, and it carries you **south** across open water" for every
voyage — actually wrong even for the two legs that shipped before this
milestone (Tarsis → Ice Wall is southwest; Ice Wall → Sancrist is
northwest, since Sancrist sits much further north on the grid) and would
have been wrong again for the new northeast-bound Sancrist → Palanthas
leg. Now computed via the existing `compassDirection(dx, dy)` helper
(`GameLoop.cpp`, already used by `lookOverworld`), from the zone's
overworld position to the destination's, before `state_.x/y` are
overwritten by the jump.

**Save compatibility.** `hasBoat`/the `BOAT <0/1>` save line are gone;
`game::SaveGame` still recognizes the `BOAT` keyword on load and discards
it, so a save written before Milestone 88 still loads cleanly instead of
fail-fasting on an otherwise-valid file. `VOYAGED` (Milestone 92) is
optional the same way `MET`/`QUEST`/`KILL` are — a save written before
this milestone simply has no line, and an empty `voyagesTaken` is the
correct default. **Known accepted edge case**: a character who already
boarded a given voyage under the pre-Milestone-92 code has an empty
`voyagesTaken` for it (the id was never recorded that way), so if they
deliberately walk back to that same POI, they'd be re-offered the voyage
instead of it reading as permanently spent. Not solved with migration
logic — reaching a past-departure POI again requires walking back across
the whole map, and re-accepting just re-runs an already-real jump, no
corruption.

**Scope note (Milestone 88, closed by Milestone 91, extended by
Milestone 92):** Milestone 88 converted only the Tarsis → Ice Wall Castle
leg, since Sancrist Isle relied on the same global `hasBoat` and had no
scripted voyage of its own once that flag was removed — see
`docs/MAP_NOTES.md`'s "Sancrist Isle reachability gap." Milestone 91
closed the *arrival* half of that gap with the second `BOAT` grant above,
but left Sancrist Isle with no talkable NPC and no way out — Milestone 92
closed the *departure* half with the third leg. Sancrist Isle also remains
reachable on foot via the coastal shallow-water (`r`) path (confirmed by a
throwaway BFS against the real grid) — the scripted voyage is a shortcut,
not the only route.

**No HUD indicator and no random-encounter risk from this at all now** —
since ocean is plain impassable terrain again, its `minutesToCross`/
`encounterChancePercent` are inert values, same as the Blood Sea's.

## Frameless overworld/zone layout + NPC "Look" (Milestone 43)

Prompted by a second reference screenshot (a different terminal RPG,
"Bob's game") the user asked to match: no outer box border, a single-line
header instead of a 2-line HUD, rule dividers (`=`/`-`) instead of
`+--+`/`| |`, and a labeled status panel (mode, current location,
coordinates, stats, a headed action log) instead of a raw log tail
sharing the frame with the HUD. Bundled with an unrelated but
overlapping-scope request: an NPC's full description should no longer
auto-print to the log on arrival — just their name — with the
description now revealed on demand via Look (`;`), and Look should offer
a picker when more than one NPC is present, mirroring `pickAndTalk`.

**Scope boundary, stated plainly**: only `drawOverworldFrame` and
`drawZoneFrame` changed shape. `drawCharacterSheet`/`drawCombatFrame`/
`drawShopFrame`/`drawInventoryFrame`/`drawPickerFrame`/
`drawDialogueFrame`/`drawLogFrame` (and the `writeBorder`/`writeBoxed`
helpers they share) keep the Milestone 32 bordered-box look, unchanged —
same "only the two frames the reference actually depicts" precedent
Milestone 29 already established. Pressing `t`/`c`/`p`/`i`/`v` now pops a
bordered box over what was just an edge-to-edge frame; accepted as
consistent with that precedent, not revisited here.

**The 2-line HUD (`writeHud`) is gone, split across two places instead of
one.** `writeHeaderLine` (a new `MapRenderer.cpp` free function) builds a
single line: the title on the left, `<name> (<class>)   HP [bar]
cur/max   Day D, Hour H` right-aligned. AC/THAC0/Steel didn't disappear —
they moved into the status panel (below), on two lines rather than one,
because a single combined `"AC n   THAC0 n   Steel: n stl"` line measures
~33 characters, wider than the panel's own minimum width; splitting it
avoids silently truncating the steel figure on a small terminal.

**`MapRenderer::buildStatusPanel`** replaces the old bare log panel with a
fixed 12-row header (`MODE: EXPLORING`/`INDOORS`, blank, `Standing On:`,
the location/zone name, blank, `Position: (x, y)`, blank, the AC/THAC0
and Steel lines above, blank, `ACTION LOG:`, blank) followed by
`buildLogPanel`'s scrolling tail filling whatever rows remain — 18 of 30
at the preferred size, down from all 30 before. The full log is never
lost (`handleLog`/`drawLogFrame`, the `'v'` full-history pager from
Milestone 31, is unaffected and unchanged), just less of the live tail is
visible at once; accepted as the direct, known cost of the richer panel.
`MODE`/the standing-on value are colored (`colorLine`, a new helper: pads
to width *then* wraps in an ANSI set/reset pair, so the result is still
exactly `width` visible columns by construction, same contract the
map-glyph rows already followed) — cyan for `MODE`, the same bright
yellow already used for location/POI glyphs for the standing-on value, so
the color is reused meaningfully rather than picked arbitrarily.

**`buildLogPanel` gained a `"> "`/`"  "` prefix**, matching the
reference's action-log convention: each raw entry wraps to `width - 2`
(reserving the prefix columns) instead of `width`, the first physical
line gets `"> "`, continuation lines get `"  "`. A literal empty entry
(the blank-spacer lines `GameLoop::pushLog("")` already pushes between
arrival blocks) stays a plain blank row with no prefix, special-cased
before wrapping. `buildLogPanel` has exactly one caller family
(`buildStatusPanel`, itself only called from the two restyled frames) —
`drawLogFrame`'s full-history pager does its own separate wrapping and
was confirmed unaffected.

**Constants**: `kChromeRows` 7→4 (header + `=` rule + `-` rule + footer;
the rules are the separators now, no more blank spacer rows).
`kChromeColumns` 4→1 (was 2 border chars + 2 padding; kept at 1, not 0,
as cheap insurance against an off-by-one in the detected terminal width —
this project has already been bitten once by a console-sizing edge case,
Milestone 33's `dwSize`-vs-`srWindow` bug). `kLogPanelGap` 2→3, now the
width of the literal `" | "` divider drawn between map and status panel,
not a bare gap. `kMinLogPanelWidth` 20→24 — the panel now has to fit real
fixed content, not just wrapped prose: "High Clerist's Tower" (23 chars)
and "Inn of the Last Home" (21 chars) both overflow a 20-column floor.
`kAbsoluteMinColumns`/`kAbsoluteMinRows` recompute from these
automatically, 70×23 → **72×20**.

**NPC announce/Look, the other half of this milestone.**
`announceOverworldTile`/`announceZoneTile` (Milestone 30) used to push a
present NPC's full flavor text/description straight to the log
(`name + ": " + text`). As of Milestone 43 that's just `name + " is
here."` for anything that counts as an NPC — a present canon character
(always), or a zone POI with non-empty `dialogue` (the same test
`handleTalk` already uses to distinguish an NPC POI from scenery; a
scenery POI, empty `dialogue`, is unaffected and still prints its full
description immediately, since Look gives it no other way to be
revealed). The withheld text resurfaces through a new
`GameLoop::pickAndLook(candidates)`, the read-only sibling of
`pickAndTalk` (`LookCandidate{name, description}` instead of
`TalkCandidate{id, name, Speech}` — Look never touches
`GameState::metCharacters` or grants anything): 1 candidate shows its
description directly via `drawDialogueFrame`; 2+ run the identical
nested-loop `drawPickerFrame("Look at whom?", ...)` picker `pickAndTalk`
already established, just deciding what to *show* rather than who to
*talk to*. `lookOverworld`/`lookZone` gather candidates the same way
`handleTalk`'s two branches do (present canon characters via
`Timeline::presentAt`, plus — zone only — the current POI and any
`TIMELINE_ANCHOR` presence), but **deliberately unfiltered** by
`window->dialogue.empty()` (unlike `handleTalk`'s own filter): Look
should reveal a presence window's flavor text even for a window with no
`SAY` line authored yet, matching how the old announce code iterated
unfiltered too. Only when nobody's present do the two functions fall
through to their pre-Milestone-43 behavior unchanged — the nearest-
landmark compass search on the overworld, the "nothing else catches your
eye" stub in a zone.

## Quest system (Milestone 51)

The world had places, people, and monsters, but nothing that gave the
player a goal — this milestone's answer, built as glue over what already
existed rather than a new subsystem. Full design writeup, grammar, and the
turn-in state table: `docs/QUEST_NOTES.md`. The short version, for how it
fits the rest of this document's shape:

A new `quest/` module (`Quest.h`/`Quest.cpp`/`QuestLoader.cpp`) joins
`world/`/`timeline/`/`character/` as a fourth leaf with no in-project
dependencies — static content, loaded fresh every run from
`data/quests.txt`, never mutated. `game::GameState` gained two flat fields
following the `hasBoat`/`metCharacters` precedent exactly: `quests` (id ->
`QuestStatus`, absence meaning "not started") and `monsterKills` (a
lifetime tally `GameLoop::runCombat` increments on every kill). Both round-
trip through the save file as new optional lines, the same
backward-compatible-by-default shape `BOAT` and `RESTDAY` established.

The evaluation logic (`objectiveProgress`/`objectiveMet`/
`allObjectivesMet`) lives in `game::GameLoop.cpp`, not `quest/` — the same
split `game::conditionMatches` already established for `SAY_IF`: the
static content type stays ignorant of `character::Character`/`GameState`,
and `game/` is where the two actually meet. Turn-in itself hooks into
`GameLoop::talkTo` in the exact slot the `grantsBoat` precedent (Milestone
36) occupies — after `metCharacters.insert`, before the topic-picker loop
— so a quest-giver POI's `QUEST <char> <quest-id>` behaves like one more
layered ability on top of an existing `TALK`ed POI, the same shape
`SHOP`/`BOAT`/`BED`/`TIMELINE_ANCHOR` already use (see
`docs/ZONE_NOTES.md`).

New UI: a `render::Key::Journal` bound to `'g'` (`'j'`/`'q'`/`'l'` were all
already taken — see `docs/GOTCHAS.md`) opens `drawJournalFrame`, a
one-keypress-block screen in the `showCharacterSheet`/`showHelp` shape,
not a nested loop.

**Follow-up, same milestone's playtest**: the user asked for a proactive
"quest complete, go collect your reward" notification rather than having
to guess and check the journal. `QuestStatus` gained a third,
append-only-safe value (`ReadyToTurnIn`); see this document's "What's
deliberately NOT abstracted yet" section above for why the fix
(`GameLoop::checkQuestReadiness`, four fixed call sites) is not the event
bus it might sound like, and `docs/QUEST_NOTES.md`'s "Proactive readiness
notification" for the full mechanism.

## Character creation redesign: screen-per-step, colorized (Milestone 69)

Prompted by two reference screenshots — `References/abilityscore.png`
(a "STEP 1: ABILITY SCORES" wizard screen from another terminal RPG) and
`References/Bobs_Game.png`, the same "Bob's game" already cited above for
Milestone 43's overworld/zone color palette. `CharacterCreator.cpp`
gained per-step cleared screens (`STEP n: TITLE`) and that same ANSI
palette (`\x1b[93m` yellow, `\x1b[96m` cyan, `\x1b[97m` white, plus one
new code, `\x1b[92m` bright green, for assigned/confirmed values) —
reused verbatim from `MapRenderer.cpp`, not a new palette invented for
this one file. Full design rationale (including the dice-method change
this triggered — PHB Method V, superseding a previously documented
Method I decision) is in `docs/CHARACTER_NOTES.md`'s "Ability score
generation" and `docs/MILESTONES.md`'s Milestone 69 entry; the two things
worth knowing before touching this file again:

**Still plain `std::cin`/`std::cout`, on purpose.** The "Character
creation" section above explains why: it's the one part of the game a
piped/redirected script can drive end-to-end. The reference screenshot's
live-highlighted arrow-key cursor was adapted to a numbered-list
`promptChoice`, the same interaction the race/class/alignment menus
already used — no live-cursor rendering was added, and none should be
without first solving the same piped-testability problem
`docs/GOTCHAS.md` already documents for the real `render::Console::readKey()`-driven
game loop.

**Screen-clearing is inline, not a `render::Console` call.** Both
`CharacterCreator.cpp` and `MapRenderer.cpp` now emit the same raw
`\x1b[2J\x1b[H` VT100 sequence directly rather than the former calling
`render::Console::clearScreen()`. This keeps `character/` at zero
dependencies on `render/`, preserving the one-way dependency graph in
the "Module map" section above — a real constraint this file's own
"Character creation" section documents `CharacterCreator` was designed
around already. Safe because `render::Console`'s constructor (which
enables `ENABLE_VIRTUAL_TERMINAL_PROCESSING` on Windows) always runs at
the top of `main()`, before `CharacterCreator::run()` is ever called.

## Colored dialogue/picker/combat screens, and re-picking after a talk (Milestone 73)

Two small, unrelated-but-overlapping-scope changes to `GameLoop::pickAndTalk`
and the "organic" (bordered) screens, requested together.

**The "Bob's game" palette (`\x1b[93m` yellow, `\x1b[96m` cyan, `\x1b[97m`
white — already used by `drawOverworldFrame`/`drawZoneFrame`'s status panel
since Milestone 43, and by `CharacterCreator` since Milestone 69) now also
reaches `drawDialogueFrame`, `drawPickerFrame`, and `drawCombatFrame`** —
the three "organic" screens a player actually sees while talking to someone
or fighting. `drawCharacterSheet`/`drawSpellbookFrame`/`drawShopFrame`/
`drawInventoryFrame`/`drawJournalFrame`/`drawHelpFrame`/`drawAskInputFrame`
deliberately stay plain, unchanged — narrower scope than "every organic
screen," matching what was actually asked.

**Why a new `BoxLine` type instead of coloring the existing
`std::vector<std::string>` pipeline in place**: `colorLine` (Milestone 43)
only ever colors an already-*exactly-width* line — it pads first, then
wraps the padded result in a set/reset pair, so the visible width never
changes. `writeBoxed`'s existing pipeline measures each raw line's width
*before* padding (to size the box), and `wrapText`'s word-wrap also counts
by raw character length. Feeding a pre-colored string through either would
miscount invisible escape bytes as visible ones — the exact trap
`writeBoxed`'s own comment already warns about for the map/log rows. `struct
BoxLine { std::string text; const char* color = nullptr; }` sidesteps this:
a second, parallel `wrapLongLines`/`writeBoxed` overload wraps/measures/pads
`BoxLine.text` exactly like the plain overload always has, and only applies
`colorLine` (color-after-padding) when assembling the final exact-width
`exact` vector, for any line carrying a non-null color. The original plain
`std::vector<std::string>` overloads are untouched, so every screen not
listed above needed zero changes.

**Where color landed, and why each choice**: `drawDialogueFrame` colors
the speaker's name — on its own line, not a colored `"Name: text"` prefix,
since `BoxLine` can only color a *whole* line safely (same constraint as
above) — bright yellow, reusing the "named thing in the world" meaning
`colorLine` already gave `Standing On:` and location/POI glyphs, rather
than picking a new color arbitrarily. `drawPickerFrame` colors the
selected row's cursor+text bright white, matching the player's own `@`
glyph's "must always read clearly" convention — since every "talking to
someone" picker (Talk to whom?, Ask about..., quest Accept/Decline) and
Look's own picker share this one function, all of them picked this up for
free, not just the ones the request named. `drawCombatFrame` colors the
player's stat line bright white (same "this is you" convention as the
picker cursor) and the monster's stat line bright red (`\x1b[91m`, not
previously used anywhere in the project) — a genuinely new color, since
nothing existing meant "hostile." Combat's own scrolling log lines stay
plain, matching the precedent that the main screen's action log entries
are plain too (only labels/named-things are colored, never free-form
prose).

**Verified via the throwaway self-test pattern**, not by eye: a
`ColorSelfTest.cpp` calling the three changed functions with sample data,
piped to a file and inspected with escape codes visible (`cat -v`) —
confirmed every set code is immediately followed by `\x1b[0m` before the
line's trailing padding (see `docs/GOTCHAS.md` on color bleed) and that
every bordered row's closing `|` still lines up column-for-column across
colored and uncolored lines in the same box. Deleted after use, per
CLAUDE.md. Real in-terminal color rendering (as opposed to correct escape-
code structure) still needs the user's own eyes — same limitation as every
prior color/layout milestone.

**`pickAndTalk`'s "Talk to whom?" loop no longer exits after one
conversation.** Previously, picking a name, finishing that conversation
(however it ended — the topic menu's "Nothing, thanks"/`q`, or no topics at
all), and returning from `talkTo` fell all the way back to the explore
screen, even when other candidates were still standing right there —
talking to a second present NPC meant re-pressing `t` and re-triggering
`handleTalk`'s whole candidate-gathering query. Fixed by dropping the
`return` after `talkTo(candidates[selected])` inside the picker's `Enter`
branch: the loop simply redraws the same "Talk to whom?" picker afterward.
`q`/`Quit` at that picker (not reached mid-conversation) is unchanged and
still returns to explore. One line removed, `pickAndLook`'s equivalent loop
was deliberately left alone — the request was specifically about talking,
and Look has no multi-step conversation to return from.

## Widening the palette to the remaining organic screens (Milestone 81)

The seven screens Milestone 73 deliberately left plain narrowed to one:
character sheet, spellbook, shop, and inventory (`drawCharacterSheet`/
`drawSpellbookFrame`/`drawShopFrame`/`drawInventoryFrame`) and journal/help
(`drawJournalFrame`/`drawHelpFrame`) were converted from the plain
`std::vector<std::string>`/`writeBoxed` overload to the `BoxLine`/`writeBoxed`
overload Milestone 73 introduced, picking up color strictly from the palette
that already existed — no new color, no new meaning invented:

- A new named constant, `kSectionLabelColor` (`"\x1b[96m"`, the same bright
  cyan `buildStatusPanel` already used inline for `MODE:`), formalizes
  "fixed section/category label" as a third reusable meaning alongside
  `kNpcNameColor` ("named thing") and `kSelectedItemColor` ("this is you/
  selected"). Applied to: the character sheet's `Saving Throws:` header,
  the spellbook's `Level N (...):` headers, the shop's `-- Buying --`/
  `-- Selling --` headers, the inventory's `Carried items:` header, the
  journal's `Completed:` header, and the Help screen's three category
  headers (`Movement:`/`Overworld / zone:`/`Combat:`).
- The shop and inventory screens' selected cursor row (`> ...`) now colors
  bright white via `kSelectedItemColor` — a direct reuse of
  `drawPickerFrame`'s existing cursor convention, since both screens are the
  same "cursor list" shape.
- Journal quest titles color bright yellow via `kNpcNameColor` — the
  clearest "named thing" analog outside an NPC/location name.
- Everything else on these screens (ability scores, HP/AC/THAC0, weapon/
  armor lines, spell lists, item labels, footers, free-form messages) stays
  plain, matching the "only label sections, named things, and the
  selection — never free-form prose" restraint Milestone 73 established.

`drawAskInputFrame` was deliberately left unchanged: both its lines are
short instructional prose with no line that's purely a section label, a
named thing, or a selected row, and `BoxLine`/`colorLine` still only ever
color a *whole* padded line (Milestone 73's constraint, unchanged) — so
tinting just the NPC's name inline would mean restructuring the sentence,
out of scope for "widen the existing palette to existing lines."

**Verified the same throwaway self-test way as Milestone 73**: a
`ColorSelfTest2.cpp` calling all six changed draw functions with sample
data (a Mage and a Knight-of-the-Crown Fighter for the sheet, a shop in
both buy/sell mode with a selected row, an inventory with a selected
potion, a journal with one active and one completed quest), piped to a
file and inspected with `cat -v` — confirmed every set code is immediately
followed by `\x1b[0m` and that bordered rows stay aligned column-for-column
between colored and plain lines. Deleted after use, along with the
temporary CMake target, per CLAUDE.md. Real in-terminal rendering still
needs the user's own eyes, same limitation as every prior color milestone.

## Party companions: a small first step, then real combat, then a real roster (Milestones 116-118)

`docs/COMBAT_NOTES.md`'s "Extending this later" section had long flagged "a
party of up to six characters" as the single biggest remaining gap between
this project and a real Gold Box game, but also warned it wasn't proposed
lightly — `game::GameLoop::runCombat` is one large function already built
entirely around a single `character::Character`, and `combat::
resolvePlayerAttack`, `render::MapRenderer::drawCombatFrame`, the HUD, and
`CharacterCreator` all assume exactly one PC exists. Rather than attempt the
whole thing (deployment order, front/back lines, `UIC` NPC control, thief
backstab, fighter "sweep") in one pass, this milestone deliberately shipped
the smallest real slice first: **one recruitable companion, with no combat
integration at all** — the same "ship the smaller half first" precedent
Milestone 113 (monster *groups*) set before Milestone 114 (the *grid*).

**The companion is a real `character::Character`, not a parallel struct.**
This is what lets a later combat-integration phase reuse `combat::
resolvePlayerAttack`/`render::MapRenderer::drawCombatFrame` unchanged, since
both already operate on `const character::Character&`.

**Deterministic base, mutable HP.** New sibling module `character/
Companion.h`/`.cpp` (depends only on `character/`, same independence as
`Leveling`/`Spellcasting`/`Equipment`/`WizardOrder`) exposes two builders --
`character::buildCompanion()` (Bren Alder, a level-1 Fighter) and
`character::buildDessaCorrin()` (a level-1 Thief, added at Milestone 118) --
plus `character::buildCompanionById(id)`, which dispatches to the right one
and fails fast on an unrecognized id, same idiom as every other loader in
this project. Both builders use fixed ability scores and starting steel
(deliberately never `character::roll`), reusing the same non-interactive
rules functions `CharacterCreator::run()` already calls (`classInfo`,
`hpAdjustmentForConstitution`, `recomputeArmorClass`,
`applyRacialSavingThrowBonus`). Calling either twice always produces
identical stats *except* `currentHp`, which combat can change (Milestone
117) — `game::SaveGame` therefore persists one `COMPANION <id> <currentHp>`
line per recruited companion; every other field still comes from
`buildCompanionById(id)` on load. A pre-Milestone-117 save has only the
one-token `COMPANION 1` line (the literal token `"1"`, mapped to id
`"bren_alder"`) and still loads correctly, defaulting to full health — see
`docs/GOTCHAS.md`.

**A real roster as of Milestone 118**: `GameState::companions` is a
`std::vector<game::RecruitedCompanion>` (each entry pairing a save-format
`id` with a full `character::Character`), replacing Milestone 116/117's
single `hasCompanion`/`companion` fields. There's no hardcoded numeric cap
— the roster's real size is bounded by how much content exists (two
companions today: Bren Alder at Solace, Dessa Corrin at Haven), not a
`kMaxPartySize` constant, matching this project's "no premature
abstraction" rule. **No dismiss mechanic** (once recruited, a companion
stays) and **no map glyph/independent position outside combat** (companions
are narratively "with you" between fights — character sheet and HUD only,
never a second tile on the overworld/zone grid) are both still deliberate
omissions, not oversights. *Inside* combat, every companion gets a real
position — see below.

**Recruiting reuses the existing "TALK layers an ability onto a POI"
pattern, now with a real id payload.** A `RECRUIT <char> <companion-id>`
zone-grammar line (`world::PointOfInterest::recruitCompanionId`, same "must
already have a TALK line" validation `BOAT`/`GRANTS_ITEM`/`QUEST` already
enforce — see `docs/ZONE_NOTES.md`) marks a POI as offering to recruit a
specific companion. Whether the id is real is cross-checked by `main.cpp`
against `character::isKnownCompanionId` once all zones are loaded (`world::`
can't see `character::` directly, so `ZoneLoader` itself can't validate
this — the same deferred-cross-check shape `QUEST`/`SHOP_LOCKED` ids already
use). `GameLoop::talkTo`'s recruit block is gated per-companion-id (is
*this* id already somewhere in `state_.companions`) rather than a single
Milestone-116-vintage bool, so a declined offer stays re-offerable on a
later visit, an accepted one never re-offers, and Bren Alder and Dessa
Corrin can each be recruited independently, in either order.

**Combat, as of Milestone 117/118: every companion is a real, AI-controlled
combatant.** `GameLoop::runCombat` reads and mutates `GameState::companions`
directly — the touch that first broke `runCombat`'s old single-`Character`
assumption, made possible because `combat::resolvePlayerAttack`/
`resolveMonsterAttack`/`rollSavingThrow` already took a generic `const
character::Character&` (Phase 1's design doc predicted exactly this payoff
from "the companion is a real Character, not a parallel struct"). Each
companion gets its own `combat::GridPos` on the same tactical grid the
player and monsters already occupy (Milestone 114), starting adjacent to
the player in a small alternating left/right pattern. Each round, right
after the player's own action (not a separate initiative roll — see
`combat::playerActsFirst()`, unchanged), a `companionActs()` lambda loops
every companion in roster order and resolves each one automatically: attack
an adjacent alive monster instance, or take one `combat::stepToward` step
toward the nearest one (`combat::chebyshevDistance`, added at Milestone
117). Monster AI (`monstersAct()`) builds a small local `PartyTarget` list
(the player plus every alive companion) each turn and picks among whichever
are adjacent — exactly one adjacent, attack that one; more than one
adjacent, pick uniformly at random via `character::roll(1, N)` (this is the
direct generalization of Milestone 117's player/companion coin-flip); none
adjacent, path toward whichever is nearest. `render::MapRenderer::
drawCombatFrame` reuses the existing `CombatMonsterView` struct as a
presentation-state carrier for companions (not because they ARE monsters)
— a `std::vector<CombatMonsterView>` (was: a single optional pointer)
threaded through every one of `runCombat`'s draw call sites via a
`companionViews()` lambda that rebuilds it fresh from live state on every
redraw, same "no caching" precedent `buildViews()` already sets for
monsters; each companion gets its own combat glyph (`'c'`, `'d'`, ...) so
more than one reads clearly on the grid at once.

A knocked-out companion (HP <= 0) is removed from acting/being targeted for
the rest of that one fight (every AI decision above gates on a
`companionAlive(i)` check) but does **not** end the fight — only the
player's own knockout does that, unchanged. When the player's own knockout
*does* end the fight (a party wipe), `knockedOutBy` restores every
recruited companion to full HP right alongside the player's own "struck
down... wake up back in <refuge>" recovery — a Milestone 118 same-session
bug fix; before it, a companion knocked out during a losing fight stayed at
0 HP indefinitely (silently benched from every fight until the next Rest/
BedRest), since only the player was being healed on that path.
`GameLoop::handleRest`/`handleBedRest` heal every companion the same amount
they already heal the player, with no separate `lastRestDay` gate of its
own (a companion only ever rests when the player does).

**What's deliberately still deferred**, honestly flagged rather than
overlooked: the Brooch of Imog's globe wards the player only (it's the
player's own item); Bozak's Magic Missile and Aurak's breath weapon stay
hardcoded to always target the player (teaching them to pick among the
whole party is real extra scope, the same family as the already-deferred
"no square-cursor for AoE" gap in `docs/COMBAT_NOTES.md`); Sivak's
death-burst still only damages the player regardless of who lands the
killing blow (a simplification that predates companions, not extended
here); companion movement never provokes or takes opportunity attacks; and
there's no "finish off a downed ally" mechanic. Thief backstab and
Fighter sweep — both needed a second party member and both apply
identically to the player and to companions — shipped at Milestone 119,
see `docs/COMBAT_NOTES.md`'s "Thief backstab and Fighter sweep attacks".
Player-directed control of companions (a UIC-style toggle — DQoK.pdf's
own manual: "You control the actions of PCs. The computer controls the
actions of monsters, NPCs, and PCs set to computer control with the UIC
command") and deployment order still stay reserved for a later phase —
see `docs/COMBAT_NOTES.md`'s "Extending this later".

## Extension points for later milestones

These are the seams intentionally left in the code so later systems can
attach without reworking it:

- **Leveling past level 20**: `Leveling.cpp`'s XP tables now cover the
  full 1–20 the PHB prints for these classes — see `docs/CHARACTER_NOTES.md`.
- **Fighter's extra attacks per round** (level 7+, PHB Table 15): needs
  `GameLoop::runCombat`'s round loop restructured to resolve more than one
  attack per side.
- **Spellcasting past one known spell each**: a real spellbook/spell-
  selection system, spells above 1st level, and Wizard Robe spell-sphere
  restrictions (moot today since the one Mage spell isn't sphere-
  restricted) — see `docs/CHARACTER_NOTES.md` and `docs/COMBAT_NOTES.md`.
- **Equipment past the General Store's short list**: a real carried-item
  inventory landed in Milestone 21 (see `docs/CHARACTER_NOTES.md`'s
  "Carried inventory and equip/unequip" section) — still open: selling
  gear back, more shops at other zones (Haven's market stalls and
  Tarsis's old dock are natural candidates — see `docs/ZONE_NOTES.md`),
  armor weight/encumbrance, an explicit "unequip to nothing" action, and
  items beyond armor/shield/weapon.
- **Reactive dialogue/topics, as of Milestone 26**: `SAY_IF`/`TOPIC` now
  exist at all 6 of the 8 Heroes' stops, including Haven (see
  `docs/TIMELINE_NOTES.md`'s "Widened to Haven" note) — though not every
  Hero got new content at every stop (never force-fit without real
  grounding). Zone-native NPCs also gained their first `SAY_IF`/`TOPIC`
  support this milestone (`ZoneLoader`'s parser, `PointOfInterest`'s
  fields, and `GameLoop::handleTalk`'s zone branch — see
  `docs/ZONE_NOTES.md`'s "NPCs: POIs you can talk to"), though only 5 of
  the zone-native NPCs have content authored so far (Otik, Tika, the
  Seeker Guard, the Forestmaster, the Fortress Guard). The condition
  vocabulary itself (`game::conditionMatches`) is still easy to extend if
  a richer reaction is ever needed (e.g. a specific subrace, or robe
  status) — a `knight` condition was added this way at Milestone 51,
  specifically so a Knight-of-the-Sword quest's `REQUIRE` wouldn't need a
  grammar change later.
- **Quests past `VISIT`/`TALK`/`SLAY`**: `DELIVER` (carry/hand over a
  specific item) was scoped out of Milestone 51 because it needs a real
  quest-item concept `character::InventoryItem` doesn't have yet — see
  `docs/QUEST_NOTES.md`'s cut list. Item rewards beyond steel/XP are in
  the same position. Real quest *content* beyond the one proof-of-concept
  quest (`road_wolves`) is also open — see `docs/QUEST_NOTES.md`'s
  "Extending this later".

## What's deliberately NOT abstracted yet

No plugin system, no generic "event" bus, no data-driven scripting layer,
no *live* console-resize handling (Milestone 33's `MapRenderer::
configureLayout` sizes the viewport to the real console once at startup,
but a resize mid-session doesn't reflow anything until restart — see the
Milestone 33 section above and `docs/GOTCHAS.md`). Those would be
premature — the systems that would need them don't exist yet. When a
milestone that actually needs one of these is built, revisit this
document and update it to describe what
shipped, not just what was planned.

**Milestone 51's quest system is deliberately not an event bus, even
though "a quest tracks progress toward things happening elsewhere" sounds
like it wants one.** An objective is fundamentally a pull, not a push:
`objectiveMet` queries `visitedLocations`/`metCharacters`/`monsterKills`
on demand — there's no generic "you moved" / "you talked to X" / "you
killed Y" event any quest can subscribe to. Real playtesting surfaced a
real gap this created, though: with no push at all, the player had no way
to know a quest had become turn-in-ready short of guessing and walking
back to check. The fix (`GameLoop::checkQuestReadiness`, see
`docs/QUEST_NOTES.md`'s "Proactive readiness notification") is a *poll at
the small, fixed set of places state already changes* — the same three
mutation points `visitedLocations`/`metCharacters`/`monsterKills`
themselves live at, plus accepting a quest — not a new generic
subscription mechanism. Four call sites, hand-written, each one line.
Still not the event bus this section has said "not yet" to for 50+
milestones running: nothing here lets a quest (or anything else) declare
an arbitrary interest and get called back for it generically — the day
something *does* need that, this paragraph is the place to update.
