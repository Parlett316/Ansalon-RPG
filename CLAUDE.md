# CLAUDE.md

Instructions for any future session (human or AI) working in this repo.

## What this project is

**Ansalon: Age of Despair** — a personal, non-commercial fan RPG set on
Krynn (Dragonlance) during the War of the Lance, 2nd Edition AD&D rules,
presented Caves-of-Qud style: a colored ASCII overworld walked in real
time, C++17, terminal-based, built with CMake for MSVC on Windows. Not
affiliated with or endorsed by Wizards of the Coast.

The core pitch: canon Heroes of the Lance (Tanis, Sturm, Raistlin,
Caramon, Goldmoon, Riverwind, Tasslehoff, Flint) move through their real
novel-timeline locations on a schedule, so the player can stumble into a
"chance encounter" with them. Everything else (character creation,
combat, equipment, leveling, zones) exists in service of that.

Full player-facing feature list: `README.md`.

## Coding standards and constraints

- **C++17**, no MSVC-only extensions (`CMAKE_CXX_EXTENSIONS OFF`) — code
  should be portable to a future GCC/Clang build even though only MSVC is
  used today.
- **`/W4` clean, always.** Zero new warnings is the bar for every change,
  not just "it compiles."
- **True 7-bit ASCII only** — no Unicode box-drawing glyphs. Every ANSI
  color-set code must be paired with a reset (`\x1b[0m`) immediately
  after.
- **All Windows-specific code stays inside `render/Console.cpp`**, guarded
  by `#ifdef _WIN32`. Every other file is portable standard C++17.
- **Data over code**: locations, zones, timeline, and monsters are
  hand-rolled text files under `data/`, not JSON/XML, parsed by small
  fail-fast loaders (`file:line: message` on any malformed line). Follow
  the existing `trim`/`splitKeyword`/fail-fast idiom when adding a new
  loader or grammar line — don't introduce a new parsing style.
- **Explicit source file list in `CMakeLists.txt`**, not globbed — adding
  a `.cpp` is a visible, reviewable line.
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
- **No premature abstraction**: don't add subsystems, config layers, or
  generic engines before a milestone actually needs them. See
  `docs/ARCHITECTURE.md`'s "What's deliberately NOT abstracted yet".

## Which doc to read for which kind of work

| Working on... | Read first |
|---|---|
| Overall structure, module boundaries, why things are split the way they are | `docs/ARCHITECTURE.md` |
| Non-obvious traps, past bugs, platform quirks, file-format gotchas | `docs/GOTCHAS.md` |
| The overworld grid, terrain generation, location placement | `docs/MAP_NOTES.md` |
| Walkable zone interiors, POIs, TALK/SHOP/TIMELINE_ANCHOR grammar | `docs/ZONE_NOTES.md` |
| Race/class/leveling/spellcasting/equipment rules content | `docs/CHARACTER_NOTES.md` |
| The canon-character chance-encounter schedule | `docs/TIMELINE_NOTES.md` |
| Attack/damage math, monsters, victory/leveling flow | `docs/COMBAT_NOTES.md` |
| What's shipped so far (numbered milestone history), what's next | `docs/MILESTONES.md` |

Read `docs/ARCHITECTURE.md` and `docs/GOTCHAS.md` before touching
anything outside a single, well-contained file — most non-obvious traps
are already documented there.

## Build process

From a plain PowerShell (no need to run `vcvarsall.bat` — the VS
generator locates MSVC itself):

```powershell
cmake -G "Visual Studio 18 2026" -A x64 -S . -B build
cmake --build build --config Debug
```

Executable lands at `build\Debug\ansalon_rpg.exe`. It locates
`data/locations.txt` etc. (and `save.txt`) next to itself at runtime — a
CMake post-build step keeps `data/` populated in `build\Debug`/
`build\Release` automatically, so this needs no extra step in normal dev
use (see `docs/GOTCHAS.md`). To hand a runnable build to someone outside
this source tree, run `tools/package_release.ps1` — see its header comment.

Faster incremental alternative: open a "Developer PowerShell for VS
2026" and use `cmake -G Ninja -S . -B build` / `cmake --build build`.

**`_getch()` ignores piped stdin** — the real keypress loop can't be
tested headlessly. The one exception is character creation
(`CharacterCreator::run()`), which uses plain `std::cin`/`std::cout` and
*can* be piped — the standard way to verify data files parse cleanly
end-to-end without a human at the keyboard:

```powershell
echo "" | ./build/Debug/ansalon_rpg.exe
```

This should reach the character-creation prompts cleanly (not complete
them) before EOF-failing — proof `World`/`ZoneCatalog`/`Timeline`/
`MonsterCatalog` all loaded without throwing. If a real `save.txt`
exists, move it aside first, run this, then move it back — never leave
the user's real save clobbered or missing.

**Throwaway self-test pattern** for verifying new logic: write a
`*SelfTest.cpp`, add a temporary `add_executable` CMake target with only
the needed source files, build and run it, confirm it passes, then
**delete the file and the CMake target block** before the final clean
rebuild of the real `ansalon_rpg` target. Self-tests are a verification
step, not a permanent test suite — this project has none checked in by
design.

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
6. **Verify** via piped character creation (and a real playthrough
   against the user's actual save if the change touches save format or
   is otherwise risky to their in-progress character).
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
