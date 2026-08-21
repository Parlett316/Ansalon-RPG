# Map notes

## Two separate data sources

As of Milestone 2 the world is described by two independent files that
share one coordinate space (480 tiles wide × 320 tall, matching the
reference image's aspect ratio):

- **`data/locations.txt`** — hand-authored named places (grammar below).
- **`data/overworld.grid`** — machine-generated walkable terrain, produced
  by `tools/generate_overworld.py` from the reference map image. **Do not
  hand-author this file from scratch** — hand *corrections* after a
  generator run are fine but will be lost if the generator is rerun (see
  `docs/GOTCHAS.md`).

## `data/locations.txt` grammar

Block-structured plain text, parsed by `world::WorldLoader`. Blank lines and
lines starting with `#` are ignored anywhere.

```
LOCATION <id>        starts a block; <id> is the stable key (lowercase, no spaces)
NAME <text>            display name, rest of line
REGION <text>           region name, rest of line (not read by game logic yet --
                        reserved for the future timeline engine)
TERRAIN <tag>            free-form descriptive tag, informational only (the actual
                        walkable terrain at this tile comes from overworld.grid)
GLYPH <char>              single character drawn on the overworld viewport
TOWN                       (optional, no argument) marks a civilian settlement --
                          a knocked-out player respawns at the nearest TOWN
                          location, full-healed, instead of always Solace; see
                          docs/COMBAT_NOTES.md's "Death: knocked out, not killed"
POS <x> <y>                two non-negative integers -- tile coordinates in the
                          SAME space as overworld.grid (see below for how chosen)
DESC <text>              one-line description, rest of line
END                      closes the block
```

`TOWN` is set on exactly 5 of the 13 locations -- Solace, Haven, Kalaman,
Tarsis, Palanthas -- the ones already carrying a civilian-settlement
`TERRAIN` tag. Fortresses, ruins, the nomadic Plains of Dust village, and
the (deliberately excluded, for lore reasons) elven homelands don't have
it.

Every field is single-line; `WorldLoader` throws a `file:line: message`
error for anything malformed. There is no `CONNECT`/road field anymore —
travel time now comes from actually walking the terrain
(`world::Terrain::minutesToCross` — see "Movement granularity" below), not
a graph. Roads between locations are instead baked into
`data/overworld.grid` as `#` tiles by `tools/generate_overworld.py`'s
`ROAD_PAIRS` list.

## Movement granularity: minutes, not hours (Milestone 67)

`world::TerrainInfo::minutesToCross` (in-game minutes per tile stepped
onto, `GameLoop::tryMoveOverworld`) used to be a flat, whole-hour
`hoursToCross` (1-6 hours per tile). That made every single keypress
consume at least a full hour of `GameState::hoursElapsed` — the same clock
`timeline::Timeline` checks every `PRESENCE` window in `data/timeline.txt`
against — so an ordinary play session (a couple hundred tile-moves,
trivial for a real-time ASCII walker) could blow through the earliest,
narrowest story windows (Solace's is only 48 hours wide) before a player
even meant to rush anything. `minutesToCross` is a straight x15 scaling of
the old hour values (1-6 hours -> 15-90 minutes) into a new small
`GameState::minutesElapsed` remainder (0-59, rolled into the existing
`hoursElapsed` on overflow — see `GameLoop::tryMoveOverworld`) — same
relative tuning between terrain types, just four times finer-grained, so
the same amount of real playtime now costs less simulated time.
`data/timeline.txt`'s day windows are deliberately untouched by this —
they're individually sourced against each novel's own elapsed-time cues
(see `docs/TIMELINE_NOTES.md`), and rescaling them would invalidate that
research for no reason. Rest/BedRest's flat 8-hour cost is untouched too —
a night's rest is still a night's rest regardless of movement granularity.

## How location `POS` values were chosen

The reference map's place-name labels are not legible at any resolution
practical to inspect (a 10125×6750px image compressed for viewing). Rather
than guess at pixel coordinates and claim false precision, the 8 locations
in Milestone 2 were placed using **relative Dragonlance geography and the
overall visible shape of the continent** (the landmass outline, the central
mountain spine, the Blood Sea, and the large tan Plains-of-Dust/Kharolis
region are all clearly identifiable even at low resolution): Solace in
Abanasinia north of the central mountains; Qualinesti forest to its west;
Pax Tharkas in the mountains south of Qualinesti; Xak Tsaroth in swampy
lowlands east of Haven; Plains of Dust and Tarsis further south. **Treat
these coordinates as approximate**, not authoritative — the same honesty
standard as Milestone 1's schematic positions, just in a bigger coordinate
space.

## How `data/overworld.grid` is generated

`tools/generate_overworld.py` (see its own docstring for full detail) works
in two phases:

1. **Discovery**: downsample the reference image to the target grid
   resolution (box filter, so each tile gets the *average* color of its
   source region — this naturally smooths individual map icons like tiny
   tree/mountain glyphs into an overall regional color, which is the biome
   signal actually wanted per tile), then reduce to `NUM_COLORS` dominant
   colors via Pillow's built-in median-cut quantizer. Saves a large
   inspectable preview PNG (`data/overworld_preview.png`, gitignored) and
   prints each color's RGB and coverage percentage.
2. **Generation** (after a human fills in `INDEX_TO_TERRAIN`, mapping each
   discovered color index to a terrain key by eye, comparing the preview
   against the reference map): re-run to classify every tile and write
   `data/overworld.grid`, then draw road tiles between the coordinate pairs
   in `ROAD_PAIRS`.

This two-phase design exists because the source map is **painted/textured**
(mountains are little brown triangle icon clusters on green, forests are
tree-icon clusters, not flat color-coded regions) — there's no reliable way
to know "this shade means forest, not grassland" without a human looking at
it once. It is not a one-shot, fully automatic pipeline, and isn't meant to
be.

**Current classification** (16 colors, chosen by inspecting
`overworld_preview.png` against the reference map): the dark teal ocean
background split across several near-identical color-quantization buckets
(JPEG compression noise), all mapped to `ocean`; the Blood Sea's solid
maroon fill was distinct enough to get its own bucket (`blood_sea`); bright
and darker green both mapped to `forest`; the large tan region (Plains of
Dust, Kharolis lowlands) mapped to `savannah`; olive-brown mapped to
`hills`; brown mapped to `mountain`; a lighter blue mapped to `river`
(actually shallow coastal water — thin river lines mostly don't survive
downsampling at this resolution, see `docs/GOTCHAS.md`). **Glacier, bog, and
salt-flat did not emerge as distinct colors** at 16 buckets and don't appear
in the generated grid.

## The High Clerist's Tower (Milestone 35)

The first location placed outside the original 8's tight Abanasinia
cluster, and the first placed by actually cropping and inspecting the
reference image at full resolution (`References/DragonLance_-_Continent_of_
Ansalon_-_Age_of_Despair.jpg` is 10125×6750 — the earlier locations were
placed from the *shape* of the continent alone, since the labels were
illegible at any practical viewing resolution; Python + Pillow, already a
project dependency for `tools/generate_overworld.py`, made cropping tight
regions for a legible read straightforward). The image's pixel scale
matches the grid exactly (10125/480 = 6750/320 = 21.09 px/grid-unit), so a
label's pixel position can be converted to a grid `POS` directly, then
cross-checked against Solace's already-known `POS 185 230` for sanity.
`POS 173 100` for the Tower was derived this way — still **approximate**,
per this file's standing honesty rule, since it's a human eyeballing a
label's position on a downsampled crop, not a precise measurement.

**A road drawn this far north surfaces a real, pre-existing classification
gap.** The new `("solace", "high_clerist_tower")` road crosses roughly 130
grid units of the Vingaard Mountains — and inspecting the regenerated
`data/overworld.grid` around that stretch shows a large number of tiles
classified `!` (`blood_sea`, impassable) that clearly aren't the Blood Sea
on the reference map at all: the 16-color global quantization apparently
bucketed dark reddish-brown mountain-shadow shading into the same palette
index as the Blood Sea's maroon fill, somewhere the classification was
never checked before because no content existed this far from the original
cluster. This doesn't break anything gameplay-wise — the drawn road tiles
are always the `#` character regardless of what was underneath, so the
literal path to the Tower is guaranteed walkable — but wandering off-road
in this region will show visually-wrong "Blood Sea" tiles that are really
just mountain shadow. Not fixed this milestone (would mean rerunning
Phase 1 discovery with a higher `NUM_COLORS` and re-auditing the *entire*
existing classification, not just this new corner of it — out of scope for
a single-location content milestone); worth fixing before any future
milestone adds more content near mountainous terrain elsewhere on the map.

## Ice Wall (Milestone 36)

The first location placed with **no `ROAD_PAIRS` entry at all**, on purpose:
Ice Wall Castle is on a separate, sea-locked landmass south of Kharolis/
Tarsis, confirmed by cropping the reference image at full resolution
(`crop_icewall_grid.png` in that session's scratchpad) -- open ocean the
whole way, no land bridge, same situation Milestone 35 found (and skipped)
for Sancrist Isle. This time the milestone added a real sea-travel
mechanic instead of skipping the location -- see `docs/ARCHITECTURE.md`'s
"Sea travel" section.

The region itself is confirmed, not guessed: the reference map labels it
"Black Ice Valley"/"Ice Mountain Bay" south of Tarsis, and `TSR 2143
Player's Guide to the Dragonlance Campaign` independently confirms
"Icewall"/"Ice Mountain Bay" as the Thanoi/Ice Barbarians' home region in
that same spot. `POS 110 307` was derived the same pixel-crop-and-scale way
as the Tower, then nudged a few tiles onto confirmed-walkable land in the
already-generated `data/overworld.grid` (the coastline in the generator's
16-color classification doesn't land exactly where a human eye would draw
it -- same imprecision already disclosed for every other placement).

**A small, targeted hand-edit, not a regenerate.** Milestone 2's
classifier has never produced glacier (`:`) anywhere on the map (noted
since Milestone 2's writeup above) -- so without intervention, Ice Wall's
tile and its surroundings would render as green grassland, which reads
wrong for a location described as icy ruins. Rather than rerun the whole
discovery/classification pipeline (out of scope for one location), a
~46-tile patch around `POS 110 307` was hand-repainted from grassland/hills
to glacier directly in `data/overworld.grid`. Same standing caveat as any
hand-edit to this file: it will be silently lost if the generator is ever
rerun from scratch.

## Silvanesti (Milestone 37)

Unlike Ice Wall, this location needed **no boat, no hand-edit, and no
`ROAD_PAIRS` skip** — a genuinely simpler placement than either of the
prior two milestones. Cropping and gridding the reference image
(`crop_silvanost_grid.png` in that session's scratchpad) around Silvanost
confirmed the Thon-Thalas River loops around the Silvanesti peninsula as a
river, not open ocean — the same `r`/shallow-water terrain type already
classified elsewhere on the map, matching `dwn_full.txt`'s own text (the
party's griffons land on the riverbank and the party crosses on foot via
"the ferry landing," not by any special travel mechanic). `POS 312 231`
landed directly on forest terrain in the already-generated
`data/overworld.grid` — checked before writing the location, not assumed.

**The road is fully invented, more so than any prior one.** Every other
`ROAD_PAIRS` entry connects locations a party could plausibly walk between
in the story; here, the actual journey is three days by griffon flight
(`dwn_full.txt` line 3774), with no on-page overland route at all. The
`("solace", "silvanesti")` road (~127 tiles, almost due east, comparable
in length to the Tower's road) exists purely so the location is reachable
in a walking-based game, disclosed as invented rather than left
unstated — same honesty standard as every other placement in this file,
just a bigger gap between "what's sourced" and "what's drawn."

**Regenerating wipes hand-edits — hit for real this time.** Adding the
new road pair required rerunning `tools/generate_overworld.py`, which
silently erased Milestone 36's hand-painted glacier patch around Ice Wall
(exactly the caveat this file already documented, not a surprise when it
happened) — reapplied identically afterward (same coordinates, same tile
count, 46 tiles) before moving on. Worth remembering for any future
milestone that touches `ROAD_PAIRS`: check for other hand-edits in this
file before regenerating, not after.

## Kalaman (Milestone 39)

The first location placed by reading labels directly off the reference
image rather than relative geography alone — cropping
`References/DragonLance_-_Continent_of_Ansalon_-_Age_of_Despair.jpg` at
full resolution around grid (150-260, 60-160) turned out to have fully
legible place-name labels (`crop_tower_kalaman.png`/`crop_kalaman_tight.png`
in that session's scratchpad), unlike the Milestone 2-era assumption that
labels were illegible at any practical resolution. A "Kalaman" city icon on
"Kalaman Bay" was located directly, next to "Dargaard Keep" and the
"Estwilde" region label, east of the already-placed `high_clerist_tower`
across the "Hinterlund" plains — matching the novel's own description
(*Dragons of Spring Dawning*: "heading for Kalaman, northwest of Flotsam,
around the cape of Nordmaar"; "the Dargaard Mountains that divided
Solamnia from Estwilde... Kalaman and its harbor").

**Cross-checked against a known point before trusting the new one.**
Converting `high_clerist_tower`'s already-recorded `POS 173 100` to a pixel
position on this same crop landed correctly on the crop's own fortress
icon, validating the pixel-to-grid conversion before using it to place
Kalaman. The raw pixel estimate for Kalaman itself landed on a `data/
overworld.grid` tile classified mountain/hills (Northern Dargaard Mountains
foothills, visible on the reference map right next to the city, not a
misclassification this time); nudged east onto confirmed grassland
immediately next to the bay/river tiles, same "nudge onto walkable
terrain" precedent Ice Wall set. Final: `POS 262 73`, `REGION Estwilde`.

**`("high_clerist_tower", "kalaman")` added to `ROAD_PAIRS`** — the
nearest already-modeled location, not the book's own unmodeled route
(Palanthas and Vingaard Keep aren't zones yet). Regenerating wiped Ice
Wall's hand-painted glacier patch again, exactly as documented above and
already hit once at Milestone 37 — this time the patch's 46 exact tile
coordinates were captured to a scratch file *before* regenerating and
reapplied afterward with a diff confirming byte-for-byte identical
placement, rather than re-deriving the patch by eye. Worth doing the same
capture-before-regenerate step for any future `ROAD_PAIRS` change.

## Palanthas (Milestone 44)

Placed the same "crop and cross-check against a known point" way Kalaman
was, but with a calibrated-gridline overlay on the crop rather than raw
pixel-by-eye reading, which surfaced something the Kalaman pass didn't
catch: `high_clerist_tower`'s own recorded `POS 173 100` does **not** land
exactly on the Tower's map icon — the icon itself reads at roughly
(178,104), a consistent (+5,+4) bias between "recorded `POS`" and "map
icon" in this corner of the reference image (`References/DragonLance_-_
Continent_of_Ansalon_-_Age_of_Despair.jpg`, 10125×6750px,
21.09375 px/grid-unit). Worth remembering for any future placement near
here: cross-check against the Tower's *icon*, not its *recorded `POS`*, or
the same bias will silently propagate.

Palanthas's own icon read at raw grid ≈(171,90); applying the same bias
correction landed on `(166,86)`, classified mountain (`A`) in
`data/overworld.grid` — plausible on its own (Palanthas sits "almost
surrounded by arms of the Vingaard Mountains" per the Atlas of the
Dragonlance World, p.96), but not walkable. Checked the grid directly (not
assumed) rather than guessing a nudge direction: `(167,85)`, one tile
north, is grassland (`.`) — the shore of the Bay of Branchala on the
reference map — so that's the final `POS`, same "nudge onto confirmed
walkable terrain" precedent Kalaman/Ice Wall already set, and the smallest
possible nudge (distance 1).

**`("high_clerist_tower", "palanthas")` added to `ROAD_PAIRS`.** Same
capture-before-regenerate procedure Milestone 39 established for Ice
Wall's glacier patch — captured its 46 tile coordinates first this time
too, regenerated, reapplied, diff-confirmed byte-for-byte identical
afterward. The new road turned out cleaner than expected: a
column-by-column check of the generated path (at the time, a continuous,
unbroken diagonal from `(173,100)` to `(167,85)` — see "Road 4-connectivity
fix" below for why that diagonal shape has since changed, confirmed tile by
tile rather than assumed from the two endpoints) crosses ordinary
mountain/hills/shallow-water/grassland the whole way and does **not** touch
the nearby `!` Blood-Sea-classified pocket that sits a few tiles further
west/south at roughly the same rows — worth knowing if a future placement
in this same corner of the map needs to route around that pocket, since
this road happened to clear it without any special handling.

## Godshome (Milestone 45)

Unlike every prior placement, the reference image turned out to be
**directly legible at this corner of the map without any pixel-bias
correction** — a wide crop around the Khalkist/Taman-Busuk mountains
south of Kalaman showed "Godshome" as a real, standalone label,
distinct from "Ruins of Godshome" roughly 15 grid units to its
northwest, both sitting immediately next to "Neraka" inside a region
clearly labeled "Taman-Busuk" — matching the book's own geography (and
Tasslehoff's own "no, not that Godshome" line) exactly.

Cropped and gridded (`crop_kalaman_calib.png`/`crop_kalaman_godshome_grid.png`
in that session's scratchpad) the same "calibrated gridline overlay,
cross-check against a known point" method as Kalaman/Palanthas, confirming
Kalaman's own recorded `POS 262 73` still lands correctly on its map icon
before trusting the new reading. The "Godshome" label itself reads at
approximately grid **(267, 139)** — south of Kalaman across the Estwilde
plains and deep into the Khalkist mountain range.

Checked `data/overworld.grid` directly at that coordinate before writing
anything: `^` (hills, walkable) — not one of the many `!` (Blood
Sea-coded) tiles that blanket this entire mountainous region, the same
pre-existing classification gap this file already flagged near the Tower
(mountain shadow bucketed into the Blood Sea's color index during
Phase 1's 16-color quantization). No nudge needed.

**`("kalaman", "godshome")` added to `ROAD_PAIRS`** — nearest
already-modeled location, both geographically (~66 grid units) and
narratively (the Heroes' immediately-preceding stop). Same "invented road
standing in for actual travel" disclosure as Silvanesti's griffon-flight
road: the book's party makes this leg by brass dragon at night, not on
foot. Ice Wall's 46-tile glacier patch was captured to a scratch file
before regenerating and reapplied afterward, diff-confirmed identical (a
Python set-equality check rather than a byte-for-byte file diff this
time, same effective guarantee) — same procedure as every `ROAD_PAIRS`
change since Milestone 37.

## Neraka (Milestone 46)

Cropped and gridded a wide region centered on Godshome's own coordinate
(scratch files `crop_neraka_grid.png`/`crop_neraka_grid2.png`), same
calibrated-gridline-overlay method as Kalaman/Palanthas/Godshome. Read
Godshome's own icon back at approximately (268.5, 139.2) against its
recorded `POS 267 139` first, as a cross-check — close enough to confirm
this corner of the map still needs no pixel-bias correction, consistent
with Milestone 45's finding.

"Neraka" reads as a distinct labeled icon (a star-in-a-circle, the map's
own symbol for the Temple) roughly 6 grid-units east and 1 unit south of
Godshome, at approximately grid **(273, 140)** — matching the book's own
geography exactly (the party is "this near to Neraka" almost immediately
after leaving Godshome).

Checked `data/overworld.grid` directly at that coordinate: `^` (hills,
walkable) — not one of the region's many `!` (Blood Sea-misclassified)
tiles. No nudge needed.

**`("godshome", "neraka")` added to `ROAD_PAIRS`** — nearest
already-modeled location, and by far the shortest road this project has
drawn yet (~6 grid units), matching how close together the book places
them. Ice Wall's 46-tile glacier patch was captured (this time as a full
before/after row-range snapshot rather than a character-filtered set, to
avoid the risk of a filter predicate silently missing part of the patch)
and reapplied afterward, diff-confirmed byte-for-byte identical — same
procedure as every `ROAD_PAIRS` change since Milestone 37.

## Road 4-connectivity fix

A player reported getting stuck at overworld tile `(174,106)`, unable to
reach `high_clerist_tower` (`POS 173 100`) despite a road connecting them.
Root cause: `draw_line()`'s Bresenham implementation could advance both `x`
and `y` in the same step, so two consecutive drawn road tiles could end up
only *diagonally* adjacent — but the game's movement is 4-directional for
both arrow keys and WASD (`src/render/Console.cpp`); true diagonal movement
exists only via the less-discoverable roguelike `y`/`u`/`b`/`n` keys. Any
road whose endpoints weren't exactly horizontal/vertical/45° could contain
diagonal-only jumps — the `high_clerist_tower`→`palanthas` road above,
described at the time as "a continuous, unbroken diagonal," was exactly
this trap.

**Fix**: `draw_line()` now stamps a tile after each individual axis update
instead of once per loop iteration, so a would-be diagonal step becomes two
sequential orthogonal stamps (a 1-tile "staircase" corner) — the error-term
math and axis selection are untouched, so roads are still straight-ish
lines, not a pathfinder; they just pick up a tile or two extra at each
staircase corner, harmless since roads have a flat `minutesToCross = 15`
regardless of underlying terrain. `data/overworld.grid` was regenerated
with the fix (Ice Wall's 46-tile glacier patch captured, reapplied, and
diff-confirmed byte-for-byte identical afterward, same procedure as every
prior regeneration), then every one of the 12 `ROAD_PAIRS` — plus the
specific reported `(174,106)` tile — was reverified end-to-end reachable
using a 4-directional-only BFS against the same passability rule
`world::terrainFor` uses at runtime (a throwaway verification script, run
once and deleted, per this project's self-test-then-delete convention).

## Extending the map

**Adding a location**: pick a `POS` that preserves its rough real/canon
direction relative to its neighbors (precision isn't required — see above).
Add it to `ROAD_PAIRS` in `generate_overworld.py` if it should have a road,
then rerun the generator. Note in this file whether the placement is drawn
from canon or invented, so this stays a reliable record.

**Improving terrain accuracy**: rerun `generate_overworld.py`'s discovery
phase with a higher `NUM_COLORS` (more buckets to hand-assign, but finer
distinctions — e.g. recovering glacier/bog/salt-flat as their own colors),
or hand-paint specific tiles in `data/overworld.grid` directly (single
ASCII characters, grammar in `world/Terrain.cpp`) — just remember hand-edits
don't survive a regenerate.
