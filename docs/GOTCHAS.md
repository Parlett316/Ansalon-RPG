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
- **Stick to true 7-bit ASCII** for all map glyphs and line-drawing
  characters (letters, `+ - | / \ *`), not Unicode box-drawing characters.
  This sidesteps an entire class of console code-page/font mismatch bugs,
  and matches the project's explicit "ASCII based" requirement. We
  deliberately do *not* call `SetConsoleOutputCP(CP_UTF8)` — there's nothing
  in our output that needs it, and turning it on would just be one more
  thing that could interact badly with a user's console font.

## Toolchain

- **`cl.exe` (MSVC) is not on PATH** in a plain shell — it's only available
  after sourcing `vcvarsall.bat`/`vcvars64.bat` (the "Developer Command
  Prompt/PowerShell" environment). This was hit directly in this project's
  setup: manually invoking `vcvars64.bat` via a plain PowerShell session was
  the wrong move. **Use CMake's Visual Studio generator instead**
  (`cmake -G "Visual Studio 18 2026" -A x64 ...`) — CMake locates the MSVC
  installation itself (via the same mechanism as `vswhere`) and doesn't
  require the calling shell to have sourced anything. This is the
  recommended flow in `README.md`; the Developer-shell + Ninja flow is
  offered as an alternative for faster incremental builds, not as the
  primary path.
- **PATH doesn't refresh in already-open shells after installing a tool**
  (e.g. `winget install Kitware.CMake`). Observed directly during this
  project's setup — `cmake` wasn't found until a new terminal session was
  started. If a freshly-installed tool "isn't found," try a new shell before
  assuming the install failed.
- **`CMAKE_CXX_EXTENSIONS OFF`** is set deliberately, even though only MSVC
  is used today, so nothing MSVC-permissive-mode-specific creeps in that
  would silently break a future GCC/Clang build. Likewise `/permissive-` is
  passed under MSVC for the same reason — it makes MSVC reject some
  non-standard code it would otherwise silently accept.

## Data / file handling

- **`data/locations.txt`'s path is resolved via a compile-time absolute
  path** (`ANSALON_DATA_DIR`, injected by `CMakeLists.txt`), not relative to
  the executable or the caller's working directory. This is a deliberate
  scope-limiting decision for a solo/dev-only project (see the comment in
  `CMakeLists.txt`) — it means the game only works when built from this
  exact source tree. If this project is ever packaged/distributed, this
  needs to change to a real relative-to-executable resolution (which
  requires platform-specific APIs — `GetModuleFileNameW` on Windows,
  `/proc/self/exe` on Linux — that don't exist yet anywhere in this repo).
- **`WorldLoader` validates all `CONNECT` targets at load time** (every
  target id must correspond to a defined `LOCATION` block) and fails fast
  with a `file:line: message` error if not. This is intentional: the data
  file is hand-edited (an external boundary, even though we're the ones
  editing it), so a typo should surface immediately at startup with a clear
  message, not as a confusing null-pointer crash three commands into a
  playthrough.
- **`DESC`, `NAME`, `REGION`, `TERRAIN` are single-line only** in the current
  data format — there's no multi-line text support. Keep descriptions to one
  line, or extend `WorldLoader`'s grammar deliberately (and update
  `MAP_NOTES.md`) if multi-line text is needed later.

## Map fidelity

- The schematic map's `(row, col)` positions are **stylized, not
  geographically accurate** — they preserve rough relative
  north/south/east/west relationships eyeballed from the reference map
  image, not real distances or exact placement. See `MAP_NOTES.md` for
  what's treated as canon vs. invented for gameplay purposes.
- The ASCII schematic only draws road **lines** between locations that
  share a row or column (pure horizontal/vertical). Diagonal roads still
  work for travel (the data/logic doesn't care), they just don't render a
  visible line on the map grid. Not worth a general line-drawing algorithm
  for a small, hand-placed graph.
