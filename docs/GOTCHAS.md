# Gotchas & decisions log

Non-obvious traps, workarounds, and judgment calls, kept here so nobody
(human or AI) has to rediscover them the hard way. Add to this file whenever
you hit something surprising — that's the whole point of it existing.

## Windows console / ANSI

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
- **Windows arrow keys are reported as TWO bytes**, not one: a prefix byte
  (`0x00` or `0xE0` depending on keyboard/driver) followed by a scan code.
  `_getch()` must be called a *second* time to read the scan code — this is
  the single most common `conio.h` bug (forgetting the second call causes
  the scan code to be silently consumed as if it were the *next* keypress,
  which manifests as movement feeling randomly "off by one input"). See the
  comment in `Console::readKey`.
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
  (racial adjustments, prime requisite checks, HP/AC/saves/gold) was
  confirmed correct before a human ever ran it. The moment `GameLoop`
  starts, the game goes back to needing a real keyboard.

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

- **`data/locations.txt` and `data/overworld.grid` are resolved via a
  compile-time absolute path** (`ANSALON_DATA_DIR`, injected by
  `CMakeLists.txt`), not relative to the executable or the caller's working
  directory. Deliberate scope-limiting decision for a solo/dev-only project
  — the game only works built from this exact source tree. Revisit with a
  real relative-to-executable resolution if this is ever packaged for
  someone else to run outside this source tree.
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
  road generator for a handful of roads.
