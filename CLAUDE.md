# CLAUDE.md

Instructions for any future session (human or AI) working in this repo.

## What this project is

**Ansalon: Age of Despair** — a personal, non-commercial fan RPG set on
Krynn (Dragonlance) during the War of the Lance, 2nd Edition AD&D rules.
C++17, built with CMake for MSVC on Windows. Not affiliated with or
endorsed by Wizards of the Coast.

The core pitch (unchanged since the project's start): canon Heroes of
the Lance (Tanis, Sturm, Raistlin, Caramon, Goldmoon, Riverwind,
Tasslehoff, Flint) move through their real novel-timeline locations on a
schedule, so the player can stumble into a "chance encounter" with them.
Everything else (character creation, combat, equipment, leveling, zones)
exists in service of that.

**Presentation has migrated away from ASCII.** The project originally
shipped Caves-of-Qud style — a colored ASCII overworld walked in real
time in a terminal (`render::Console`, the `ansalon_rpg` target). The
user committed to a full move to real pixel-space rendering via SFML (a
monospace-grid recreation was tried first and rejected as "looks almost
exactly the same" as the terminal). Every screen has since been ported to
a second, standalone target (`ansalon_sfml_phase1`, `sfml_phase1/
main.cpp`, developed on branch `sfml-trial-3`) — placeholder shapes/text
for now, ready for real sprite art later.

**`ansalon_sfml_phase1` is now the primary/main build** — the one to
build and run by default. `ansalon_rpg` (the original ASCII console
build, still on `master`) stays in the tree as a legacy/reference build,
not retired; whether to retire it outright remains a separate, later
decision. Both targets still build side by side. See
`docs/CURRENT_WORK.md` for exactly which SFML screens are implemented
vs. still needing a live interactive playtest, and
`docs/ARCHITECTURE.md`'s SFML section for why the codebase is split this
way instead of a big-bang rewrite.

Full player-facing feature list: `README.md` — written for the console
build's UI and not yet updated for the SFML target's; the underlying
game systems/rules/content it describes apply to both.

## Coding standards and constraints

- **C++17**, no MSVC-only extensions (`CMAKE_CXX_EXTENSIONS OFF`) — code
  should be portable to a future GCC/Clang build even though only MSVC is
  used today.
- **`/W4` clean, always.** Zero new warnings is the bar for every change,
  across every build target (`ansalon_rpg` and `ansalon_sfml_phase1`
  alike) — not just "it compiles."
- **The ASCII/ANSI rendering rule applies only to the legacy console
  target** (`render::Console`, `ansalon_rpg`): true 7-bit ASCII only, no
  Unicode box-drawing glyphs, and every ANSI color-set code paired with
  a reset (`\x1b[0m`) immediately after. The SFML target
  (`ansalon_sfml_phase1`) draws real pixel-space shapes/text instead and
  isn't bound by this — see `docs/ARCHITECTURE.md`'s SFML section.
- **Windows-specific code for the console target stays inside
  `render/Console.cpp`**, guarded by `#ifdef _WIN32`; every other console
  file is portable standard C++17. The SFML target has no equivalent
  need — SFML itself abstracts windowing/input/graphics across
  platforms, so `sfml_phase1/main.cpp` calls SFML APIs directly rather
  than going through `Console`.
- **Data over code**: locations, zones, timeline, and monsters are
  hand-rolled text files under `data/`, not JSON/XML, parsed by small
  fail-fast loaders (`file:line: message` on any malformed line). Follow
  the existing `trim`/`splitKeyword`/fail-fast idiom when adding a new
  loader or grammar line — don't introduce a new parsing style.
- **Explicit source file list in `CMakeLists.txt`**, not globbed — adding
  a `.cpp` is a visible, reviewable line. Three targets currently share
  this file (`ansalon_rpg`, the parked `ansalon_sfml_trial` spike, and
  the active `ansalon_sfml_phase1`), each listing its own sources
  explicitly.
- **Sourced content is verified against actual scanned rulebooks**
  (Player's Handbook, DMG, Dragonlance Adventures, Monstrous Manual, the
  Chronicles novels), not written from memory and left unchecked. When
  adding rules or lore content, cite the page/book and note explicitly
  what's sourced vs. invented — see any "Accuracy" section in `docs/`.
- **Non-infringement discipline**: all dialogue/flavor text is freshly
  written, inspired by source-book voice/tone/specific beats — never
  transcribed.
- **Restraint over completeness**: if source material doesn't support a
  feature/location/content pass, leave it out and document why, rather
  than inventing to fill a gap. This project has done this repeatedly
  (e.g. Plains of Dust and Tarsis have no timeline content because the
  reference novel never goes there).
  **This bar is for the tracked canon Companions' documented
  timeline/dialogue specifically — not for the player's own overworld
  geography.** A `LOCATION` can be added purely because it's real on
  `References/dragonlancemap2.png` and useful for the player character's
  own free movement (Crossing, Port O'Call — real port/ferry towns, on
  the map, with no on-page presence in any tracked novel, added anyway
  because the *player's* character isn't bound to the Companions'
  specific documented path). Still needs the same placement rigor
  (pixel-measured against the reference map, terrain-checked, cited in
  `docs/MAP_NOTES.md`) — the relaxed bar is about *sourcing to a novel*,
  not about *skipping verification*.
- **No premature abstraction**: don't add subsystems, config layers, or
  generic engines before a milestone actually needs them. See
  `docs/ARCHITECTURE.md`'s "What's deliberately NOT abstracted yet".

## Which doc to read for which kind of work

| Working on... | Read first |
|---|---|
| Overall structure, module boundaries, why things are split the way they are | `docs/ARCHITECTURE.md` |
| Non-obvious traps, past bugs, platform quirks, file-format gotchas | `docs/GOTCHAS.md` |
| The overworld grid, terrain generation, location placement | `docs/MAP_NOTES.md` |
| Walkable zone interiors, POIs, TALK/SHOP/TIMELINE_ANCHOR/QUEST grammar | `docs/ZONE_NOTES.md` |
| Race/class/leveling/spellcasting/equipment rules content | `docs/CHARACTER_NOTES.md` |
| The canon-character chance-encounter schedule | `docs/TIMELINE_NOTES.md` |
| Attack/damage math, monsters, victory/leveling flow | `docs/COMBAT_NOTES.md` |
| Quest grammar, objectives, the journal, turn-in flow | `docs/QUEST_NOTES.md` |
| What's shipped so far (numbered milestone history), what's next | `docs/MILESTONES.md` |

Read `docs/ARCHITECTURE.md` and `docs/GOTCHAS.md` before touching
anything outside a single, well-contained file — most non-obvious traps
are already documented there.

## Cost Management

- Do not spawn subagents for routine content work (dialouge, monsters, shops timeline, milestone updates).
- Read files directly for those.
- Subagents are only for project-wide searches, architecture reviews, and multi-file crash debugging.
- When a subagent IS used, run it on the cheapest capable model (Haiku), reserving the main model for decisions and code.

## Build process

From a plain PowerShell (no need to run `vcvarsall.bat` — the VS
generator locates MSVC itself):

```powershell
cmake -G "Visual Studio 18 2026" -A x64 -S . -B build
cmake --build build --config Debug
```

This configures and builds all three CMake targets: `ansalon_rpg` (the
legacy ASCII console build), `ansalon_sfml_phase1` (the active SFML
migration target), and `ansalon_sfml_trial` (a parked round-2 spike —
see `docs/CURRENT_WORK.md`). The **first** configure fetches SFML via
`FetchContent` from GitHub — this project's only external dependency —
which needs network access and takes noticeably longer than later
configures.

**`ansalon_rpg`** lands at `build\Debug\ansalon_rpg.exe`. It locates
`data/locations.txt` etc. (and its save-slot files, `save1.txt`/
`save2.txt`/`save3.txt`) next to itself at runtime — a CMake post-build
step keeps `data/` populated in `build\Debug`/`build\Release`
automatically, so this needs no extra step in normal dev use (see
`docs/GOTCHAS.md`). To hand a runnable build to someone outside this
source tree, run `tools/package_release.ps1` — see its header comment.

**`ansalon_sfml_phase1`** (the primary/main build — see "What this
project is" above) lands at `build\Debug\ansalon_sfml_phase1.exe`. It
reads `data/*.txt` and `References/dragonlancemap2.png` as plain relative
literals rather than resolving them executable-relative the way
`ansalon_rpg` does (see `docs/ARCHITECTURE.md`'s SFML section), so a
CMake post-build step keeps both a `data/` copy and just
`References/dragonlancemap2.png` (not the whole 460MB+ `References/`
folder) populated next to it in `build\Debug`/`build\Release` — the same
"runs straight from the build output" convention `ansalon_rpg` already
has. Running with the repo root as the working directory instead still
works too (its original convention, unaffected). Run it with **no
arguments** to get its own native save-slot menu and character creation
wizard (create or continue a character entirely in the window, no
console step needed) — `.\ansalon_sfml_phase1.exe` from inside
`build\Debug`. A save path is still accepted directly too, unchanged, for
a quick dev launch against a known save: `.\ansalon_sfml_phase1.exe
save1.txt` from inside `build\Debug`, or
`.\build\Debug\ansalon_sfml_phase1.exe build\Debug\save1.txt` from the
repo root. To hand a runnable build to someone outside this source tree,
run `tools/package_playable_release.ps1` instead.

Faster incremental alternative: open a "Developer PowerShell for VS
2026" and use `cmake -G Ninja -S . -B build` / `cmake --build build`.

**`_getch()` ignores piped stdin** — `ansalon_rpg`'s real keypress loop
can't be tested headlessly. The one exception is character creation
(`CharacterCreator::run()`), which uses plain `std::cin`/`std::cout` and
*can* be piped — the standard way to verify data files parse cleanly
end-to-end without a human at the keyboard:

```powershell
echo "<empty-slot-number>" | ./build/Debug/ansalon_rpg.exe
```

An empty line no longer works on its own — Milestone 89's save-slot menu
sits in front of character creation now, and it needs a slot number that
is actually empty (piping the number of an occupied slot instead lands on
the "Continue this character? (y/n)" prompt and EOF-fails there, never
reaching character creation). Slot occupancy varies as the user plays —
don't assume slot 1 is empty. If unsure, run once with any digit first
just to read the printed "Save slots:" listing, note which one says
`(empty)`, then rerun piping that number.

This should reach the character-creation prompts cleanly (not complete
them) before EOF-failing — proof `World`/`ZoneCatalog`/`Timeline`/
`MonsterCatalog` all loaded without throwing. If real `save1.txt`/
`save2.txt`/`save3.txt` files exist, move them aside first, run this,
then move them back — never leave the user's real save clobbered or
missing.

**`ansalon_sfml_phase1` can't be verified this way at all** — it's a
real SFML window driven by `sf::Event`, with no piped-input path, and a
session typically has no desktop/GUI access to drive it interactively
either. The available self-check from inside a session is a **launch
smoke test**: run the exe briefly (from `build\Debug` or the repo root,
per the CWD note above) and confirm it opens without crashing/throwing
and every catalog
it loads (World, ZoneCatalog, Timeline, MonsterCatalog, a real save file
via `SaveGame`) succeeds — necessary but not sufficient. Anything that
actually needs a keypress or a rendered frame to confirm (does a picker
render right, does a fight resolve, does an overlay dismiss on the right
key) must be flagged as **"not yet interactively confirmed"** in the
hand-off and added to `docs/CURRENT_WORK.md`'s Playtest backlog for the
user to check live at their own keyboard — never claim a screen works
just because it compiled and smoke-tested clean.

**Throwaway self-test pattern** for verifying new logic: write a
`*SelfTest.cpp`, add a temporary `add_executable` CMake target with only
the needed source files, build and run it, confirm it passes, then
**delete the file and the CMake target block** before the final clean
rebuild of the real target(s). Self-tests are a verification step, not a
permanent test suite — this project has none checked in by design.

## Session workflow

This project is built incrementally, milestone by milestone, across many
sessions. Established loop for any non-trivial change:

1. **Research first** if the change touches sourced content (rules,
   lore, dialogue) — check the actual PDFs already in the project folder
   (`pdftotext -layout`, or render+read a page image for anything
   table-heavy/OCR-unreliable) before writing anything. Let findings
   shape scope, including cutting scope if the source doesn't support it.
2. **Plan** (`EnterPlanMode` before writing anything to the plan file,
   every time) → get it approved.
3. **Implement.**
4. **Self-test** via the throwaway pattern above, then clean up.
5. **Clean rebuild**, zero new `/W4` warnings.
6. **Verify**: for `ansalon_rpg` changes, via piped character creation
   (and a real playthrough against the user's actual save if the change
   touches save format or is otherwise risky to their in-progress
   character); for `ansalon_sfml_phase1` changes, via a launch smoke
   test (see "Build process" above), flagging anything not
   interactively confirmed on `docs/CURRENT_WORK.md`'s Playtest backlog
   for the user to check live.
7. **Update the relevant `docs/*.md`** (the ones in the table above,
   whichever apply) and `README.md`'s Status paragraph if the change is
   player-visible.
8. **Update `docs/CURRENT_WORK.md`** (see below) and hand off to the
   user with a concise summary: what shipped, key decisions and why,
   what was deliberately left out, how it was verified.

Don't start the next milestone without being asked — offer a short menu
of backlog options (`AskUserQuestion`) instead of assuming.

### Session hand-off: `docs/CURRENT_WORK.md`

`docs/CURRENT_WORK.md` is the single file a fresh session should read
first to reorient — what's in flight right now, as opposed to the `docs/
*_NOTES.md` files above, which describe how finished systems work.

- **At the start of a session**: read it before doing anything else.
- **While mid-task**: keep it current — if you stop partway through a
  milestone (context limit, user pauses you, end of session), update it
  with exactly enough state for a cold start to resume without
  re-deriving anything: what's done, what's left, any decisions already
  made, and the next concrete step.
- **When a milestone finishes cleanly** (implemented, tested, docs
  updated, handed off to the user): clear it back to "nothing in
  flight," don't let it accumulate history — that's what git history and
  the `*_NOTES.md` "Milestone N" writeups are for.

It should stay short. If it's answering "what should I do right now,"
it's doing its job; if it's turning into a project history, trim it.
