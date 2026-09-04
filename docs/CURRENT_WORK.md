# Current work

**Round 3 SFML spike confirmed positive -- "continue down this path" --
still in trial form, real engine migration not yet scoped.** On branch
`sfml-trial-3` (off `master`; do not merge without an explicit go-ahead).
Context: `References/dragonlancemap2.png`'s author (paercebal) gave
explicit permission to use the map this session (see `docs/MAP_NOTES.md`'s
"Source and attribution"), the user asked to see the actual map image
rendered as the overworld rather than round 2's hand-drawn tile atlas (see
the `feedback_dragonlance_sprites_deferred` memory for the full two-round
history), and after seeing a live capture the user said to continue.

What's built: a purely additive `ansalon_sfml_trial` CMake target (SFML
3.0.0 `FetchContent` -- same `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`
configure-step note as round 2, see "Parked: SFML rendering + variant tile
art" below) and `sfml_trial/main.cpp`: loads the real grid
(`world::OverworldGrid`, just for `width()`/`height()`) and real locations
(`world::World`/`WorldLoader`), draws `References/dragonlancemap2.png`
directly as one `sf::Sprite` (no tile atlas), arrow-key-panned camera
clamped to image bounds starting centered on Solace, a colored marker per
`Location` (green towns, red otherwise), a gold camera-start marker, and a
**self-capture**: since this session runs without desktop access (external
GDI/`PrintWindow` capture can't see the SFML window's OpenGL content --
confirmed both ways, `PrintWindow`'s `PW_RENDERFULLCONTENT` only grabs the
window chrome, black client area), the app screenshots its own framebuffer
~1s after launch to `sfml_trial3_capture.png` (gitignored) so the user
--physically away from this PC this session-- can review it as an image
instead of the live window.

The live capture surfaced a real, useful side effect: markers overlaid on
the actual map let the user's own tile-derived `POS` values be checked
against the map's real painted labels for the first time. That became two
milestones (real production data, not spike-scoped, both shipped on
`master`, `sfml-trial-3` rebased past both):

- **Milestone 153** -- 5 of 9 `TOWN` locations corrected.
- **Milestone 154** -- the remaining 16 non-town locations. Found and
  fixed a real bug (Southern Ergoth's `POS` was in open ocean), nudged Pax
  Tharkas, moved Qualinesti onto Qualinost's icon (user-requested).
  Darken Wood and Hopeful Vale independently re-verified against
  `References/TSR 8448 The Atlas of the Dragonlance World.pdf` (page-image
  research -- no extractable text layer) at the user's request; both
  already matched exactly, no coordinate change, just a much stronger
  citation than Milestone 85's original placeholder.

See `docs/MAP_NOTES.md`'s "Town position correction against
dragonlancemap2.png" and "Second position-correction pass" sections.

**User's verdict after seeing all 25 corrected markers live: "looks
good."** All location-position work for this round is done and confirmed.

Next step, not yet started: the user's earlier "continue down this path"
hasn't been scoped into a concrete plan yet. Open questions before that
plan: player movement/collision hookup (the trial only pans a free
camera, no player-state or walking), whether `dragonlancemap2.png` needs
to graduate out of the now-partially-gitignored `References/` into a real
shipped asset location, whether zones/combat stay ASCII. Per this
project's "don't start the next milestone without being asked" rule, plan
this properly (likely `EnterPlanMode`) rather than assuming scope.

Also still open: `master` is several commits ahead of `origin/master` and
hasn't successfully pushed yet -- the `git push` hang from earlier this
session (looked like a credential/write-scope issue, distinct from the
unrelated VPN/RDP problem also worked through this session) was never
resolved. Worth retrying now that the user is back at the keyboard.

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

## Parked: SFML rendering + variant tile art

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
