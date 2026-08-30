# Gotchas & decisions log

Non-obvious traps, workarounds, and judgment calls, kept here so nobody
(human or AI) has to rediscover them the hard way. Add to this file whenever
you hit something surprising — that's the whole point of it existing.

## Windows console / ANSI

- **`GetConsoleScreenBufferInfo`'s `srWindow` vs `dwSize` — easy to get
  backwards.** `Console::currentWindowSize()` (Milestone 33) exists
  specifically to fix "the game renders bigger than my actual console
  window, so I have to scroll." `dwSize` is the **scrollback buffer**
  size — it can be, and often is, much taller than what's actually
  visible on screen, so sizing the layout to `dwSize` would silently
  reintroduce the exact bug this function exists to fix. `srWindow` (a
  `SMALL_RECT`: width = `Right-Left+1`, height = `Bottom-Top+1`) is the
  **visible window rectangle** — that's the one to use. If you're ever
  touching this code and the game starts needing to scroll again, check
  this first.
- **ANSI/VT100 escape codes are OFF by default on Windows consoles.** Unless
  `SetConsoleMode` is called with `ENABLE_VIRTUAL_TERMINAL_PROCESSING`,
  escape sequences like `\x1b[2J` print as literal garbage instead of
  clearing the screen. This is handled once, centrally, in
  `render/Console.cpp`'s constructor — don't print raw escape codes from
  anywhere else without going through `Console`.
- **The console mode change is per-console, not per-process**, and a console
  window can outlive this process (e.g. a persistent terminal tab). So
  `Console`'s destructor restores the *original* mode rather than leaving it
  altered — deliberate, not an oversight.
- **Stick to true 7-bit ASCII** for all glyphs (letters, `~ ^ % " . # ! : *`
  etc.), not Unicode box-drawing characters. This sidesteps an entire class
  of console code-page/font mismatch bugs, and matches the project's
  explicit "ASCII based" requirement. We deliberately do *not* call
  `SetConsoleOutputCP(CP_UTF8)` — nothing in our output needs it.
- **Every ANSI color-set code must be paired with a reset (`\x1b[0m`)**
  immediately after, or the color bleeds into everything printed
  afterward — including, confusingly, text from a completely different part
  of the program (the status line, a later error message). `MapRenderer`
  does this per-glyph; if you add another place that prints colored output,
  follow the same pattern.
- **A full-screen redraw now happens on every single keystroke**, not just
  every typed command (Milestone 1 only redrew per line of input). Building
  the whole frame as one string and writing it with a single
  `std::cout <<`/`fputs` call (as `MapRenderer::drawFrame` and
  `Console::clearScreen` both do) is what keeps this from visibly
  flickering — many small interleaved writes would.
- **Windows arrow keys (and other extended keys) are reported as TWO
  bytes**, not one: a prefix byte (`0x00` or `0xE0` depending on keyboard/
  driver) followed by a scan code. `_getch()` must be called a *second*
  time to consume the scan code — this is the single most common
  `conio.h` bug (forgetting the second call causes the scan code to be
  silently consumed as if it were the *next* keypress, which manifests as
  movement feeling randomly "off by one input"). Movement's on-screen
  bindings went `wasd`-only at Milestone 63, but arrow keys were restored
  same-session as a deliberately undocumented silent alias for `wasd`
  (help screen, status line, and `README.md` still say `wasd` only) —
  every other extended key still falls through to `Key::Unknown`, and
  `Console::readKey` still has to eat the second byte of *any* extended
  key it sees, mapped or not, or the classic bug above resurfaces. See
  the comment in `Console::readKey`.
