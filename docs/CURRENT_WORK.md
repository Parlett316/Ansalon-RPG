# Current work

**Full SFML engine migration -- decided and started, Phase 1 shipped and
confirmed.** On branch `sfml-trial-3` (off `master`; do not merge without
an explicit go-ahead). History: the round-3 real-map spike (real
`dragonlancemap2.png` as the overworld, `sfml_trial/main.cpp`) got a
"continue down this path" verdict, which led to two real production data
fixes shipped straight to `master` (Milestone 153/154, then a follow-on
Milestone 155 -- see `docs/MAP_NOTES.md` and `docs/MILESTONES.md`, not
repeated here), then real collision-checked player movement in the trial.
Full history is in git log / `docs/MILESTONES.md`, not repeated here --
this file only tracks what's still actionable.

**The user then committed to a full migration** (overworld, zones, and
combat all move to SFML -- not the "hybrid" alternative, which research
found would need an SFML window and the Windows console coexisting
mid-session, no precedent in this codebase) and to **real pixel-space
rendering per screen**, not a monospace-grid recreation (already tried
once in August, rejected as "looks almost exactly the same"). See
`docs/ARCHITECTURE.md`'s SFML section for the architecture this unlocked
and the one real wrinkle it surfaced (`Console`'s methods are `static` on
a concrete class, not an interface -- a true drop-in swap needs `Console.h`
restructured via PIMPL, deferred until a phase actually needs
`GameLoop::run()`'s full dispatch).

**Phase 1 shipped this session: a real, pixel-space overworld screen.**
New standalone target `ansalon_sfml_phase1` (`sfml_phase1/main.cpp`, not
reusing `GameLoop::run()` -- see the `Console` wrinkle above) that loads a
real save file read-only via `game::SaveGame`, walks the real map with
real collision (extending what the trial proved), and renders a real
pixel-space sidebar (character name/level/race/class, HP, in-game day/
time, a short event log) using SFML text against a placeholder Consolas
system font -- not a terminal recreation. Unimplemented keys (zone entry,
talk, shop, inventory, journal, rest, etc.) log a plain "not yet in this
build" line instead of doing anything. Verified: clean rebuild (zero new
`/W4` warnings) and the user testing it live at the keyboard against a
real save. **User's verdict: "looks good."**

Phase 1 committed as `778a57c`.

**Phase 2 shipped this session too: zone interiors, in the same
`ansalon_sfml_phase1`/`sfml_phase1/main.cpp`** (extended in place, not a
new target -- this is the one growing prototype the migration
accumulates into). Added `world::Zone`/`ZoneTile`/`ZoneLoader`/
`ZoneCatalog` to the target; `state.mode` now genuinely switches between
Overworld and Zone rendering/movement, matching `game::GameState` exactly.
Real collision (`zoneTileFor`, POIs always passable), real entry/exit via
`Enter` (including `PORTAL`-nested sub-zones like Solace's Inn, pushing/
popping `GameState::zoneStack`), placeholder pixel-space tile colors (no
reference art exists for interiors, unlike the overworld's real map) with
a small name label at each POI marker. Talk/Shop/etc. stay the same
"not yet in this build" placeholders Phase 1 already had, even inside a
zone. Verified: clean rebuild (zero new `/W4` warnings), user tested live
-- walked into Solace, into its Inn via the portal, and back out through
both exits. **Confirmed working.**

**Not yet committed.**

**Next step:** decide what's next (the roadmap below lists every screen
still ASCII-only -- combat is the natural next one, being the biggest
remaining gap, but it's the user's call). Per this project's "don't start
the next milestone without being asked" rule, that's a decision for the
user to make, not to assume.

## Full-migration roadmap (screens still ASCII/terminal-only)

Each needs its own real pixel-space design pass -- not a mechanical port,
same reasoning that ruled out the monospace-grid approach above. Rough
size/complexity noted from this session's research, not a commitment to
this order:

- ~~Zone interiors (`drawZoneFrame`)~~ -- done, see above.
- Combat (`drawCombatFrame`) -- the biggest and most complex remaining
  screen (tactical grid, HP roster, target/spell pickers).
- Character sheet, spellbook, dialogue, generic picker, ask-input, shop,
  inventory, full log, world map, journal, help -- 10 more screens, each
  smaller than zones/combat.
- Character creation and the save-slot menu use plain `std::cin`/
  `std::cout` before any window exists -- can stay as-is indefinitely,
  not part of this migration.
- Deciding when (if ever) this code gets promoted to replace `ansalon_rpg`
  outright -- only once enough of the above is real, not before.

Milestones 146-152 are all implemented and documented on
`master` (146 terrain smoothing, 147 mountain glyph, 148 region-boundary
highlighting, 150 header/border scroll-drift fix, 151 two more Astinus
SUBJECT entries, 152 Fireball/Delayed Blast Fireball real area damage);
146-150 are also interactively confirmed. The `terrain-smoothing` branch is
fully merged into `master` -- no longer an open decision.

