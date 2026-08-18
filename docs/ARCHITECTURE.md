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
`timeline/`, `character/`, `combat/`, and `render/`; `render/` depends on
`world/` (it needs to read terrain/location data to draw it), `timeline/`
(to query who's present), `combat/` (`drawCombatFrame` takes a `Monster`),
and `game/GameState` (it needs to know where the player is) but never
mutates any of them; `combat/` depends only on `character/` (`Dice` for
rolls, `Character`/`Ability` for the player side of an attack); `world/`,
`timeline/`, and `character/` depend on nothing else in the project. This
means those three, `combat/`, and `render/` can each be understood, tested,
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

**Format**: a hand-rolled keyword-per-line text file (`save.txt`, path baked
in via `ANSALON_SAVE_FILE`, same pattern/limitation as `ANSALON_DATA_DIR` —
see `docs/GOTCHAS.md`), following the same `trim`/`splitKeyword`/fail-fast
idioms as `WorldLoader`/`ZoneLoader`. `character::RaceId`/`ClassId`/
`Alignment` are stored as raw enum ints rather than names — see
`docs/GOTCHAS.md` for the fragility that trades off against.

**When it saves**: `GameLoop::run()` calls `SaveGame::save` once per loop
iteration, unconditionally, right after the input switch (including on
`Quit`, which now sets a local flag and falls through to the save instead of
returning directly) — see the loop in `GameLoop.cpp`. Simpler than tracking
"did this specific key mutate state," and cheap enough not to matter: a
crash or an ungraceful close loses at most the single most recent keypress.

**When it loads**: `main.cpp`, before deciding whether to run
`CharacterCreator::run()` at all. If `save.txt` exists, it's loaded and the
player is asked (plain `std::cin`/`std::cout`, same interaction mode as
`CharacterCreator` — see above) whether to continue that character; only a
"no" (or no save existing) falls through to the ordinary `CharacterCreator`
flow. One cross-check happens here, not inside `SaveGame::load` itself, for
the same reason `main.cpp` (not `WorldLoader`) validates a location's `POS`
against the grid: `SaveGame` doesn't know about `ZoneCatalog`, so `main.cpp`
is where a save referencing a since-removed zone gets caught with a clear
error instead of crashing later.

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
permadeath: HP reaching 0 caps it at 1 and warps the player back to
Solace. This is *why* combat could stay out of the save format at all —
if death were permanent (or even just persistent partial damage across a
crash mattered), losing an in-progress fight to a crash would be a much
bigger deal.

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
  a richer reaction is ever needed (e.g. a specific subrace, or
  knight/robe status).

## What's deliberately NOT abstracted yet

No plugin system, no generic "event" bus, no data-driven scripting layer,
no dynamic console-resize handling (the viewport is a fixed default size —
see `docs/GOTCHAS.md`). Those would be premature — the systems that would
need them don't exist yet. When a milestone that actually needs one of
these is built, revisit this document and update it to describe what
shipped, not just what was planned.