- **Every obvious single-letter key mnemonic is already taken.** As of
  Milestone 63, on-screen movement bindings are `wasd`-only (no `hjkl`,
  no `yubn` diagonals — dropped for a simpler, more discoverable scheme;
  arrow keys still work as a silent, undocumented alias, see above), and
  `t`/`p`/`i`/`v`/`c`/`r`/`z`/`f`/`m`/`?`/`q`/`l` are all bound too —
  check `render::Console::readKey`'s doc comment before assuming a letter
  is free. When adding a new key, pick the letter *last*, after
  confirming nothing else already owns it.
- **`_getch()`/`_kbhit()` read from the real console, not from redirected
  stdin.** Piping input into the game the way Milestone 1's `getline`-based
  loop could be tested (`echo ... | ansalon_rpg.exe` or `< file`) does
  **not** work anymore — `_getch()` blocks waiting for an actual keypress on
  the physical console regardless of what's piped into stdin. This was
  confirmed directly while verifying Milestone 2: a piped-empty-stdin run
  rendered the first frame correctly and then blocked until killed, rather
  than reading anything from the pipe. **Practical consequence**: automated
  end-to-end testing of the keypress loop itself isn't possible without
  either a real human at the keyboard or OS-level input injection (not set
  up in this project). Movement/passability *logic* can still be verified
  headlessly by calling `world::Terrain`/`OverworldGrid`/`World` directly,
  bypassing `Console::readKey` and `GameLoop` entirely — that's how
  Milestone 2's terrain/routing was sanity-checked before a human ever
  played it (a throwaway Dijkstra-based reachability check, not part of the
  repo).
- **The exception: character creation CAN be piped.** `character::CharacterCreator::run()`
  deliberately uses plain `std::cin`/`std::cout`, not `Console::readKey()`
  (see `docs/ARCHITECTURE.md`), specifically because it runs before the
  raw-keypress world starts. That means, unlike everything else in this
  game, a full character-creation playthrough (name, ability score
  rerolls, race/class/alignment choice, confirm) can be scripted and piped
  into the exe for automated verification — this was how Milestone 4's math
  (racial adjustments, prime requisite checks, HP/AC/saves/starting steel) was
  confirmed correct before a human ever ran it. The moment `GameLoop`
  starts, the game goes back to needing a real keyboard.