This session also researched (but did not implement, at the user's choice
of scope) two follow-on combat-visuals ideas raised alongside Milestone
152: per-cell color on the combat grid (currently impossible -- `writeBoxed`/
`BoxLine`, the "organic" screen family's rendering, only supports one color
per whole line, see `docs/ARCHITECTURE.md`'s Milestone 32 note) and an
animation/timing primitive (`<chrono>`/sleep, currently absent everywhere in
`src/`). Neither is started; either is a reasonable next step if the user
wants combat visuals to keep moving.

Two things carry over to the next session:

## Playtest backlog

Implemented, documented, and verified via piped-smoke-test, but not yet
interactively walked with a real save/keyboard -- worth clearing before
piling on more unverified content. Full sourcing/detail for each is in its
`docs/MILESTONES.md` entry.

- **152** -- Fireball/Delayed Blast Fireball are now real area attacks
  (radius 2 grid cells, Chebyshev distance). Fight a multi-instance group
  (e.g. Goblins), memorize Fireball, cast it at one instance while a second
  is within 2 cells, and confirm both take the same damage with both named
  in the log; separately confirm a solo/isolated target still reads as a
  clean single-target hit. Save slot 2 (`Regan`, level 20 Human Mage) should
  already have Fireball available.
- **137** -- day-gated Astinus dialogue, fixing 12 shipped spoilers. Talk to
  Astinus in Palanthas before day 2/3/12/17 and again after day 160; confirm
  both halves read correctly.
- **138/139** -- 6 Astinus SUBJECT topics (Kagonesti, gnomes, gully dwarves,
  minotaurs, ogres/Irda, Reorx). Ask Astinus about each.
- **142** -- Harpy/Griffon/Stirge (Monster Manual). Trigger wilderness
  encounters on hills/mountains (Griffon), grassland/hills (Harpy), forest
  (Stirge).
- **143** -- `a_widows_due` DELIVER quest at Kalaman. Talk to the Curiosities
  Cart, find the Furtive Trader POI, deliver the wedding band.
- **145** -- 3 Astinus SUBJECT topics (Fizban, Silvara, Berem/the Everman),
  each day-gated. Ask before/after day 192/69/193 respectively.
- **149** -- 3 Astinus SUBJECT topics (Gilthanas, Lord Soth, Ariakas), each
  day-gated. Ask before/after day 69/190/193 respectively.
- **151** -- 2 more Astinus SUBJECT topics (Alhana Starbreeze, Porthios),
  each day-gated. Ask before/after day 51/70 respectively.

## Parked: SFML rendering + variant tile art (round 2, `sfml-trial-2`)

**Superseded in direction, not necessarily obsolete.** This is round 2's
tile-atlas approach (hand-drawn 32px terrain tiles); the active migration
above instead renders the real map image directly, pixel-space. Still
worth keeping this note around in case tile-based art becomes relevant
again for a screen the real map can't represent (a zone interior, say) --
just don't assume it's the plan for the active migration above.

Not abandoned, not committed to -- purely the user's call whenever (or if)
they revisit it. Don't propose pushing this further unprompted; this project
has a real history of visual-change attempts that didn't land (SFML with no
art, twice-rejected ANSI truecolor shading), and this round's two checkpoints
("looks a bit better", then "good enough, stop here for now") were engaged
with productively but still didn't convert into a commit-to-it decision.

What exists: an SFML window rendering real overworld data with actual
pixel-art sprite tiles (scrolling camera, per-tile animation) hit and fixed a
real bug (a dangling `std::string&` in a hand-rolled JSON parser). On top of
that, Python/Pillow mockups added 6 art variants per terrain code (forest,
grassland, hills, mountains, savannah, bog, salt flat, glacier), picked
per-tile via a stable position hash so large same-terrain regions -- bigger
now after Milestone 146's smoothing -- stop reading as repeating wallpaper.

To resume:

- SFML trial code: `git checkout sfml-trial-2 && git stash pop` (uncommitted
  `CMakeLists.txt` target `ansalon_sfml_trial` + `sfml_trial/main.cpp`).
- Variant art already exists and is proven, just not wired in:
  `References/variant_tiles6.png` (forest/grassland/hills) and
  `References/variant_tiles_extra.png` (mountains/savannah/bog/salt-flat/
  glacier), each a 6-column strip; the position-hash selection logic
  (`(x*73856093) ^ (y*19349663)`) is proven in the mockup script. Porting
  both into `terrain_tileset.json`'s schema (single rect per code -> list of
  rects) and `sfml_trial/main.cpp`'s tile lookup is the concrete next step,
  not a re-derivation.