- **`render::Console::readLine` (the free-text "ask about..." prompt, see
  `docs/TIMELINE_NOTES.md`'s "Ask about anything") deliberately does NOT use
  `std::cin`/`getline`, even though `CharacterCreator` already does.** The
  difference: `CharacterCreator` only ever runs *before* `GameLoop`'s
  `_getch()` loop starts (see above) — the two have never been interleaved
  in the same session anywhere in this codebase, so there's no established
  precedent that mixing buffered `std::cin` reads with raw `_getch()` reads
  inside one live session is safe. Rather than gamble on that, `readLine`
  reads one raw character at a time via `_getch()` itself (same primitive
  `readKey()` uses) and echoes/backspaces manually. If a future change ever
  wants real `std::cin` input *during* `GameLoop`, treat that as new,
  untested territory, not something this precedent already covers.
- **`readLine`'s cancel key is Esc, not `q`, unlike everywhere else in this
  game.** Every other screen uses `q`/Esc interchangeably to mean Quit/
  cancel, but `readLine` is reading arbitrary free text — `q` is a
  perfectly ordinary character to type in a real question (e.g. "ask about
  the Test") and can't be reserved. Esc is the only cancel key while typing.
- **`tokenizeAskInput` keeps a literal hyphen inside a word, it does not
  split on it or strip it to nothing** (confirmed by a Milestone 72
  throwaway self-test, per `docs/MILESTONE_72_SPEC.md` section 4.6 — see
  `docs/TIMELINE_NOTES.md`'s "Ask about anything"). Typing "half-sister"
  tokenizes to the single word `half-sister`; typing "half sister" (a
  space, not a hyphen) still splits normally into `half` and `sister`. This
  is why an authored `half-sister` keyword needs a separate `sister` alias
  alongside it if a player typing the two words apart (no hyphen) should
  still match — the hyphenated form alone only catches someone who types
  the hyphen too.
- **`SUBJECT`/`SUBJECT_WHEN` keyword collisions are a load-time warning
  (stderr), not a load failure** — see `TimelineLoader`'s
  `reportKeywordCollisions` (Milestone 72). A window's own `SUBJECT`
  sharing a keyword with a character-level pool entry is the *intended*
  override mechanism (`docs/TIMELINE_NOTES.md`'s "Ask about anything"), so
  failing hard on every collision would forbid the pattern the milestone
  was built to enable. It still warns on every case, override or not,
  because the loader can't distinguish "deliberate" from "forgot this
  keyword was already claimed" — read the warning's two line numbers
  before assuming it's fine. Author keyword lists specific-before-general
  (e.g. `orb`/`orbs` before anything claiming the bare `dragon`) to avoid
  the accidental kind. **This check is off by default** —
  `TimelineLoader::loadFromFile`'s `reportCollisionWarnings` parameter, set
  by `main.cpp` only when launched with `--check-timeline` — because a
  deliberate override (like Raistlin's `kitiara` entries) would otherwise
  warn on *every single launch forever*, which is not something a player
  should ever see. Pass `--check-timeline` after editing `timeline.txt` to
  see these warnings again.

## Toolchain

- **`cl.exe` (MSVC) is not on PATH** in a plain shell — it's only available
  after sourcing `vcvarsall.bat`/`vcvars64.bat` (the "Developer Command
  Prompt/PowerShell" environment). **Use CMake's Visual Studio generator
  instead** (`cmake -G "Visual Studio 18 2026" -A x64 ...`) — CMake locates
  the MSVC installation itself and doesn't require the calling shell to have
  sourced anything. This is the recommended flow in `README.md`. (A manual
  `vcvarsall.bat`+`cl` invocation still works for one-off standalone
  compiles, e.g. throwaway test programs outside the CMake project — see
  above — but isn't how the game itself should be built day to day.)
- **PATH doesn't refresh in already-open shells after installing a tool**
  (e.g. `winget install Kitware.CMake`). If a freshly-installed tool "isn't
  found," try a new shell before assuming the install failed.
- **`CMAKE_CXX_EXTENSIONS OFF`** and MSVC's `/permissive-` are both set
  deliberately, even though only MSVC is used today, so nothing
  MSVC-permissive-mode-specific creeps in that would silently break a
  future GCC/Clang build.
- **The offline map generator (`tools/generate_overworld.py`) is Python,
  deliberately not C++.** It needs image decoding (Pillow) which we do not
  want as a runtime dependency of the shipped game — the C++ code only ever
  reads the plain-text `data/overworld.grid` the script produces, never the
  source JPG, never Pillow. Running the script requires `pip install
  pillow`; the game itself does not.

## Data / file handling

- **`data/locations.txt` and `data/overworld.grid` are resolved relative to
  the running executable** via `render::Console::executableDirectory()`
  (`GetModuleFileNameA` on Windows, the one Windows-specific API this
  needs, kept inside `render/Console.cpp` per `CLAUDE.md`) — `main.cpp`
  looks for a `data/` folder next to the exe. This is what lets a build be
  zipped up and shared (see `tools/package_release.ps1`); `CMakeLists.txt`'s
  post-build step also copies `data/` next to the exe in `build/Debug`/
  `build/Release` so the normal dev workflow needs no extra step. The old
  compile-time absolute path (`ANSALON_DATA_DIR`, injected by
  `CMakeLists.txt`) still exists as a fallback for when the executable's own
  path can't be determined (non-Windows, or editor tooling that skips CMake
  configuration) — see `ANSALON_SAVE_FILE` below for the equivalent for
  `save.txt`.
- **`WorldLoader` fails fast** (`file:line: message`) on any malformed line
  in `data/locations.txt`, so a hand-edit typo surfaces immediately at
  startup instead of as a confusing bug deep in gameplay. It does *not*
  validate that a location's `POS` falls inside the overworld grid — that
  check happens in `main.cpp`, the one place both files are loaded together
  (see `docs/ARCHITECTURE.md`).
- **`OverworldGrid` requires every row to be the same width** (a rectangular
  grid) and fails fast if not. Since `data/overworld.grid` is generated by
  `tools/generate_overworld.py`, this should never trip in practice unless
  the file is hand-edited incorrectly (e.g. a row with a character
  accidentally deleted).
- Every named location's `POS` currently lands exactly on a road tile in
  `data/overworld.grid` — not a coincidence: `generate_overworld.py` draws
  roads as lines *between* location coordinates, so the endpoints are
  always road. This incidentally guarantees every location is on passable
  terrain; if a location is ever added without a road connecting to it,
  double-check its `POS` doesn't land on impassable terrain (ocean, the
  Blood Sea) by hand.

## Timeline (`timeline::Timeline`)

- **`TimelineLoader` does not validate `PRESENCE`'s `location-id` against
  `data/locations.txt`** — unlike `WorldLoader`'s fail-fast validation, a
  typo'd location id in `data/timeline.txt` loads without error and just
  never matches anything at query time (see `docs/TIMELINE_NOTES.md`).
  Deliberate for now (a small, hand-authored file), but a silent-failure
  risk worth revisiting if this file grows much bigger.
- **Presence is overworld-only** — standing on a location's tile checks the
  timeline; being inside that location's zone (interior) does not. Finding
  a canon character specifically inside, say, the Inn of the Last Home
  isn't wired up yet — see `docs/TIMELINE_NOTES.md`.
- **`day = hoursElapsed / 24`, integer division** — matches how the status
  line already computes "Day N," but means a character's window boundary
  lands on a whole-day granularity, not an hour one. A window `PRESENCE
  solace 0 1` covers all of hours 0–47, not just a couple of hours.

## Combat (`combat::Combat`, `game::GameLoop::runCombat`)

- **Combat state is never saved.** An encounter is a nested loop inside
  `GameLoop`, not a `GameState.mode` — see `docs/COMBAT_NOTES.md`. A crash
  or force-close mid-fight just loses that encounter entirely (player
  resumes on the overworld tile where it started, monster forgotten). This
  is an accepted trade-off given the "knocked out, not killed" death rule,
  not an oversight — don't add mid-combat autosaving without also
  reconsidering whether it's still needed.
- **Adding a new `render::Key` value requires touching every `switch` over
  `Key`, not just the one that cares about it.** This project relies on
  exhaustive `switch` coverage instead of a `default:` label so a
  genuinely unhandled case is easy to spot in review. Adding `Key::Flee`
  needed a `case render::Key::Flee: break;` in the main loop's `switch` in
  `GameLoop::run()` even though that switch does nothing with it —
  `runCombat` has its own separate, narrower key check.
  **Correction (verified at Milestone 51, don't trust the old wording
  above): this is NOT actually a compiler safety net.** MSVC's C4061/C4062
  ("enumerator in switch of enum is not explicitly handled by a case
  label") are real warnings, but they're off by default and **not** part
  of `/W4` — only `/Wall` enables them, which this project doesn't use.
  Confirmed empirically while adding `Key::Journal`: temporarily deleting
  its `case` from `GameLoop::run()`'s switch and rebuilding under this
  project's actual `/W4 /permissive-` flags produced zero warnings — the
  key would have just silently done nothing. **Every switch over `Key`
  genuinely has to be checked by hand** (there are three consumers:
  `Console.cpp`'s two `readKey()` switches, both of which *do* have
  `default:` and so also silently swallow anything unhandled, plus
  `GameLoop::run()`'s dispatch switch) — verify a new key by actually
  pressing it, not by trusting a clean build.
- **Monster stats in `data/monsters.txt` are invented, not sourced** — see
  `docs/COMBAT_NOTES.md` for why (no Monstrous Compendium/Monster Manual
  in this project's reference library). Don't assume they're
  book-verified the way class/race numbers are.
- **`character::roll(count, sides)` used to crash on `count == 0`,
  regardless of `sides`** — it unconditionally built
  `std::uniform_int_distribution<int> die(1, sides)` before ever checking
  `count`, so `roll(0, 0)` (`STEEL 0 0 0`, "this monster drops no steel" —
  Timber Wolf, Skeleton, Zombie) violated the distribution's own `min <=
  max` precondition and crashed the whole process the instant one of
  those three was the killing blow. Fixed at the root (`count <= 0`
  returns `0` before constructing anything) — see
  `docs/COMBAT_NOTES.md`'s "Bug fixed" section. If you're ever
  hand-rolling a new `uniform_int_distribution` somewhere instead of going
  through `character::roll`, this is the exact mistake to avoid: check
  the count/range is non-empty *before* constructing the distribution,
  not after.

## Save/load (`game::SaveGame`)

- **3 save-slot files (`save1.txt`/`save2.txt`/`save3.txt`), resolved next
  to the running executable** (Milestone 89), same
  `executableDirectory()`-based resolution as `data/` above (falling back to
  the compile-time `ANSALON_SAVE_FILE_BASE` in the same rare cases). This
  changed from "always the repo root" — running `build\Debug\ansalon_rpg.exe`
  now reads/writes `build\Debug\save<N>.txt`, not `<repo root>\save<N>.txt`,
  and a packaged build (`tools/package_release.ps1`) gets its own
  independent set of slots next to wherever it's unzipped. All 3 are
  gitignored — see `.gitignore`. A pre-Milestone-89 single `save.txt` in
  that same directory is auto-migrated to `save1.txt` the first time it's
  found with no `save1.txt` already present (see `docs/ARCHITECTURE.md`);
  `save.txt` itself stays gitignored too, in case a stray one lingers from
  before the migration.
- **NEVER "clean rebuild" by deleting `build\` wholesale.** The save slots
  live *inside* `build\Debug` (see the bullet above), `build\` is
  gitignored, and `Remove-Item -Recurse -Force` bypasses the Recycle Bin —
  so `rm -rf build` destroys the player's characters with no recovery path
  unless a shadow copy happens to exist. This actually happened at
  Milestone 120 and cost the user their save slots. A clean rebuild means
  `cmake --build build --config Debug --clean-first`, or deleting only the
  CMake artifacts — never the directory the running exe keeps its state
  in. If a full wipe is genuinely needed, copy `build\Debug\save*.txt`
  somewhere outside `build\` first and copy them back afterward, the same
  move-aside/restore discipline the piped smoke test already requires.
- **A bad slot no longer aborts the whole program.** Before Milestone 89, a
  single corrupt/stale `save.txt` (e.g. referencing a since-removed
  `ZONE`) made the game unplayable until the file was deleted by hand.
  Now each slot is loaded and cross-checked independently
  (`main.cpp`'s local `describeSlot`); a slot that fails is shown as
  "(unreadable save: ...)" in the menu, and the other two remain playable.
  The `RACE`/`CLASS`/`ALIGNMENT` raw-enum-int fragility described below
  still applies — just now per-slot instead of to one single file.
- **`RACE`/`CLASS`/`ALIGNMENT` are stored as raw enum ints**, not names
  (`static_cast<int>(character::RaceId)` etc.). This is a deliberate
  simplicity trade-off (no name↔enum reverse lookup needed anywhere else in
  the project) but it means **reordering or inserting values in `RaceId`,
  `ClassId`, or `Alignment` silently corrupts any existing `save.txt`** — the
  int will still parse and pass the range check, it'll just now mean a
  different race/class/alignment. There's no version field to detect this.
  Acceptable for a single personal save file; would need addressing (a
  format version, or switching to stored names) before this format could
  ever be shared between builds. **Removing** a value is the sharper edge
  of this same fragility, and it's the one that's actually bitten this
  project: deleting `RaceId::Halfling` (Milestone 110) silently renumbered
  `Kender` from 6 down to 5 and broke a real, freshly-created save
  (`RACE 6`) within the same session. Fixed by pinning `Kender = 6`
  explicitly rather than letting it renumber, leaving 5 permanently unused
  — see `character::kRaceIdCount` (used only by `SaveGame.cpp`'s RACE
  bounds check, `kAllClasses`/`kClassIdCount` below has the same shape for
  Tinker). **Before ever removing an enum value from `RaceId`/`ClassId`/
  `Alignment`, check every real `save*.txt` this repo can reach** (root
  `save.txt` and `build/<Config>/save1.txt`/`save2.txt`/`save3.txt` — the
  build directory copy is easy to forget since it's gitignored and doesn't
  show up in `git status`) for the raw int the removed value used to be,
  and pin any higher-numbered surviving value that would otherwise shift
  down past it, the same way `Kender` was pinned here.
- **Autosave happens after every processed keypress, unconditionally** — see
  `GameLoop::run()`. This was a deliberate simplicity choice over only
  saving on state-changing keys (`Look`/`Sheet` don't mutate `GameState`,
  but saving anyway is simpler than tracking "did this branch change
  anything," and the write is cheap). Practical effect: `Ctrl+C`/closing the
  terminal loses at most the single most recent keypress, not a session.
- **`SaveGame::load` does not validate cross-references** against `World` or
  `ZoneCatalog` (e.g. that a saved `ZONE <id>` still exists) — same
  separation as `WorldLoader` not knowing about `OverworldGrid`. That check
  happens in `main.cpp`'s `describeSlot` helper, the one place a loaded save
  and the freshly-loaded `ZoneCatalog` are both available; a stale
  reference is caught there and, since Milestone 89, surfaces as that one
  slot showing "unreadable" in the save-slot menu rather than aborting the
  whole program.
- **`load()` accepts both `STEEL` and legacy `GOLD` as the currency
  keyword** (Milestone 22's Gold -> Steel rename, see
  `docs/CHARACTER_NOTES.md`) — `save()` only ever writes `STEEL` now, but a
  save file written before that milestone still has a `GOLD <n>` line, and
  refusing to load it would have broken the user's real in-progress
  character. This is a deliberate, permanent compatibility fallback, not
  a TODO to remove later — the same "keep old saves loading" spirit as the
  raw-enum-int fragility above, just handled explicitly this time instead
  of left as an accepted risk.
- **`ZONESTACK` must stay the last thing `save()` writes.** `load()`
  consumes `ZONESTACK`'s N entry lines by counting, not by keyword — once
  it's seen the `ZONESTACK <n>` line, the next `n` lines are read as raw
  `<zoneId> <x> <y>` rows regardless of what they'd otherwise parse as.
  Milestone 51's `QUEST`/`KILL` lines had to be inserted *before*
  `ZONESTACK` (right after `MET`, Milestone 92's `VOYAGED` now between
  them) for exactly this reason — putting them after would have them
  silently swallowed as phantom zone-stack entries (or worse, corrupt real
  ones) the moment the stack is non-empty. Any future new save keyword
  needs the same check: does it land before
  `ZONESTACK`, or does it break the count?
- **`COMPANION` persists a flag plus one real field (`currentHp`), every
  other field still reconstructed.** Milestone 116 Phase 1 originally wrote
  just `COMPANION 1`, since `character::buildCompanion()` is pure/
  deterministic (fixed ability scores/steel, never `character::roll`) and
  `SaveGame::load` could just call it again to reconstruct an identical
  companion. Milestone 117 let combat actually change the companion's HP,
  so the line grew a second, optional token: `COMPANION 1 <currentHp>`.
  `load()` reads the `1` flag exactly as before (still `fail()`s if it
  isn't `1`), then *tries* to read a second int; if present, it overwrites
  `buildCompanion()`'s default `currentHp` (clamped to `[0, maxHp]`); if
  absent -- a pre-Milestone-117 save with the old one-token line -- it
  silently keeps the full-health default, which is correct, since combat
  never touched the companion before Milestone 117. Every field other than
  `currentHp` (race/class/scores/steel/...) is still never serialized --
  `buildCompanion()` remains the source of truth for all of them. The whole
  line is still optional, same "absence means false" convention as `MET`/
  `VOYAGED`/`QUEST`/`KILL` -- a pre-Milestone-116 save has no `COMPANION`
  line at all and `hasCompanion` correctly stays false. Any future field
  that becomes mutable (e.g. if the companion ever gains levels) would need
  the same treatment: a new optional trailing token, defaulted sensibly
  when absent, never breaking an older save. Must stay before `ZONESTACK`,
  same ordering rule as every other optional keyword above.
- **A quest's `TALK <met-id>` objective isn't validated against real
  character/NPC ids at load time.** `quest::QuestLoader` can't see
  `data/timeline.txt` or `data/zones/*.txt` (same one-way dependency
  direction as everything else — see `docs/ARCHITECTURE.md`), so a typo'd
  or wrong met-id (it's `tanis`, not `"Tanis"`; `haven:G`, not `G` or
  `"Seeker Guard"`) parses cleanly and the objective simply never
  completes. Same silent-failure class as `TimelineLoader` not validating
  a `PRESENCE` location id. See `docs/QUEST_NOTES.md`'s "The met-id trap."

## Zones (walkable interiors)

- **A zone's `GRID` must fit inside the viewport** (`MapRenderer::kViewportWidth/Height`,
  78×20) — there is no camera/scrolling for zones, unlike the overworld
  (see `docs/ZONE_NOTES.md`). A larger zone will render off the edge of a
  normal terminal, not scroll into view.
- **`ZoneCatalog` matches zones to locations purely by filename**
  (`data/zones/<id>.txt` ↔ `LOCATION <id>`), not a field stored on
  `Location`. Renaming a `LOCATION` id in `data/locations.txt` silently
  disconnects its zone file (the game just won't find it — no error) unless
  the zone filename is renamed to match. There's no validation that catches
  this at load time, since `WorldLoader` has no knowledge that zones exist.
- **`ZoneLoader`'s `GRID`/`ENDGRID` block is raw text, not keyword-parsed**
  — every line in it is taken literally (no trimming, no `#`-comment
  handling) until a line reads exactly `ENDGRID`. Don't try to add a comment
  inside a `GRID` block expecting it to be stripped; it'll be read as a row
  of terrain characters (and almost certainly fail the "every row same
  width" check).
- **Every non-base-terrain character in a `GRID` block needs a matching
  `POI` declaration**, or the whole file fails to load with a clear error —
  deliberately strict, same reasoning as `WorldLoader`'s `CONNECT`
  validation in Milestone 1: a typo'd tile character should be caught at
  startup, not silently misrendered during play.
- **Bug found and fixed while adding the Inn of the Last Home's interior**:
  every POI tile (Inn, Notice Board, named trees, etc.) was actually
  *unwalkable*. `world::zoneTileFor` only recognizes the 5 base terrain
  codes; any other character (including every POI code) fell through to the
  "unknown tile" default, which is impassable. `GameLoop::tryMoveZone`
  looked up passability purely via `zoneTileFor`, with no POI-aware branch,
  so the player could never actually stand on a POI tile to see its
  description — contradicting what `docs/ZONE_NOTES.md` and
  `MapRenderer::drawZoneFrame` both assumed. Fixed by checking
  `Zone::poiAt` first in `tryMoveZone`: a POI tile is now always passable,
  regardless of glyph. This had gone unnoticed because earlier playtesting
  exercised general movement, not those specific tiles.
- **Zones can now portal into other zones** (`PORTAL <char> <zone-id>`, see
  `docs/ZONE_NOTES.md`) — used for the Inn of the Last Home's interior,
  entered from a door tile inside Solace's town square. A zone reached only
  via a portal has no matching `LOCATION`, so `ZoneCatalog::loadForWorld`
  has a second pass that follows `PORTAL` links transitively to find those
  files; a portal pointing at a missing zone file throws at startup, same
  as every other load-time validation in this project.

## Presentation (Milestone 29: wide HUD + log panel)

- **`drawZoneFrame` loops the full `kViewportWidth`/`kViewportHeight`
  now, not `zone.width()`/`height()`.** This depends on
  `Zone::tileCodeAt`/`poiAt` (`src/world/Zone.cpp`) returning `'#'`/
  `nullptr` for any out-of-bounds coordinate rather than throwing/
  asserting — that behavior already existed (for the entry/exit edge
  case), this milestone just started relying on it for every
  out-of-zone-bounds tile, not just a few. If that OOB behavior is ever
  changed (e.g. made to assert in a debug build), `drawZoneFrame` will
  need an explicit bounds check reintroduced before the render loop.
- **The log panel wraps and pads to a fixed width/height
  (`MapRenderer::buildLogPanel`)** — every raw `log_` entry is word-
  wrapped to `kLogPanelWidth` (40) and the result is truncated to the
  last `kViewportHeight` (20) physical lines. A single very long log
  entry can therefore push earlier entries off the top of the panel in
  one frame; this is intentional (matches a real scrolling log), not a
  bug, but worth knowing if a future entry seems to "disappear" sooner
  than expected.

## Map generation & fidelity

- **Location `POS` coordinates are approximate**, chosen from relative
  Dragonlance geography and the overall shape of the reference map, not
  pixel-measured off it — the map's small text labels aren't legible at any
  resolution that was practical to inspect. See `docs/MAP_NOTES.md`.
- **Terrain classification is inherently iterative, not one-shot.** The
  reference map is painted/textured (mountains drawn as icon clusters on
  green, not flat color-coded regions), so there's no reliable fully-
  automatic way to know "this shade of green is forest, not grassland"
  without a human looking at a preview once. `generate_overworld.py` is
  deliberately two-phase (discover colors → hand-assign terrain → generate)
  for exactly this reason — see the script's own docstring and
  `docs/MAP_NOTES.md`.
- **Re-running `tools/generate_overworld.py` OVERWRITES `data/overworld.grid`**,
  including any hand corrections made after a previous run. If you hand-fix
  a tile (e.g. a misclassified pixel near a settlement), either don't rerun
  the generator afterward, or reapply the fix.
- **Glacier, bog, and salt-flat terrain don't currently appear** in the
  generated grid — at `NUM_COLORS=16` they didn't emerge as distinct
  classified colors (see the script's `INDEX_TO_TERRAIN` comment). They
  still exist in `world::Terrain`'s vocabulary and can be hand-painted into
  `data/overworld.grid`, or would appear with a higher `NUM_COLORS` and a
  re-run of the discovery phase.
- **Roads are drawn as straight Bresenham lines** between location
  coordinates, not routed around obstacles — a road can visually cut
  through terrain it "shouldn't" (e.g. a mountain) if the straight line
  between two locations happens to cross one. Hand-edit
  `data/overworld.grid` to fix specific cases; not worth a pathfinding-based
  road generator for a handful of roads. `draw_line()` guarantees
  consecutive road tiles are always 4-directionally adjacent, never
  diagonal-only — see `docs/MAP_NOTES.md`'s "Road 4-connectivity fix" — so
  this simplification no longer risks a walkability dead end, just an
  occasionally-wrong-looking terrain crossing.
