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
                          (or SEA_LOCKED) location, full-healed, instead of
                          always Solace; see docs/COMBAT_NOTES.md's "Death:
                          knocked out, not killed"
SEA_LOCKED                 (optional, no argument) marks a location reachable
                          only by a one-time scripted BOAT voyage -- also a
                          valid knockout wake-up point even though it isn't a
                          TOWN, so a knocked-out player is never stranded
                          somewhere with no way back; see
                          docs/COMBAT_NOTES.md's "Death: knocked out, not
                          killed"
POS <x> <y>                two non-negative integers -- tile coordinates in the
                          SAME space as overworld.grid (see below for how chosen)
DESC <text>              one-line description, rest of line
END                      closes the block
```

`TOWN` is set on exactly 5 of the 15 locations -- Solace, Haven, Kalaman,
Tarsis, Palanthas -- the ones already carrying a civilian-settlement
`TERRAIN` tag. Fortresses, ruins, the nomadic Plains of Dust village, and
the (deliberately excluded, for lore reasons) elven homelands don't have
it.

`SEA_LOCKED` is set on exactly 1 location -- Ice Wall Castle, the only
place reachable solely by a one-time `BOAT` voyage (see
`docs/ARCHITECTURE.md`'s "Sea travel"). Added at Milestone 90 to close a
softlock: without it, a player knocked out on the glacier next to Ice Wall
(fighting the Thanoi the `frostreaver_salvage` quest requires) woke up in
Tarsis, and the Knight's Runner's `BOAT` trip is deliberately one-time
(Milestone 88), so there was no way back.

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

## Higher-fidelity map re-derivation (Milestone 85)

Every placement above was derived from
`References/DragonLance_-_Continent_of_Ansalon_-_Age_of_Despair.jpg`, a
10125x6750 JPEG whose labels were illegible at any practical resolution
for the original 8 locations, and fought JPEG compression noise plus a
coarse 16-color classification for every placement since. The user found
a much higher-fidelity map, `References/dragonlancemap2.png` (8192x5461,
clean linework, legibly labeled down to small settlements), and this
milestone re-pointed the whole pipeline at it and re-derived every
location's `POS` from actually-read labels instead of relative-geography
guesses.

**A second image, `References/map-mt98e24b.png`, was investigated and
ruled out as a data source.** It's an ASCII-art conversion of the same
map the user also provided; zooming in shows character choice follows
local luminance/density (a cosmetic photo-to-ASCII filter), not terrain
semantics -- there's no reliable way to recover "this character means
forest" from it. Per the user it's inspiration for a possible future
overworld-rendering style, unrelated to this milestone's data work -- not
acted on here.

**Terrain classification improved substantially, not just location
precision.** `NUM_COLORS` was bumped from 16 to 32 and, rather than
judging raw RGB triples by eye as before, each candidate bucket was
verified by an index mask (highlighting one discovered color at a time in
red against the rest of the downsampled image) before assigning it a
terrain key -- see the `INDEX_TO_TERRAIN` comment in
`tools/generate_overworld.py` for the full per-index reasoning. Two
concrete wins over the old classification:
- **Glacier is recoverable this time** (`:`, previously undiscoverable at
  any tried `NUM_COLORS`) -- one discovered color traces the Icewall
  Glacier, a Sancrist/Ergoth-area highland, and a barren "Northern
  Wastes" patch cleanly. Ice Wall Castle's new `POS` lands directly on
  real generated glacier (confirmed by checking a wide neighborhood of
  tiles around it, not just the single tile), so **Milestone 36's
  46-tile hand-painted glacier patch was retired, not reapplied** -- it's
  no longer needed and the standing "hand-edits don't survive a
  regenerate" caveat no longer applies to Ice Wall's surroundings.
- **The Blood Sea's color bucket traces only the Blood Sea itself** (plus
  a couple of stray single-tile dots elsewhere, most likely small
  red-inked region-boundary dashes on this map bleeding into the same
  quantization bucket -- harmless, since nothing routes through them).
  This map does **not** reproduce the old JPEG's mountain-shadow-into-
  Blood-Sea misclassification documented above in the High Clerist's
  Tower section -- that was specific to the old source image's
  compression artifacts, not a limitation of the classification method
  itself.

Bog and salt_flat still did not emerge as distinct colors at
`NUM_COLORS=32` -- same honest gap as before, not invented. Forest vs.
plains vs. hills also still don't cleanly separate by flat color (mask
inspection showed these buckets are mostly scattered shading/hatch-line
noise from the map's painted texture, not one coherent region each) --
approximated the same way the old classification was, by G-vs-R channel
lean per bucket, disclosed as approximate rather than treated as precise.

**11 of the 15 locations were placed from a directly legible label or
icon this time** (Solace, Haven, Xak Tsaroth, Qualinesti -- city labeled
"Qualinost" -- Pax Tharkas, Tarsis, High Clerist's Tower, Silvanesti --
city "Silvanost" -- Kalaman, Palanthas, Godshome, Neraka), plus Plains of
Dust (a region label, not a settlement icon -- placed centrally within
the labeled region on savannah/hills terrain, same spirit as the
original placement). Each was read via a full-resolution crop with a
burned-in pixel-coordinate grid overlay (gridlines every 25-50px,
labeled), converted to grid coordinates at this image's
8192/480 = 5461/320 ~= 17.07 px/grid-unit scale, and cross-checked
against at least one other already-converted point in the same region
before trusting nearby readings -- the same discipline Milestone 44
established after finding a silent pixel bias could otherwise propagate.
No bias was found this time; e.g. High Clerist's Tower and Palanthas'
relative offset came out consistent with the old map's own finding (the
Tower sitting just southeast of Palanthas) even though every individual
coordinate moved.

**Darken Wood and Ice Wall Castle still have no direct label on this map
either** (checked multiple crops around both) -- re-anchored using the
same relative-geography method the originals used: Darken Wood
positioned between the newly-placed Solace/Haven/Qualinesti cluster
(which came out much tighter together on this map than the old one
implied), Ice Wall Castle placed centrally within the newly-generated
glacier landmass southwest of Tarsis, in the same relative direction the
old placement used. Both remain disclosed as approximate, per this file's
standing honesty rule.

Every location's `POS` (`x y`, old -> new):

| Location | Old | New | Basis |
|---|---|---|---|
| Solace | 185,230 | 191,200 | direct label |
| Darken Wood | 192,248 | 189,208 | approximate (re-anchored) |
| Haven | 205,250 | 189,213 | direct label |
| Xak Tsaroth | 215,265 | 202,203 | direct label |
| Qualinesti | 150,245 | 184,217 | direct label ("Qualinost") |
| Pax Tharkas | 160,270 | 192,221 | direct label |
| Plains of Dust | 205,285 | 254,258 | region label |
| Tarsis | 170,300 | 197,274 | direct label |
| High Clerist's Tower | 173,100 | 194,95 | direct label |
| Ice Wall Castle | 110,307 | 150,305 | approximate (re-anchored) |
| Silvanesti | 312,231 | 341,236 | direct label ("Silvanost") |
| Kalaman | 262,73 | 267,69 | direct label |
| Palanthas | 167,85 | 190,79 | direct label |
| Godshome | 267,139 | 294,134 | direct label |
| Neraka | 273,140 | 301,137 | direct label |

`GRID_WIDTH=480` was left unchanged -- the new image's aspect ratio
(8192/5461 ~= 1.4998) is close enough to the old one (10125/6750 = 1.5)
that the generated grid is still 480x320, so no other code needed to
change. `ROAD_PAIRS` itself is unchanged (same 14 connections); only the
coordinates they resolve to moved. All 14 were re-verified end-to-end
reachable with a throwaway 4-directional BFS against the real generated
grid (same self-test-then-delete approach the "Road 4-connectivity fix"
above used), and every location's `POS` was confirmed to land on
passable terrain -- 14 of the 15 land on a drawn road tile automatically
(a location connected by at least one `ROAD_PAIRS` entry always does, per
`docs/GOTCHAS.md`); Ice Wall Castle, which by design has no
`ROAD_PAIRS` entry, was checked directly and confirmed to sit solidly
inside the glacier landmass, not an isolated tile.

The old reference JPEG stays in `References/` -- this milestone's writeup
and every earlier one in this file still cite it, and there's no reason
to delete a still-referenced historical asset.

## Thorbardin and Sancrist Isle (Milestone 86)

Two new locations, placed with the same full-resolution-crop-plus-pixel-
grid-overlay method Milestone 85 used throughout, added at the user's
request after researching *Dragons of Winter Night* turned up strong,
book-significant content for both (see `docs/TIMELINE_NOTES.md` for the
full sourcing).

**Thorbardin** reads directly off the reference map at pixel (3230,4080),
converting to grid `POS 189 239` — close to the already-placed Pax
Tharkas (`192 221`), matching the books' own "the only way through the
Kharolis Mountains was through Thorbardin" geography. Lands on `^`
(hills) in the generated grid; `("pax_tharkas", "thorbardin")` was added
to `ROAD_PAIRS`, so the location's own tile becomes a `#` road tile
regardless, per the standing "every road-connected location's `POS` ends
up on a road tile" guarantee (`docs/GOTCHAS.md`).

**Sancrist Isle** is anchored to Castle Uth Wistan specifically (the
actual site of Sturm's Knights' Trial), not a generic point on the
island — same "anchor to the specific named site" precedent Godshome and
Neraka set for their own regions. Reads at pixel (945,2622), converting
to grid `POS 55 154`. Lands on `%` (forest) in the generated grid, and —
checked directly, not assumed — sits solidly inside a large, contiguous
walkable landmass, not an isolated tile; no `ROAD_PAIRS` entry, same
sea-locked, boat-only precedent Ice Wall Castle set at Milestone 36.

Both locations' tiles and every `ROAD_PAIRS` connection (16 total after
this milestone) were re-verified with the same throwaway 4-directional
BFS approach every map change since the "Road 4-connectivity fix" has
used.

## Sancrist Isle reachability gap (Milestone 88, arrival closed by Milestone 91, departure closed by Milestone 92)

Milestone 88 replaced `GameState::hasBoat` (a permanent, global "cross any
ocean tile" flag) with a scripted, point-to-point sea voyage granted by
talking to a specific NPC — see `docs/ARCHITECTURE.md`'s "Sea travel" and
`docs/ZONE_NOTES.md`'s "Boats" section. Only the Tarsis → Ice Wall Castle
leg was converted (the user asked to scope Sancrist Isle's own voyage as a
separate follow-up). Since Sancrist Isle's reachability above was written
against the old `hasBoat` mechanic (any ocean tile, from anywhere, once
granted), it became unreachable by any scripted route — the boat granted at
Tarsis only carried the player to Ice Wall Castle.

It remained reachable on foot the whole time: a throwaway BFS
against the live `data/overworld.grid`, walking only genuinely-passable
terrain (never true `~`/`!` water), confirmed the entire continent's
land-plus-coastal-shallow-water (`r`) network is one connected component —
Solace, Tarsis, Ice Wall Castle, and Sancrist Isle are all mutually
reachable by foot alone, matching Milestone 87's own BFS finding above.

**Milestone 91** closed the scripted-route gap with a second `BOAT` grant,
`data/zones/ice_wall.txt`'s new "An Ice Barbarian Guide" carrying the
player from Ice Wall Castle onward to Sancrist Isle — see
`docs/ZONE_NOTES.md`'s "Boats" and "Ice Wall Castle" sections. The foot
route above still exists as an alternative; the scripted voyage is a
shortcut, not a passability change.

**But arrival wasn't the whole gap.** Sancrist Isle shipped at Milestone 86
with no talkable NPC at all (a deliberate call at the time — see
`docs/ZONE_NOTES.md`'s "Sancrist Isle" section), which only became a real
problem once Milestone 91 made it a real destination: nothing in the zone
could grant a `BOAT` back out, so a player who sailed in was stuck (the
same foot route out still existed, but with no in-zone signal it did).
**Milestone 92** closed this half with a third `BOAT` grant, `data/zones/
sancrist_isle.txt`'s new "An Embarkation Officer" carrying the player on
to Palanthas — see `docs/ZONE_NOTES.md`'s "Boats" and "Sancrist Isle"
sections, `docs/TIMELINE_NOTES.md`'s "Sancrist Isle" section for the
sourcing.

## Fixing roads that crossed open water (Milestone 87)

Spotted by the user on a rendered view of the map: `ROAD_PAIRS` draws a
`#` road as a straight Bresenham line between two locations' `POS`
values, with no regard for what terrain lies between — and a few of
those straight lines cut across real bodies of water, which Ansalon has
no bridges over in canon.

**Two different metrics matter here, not one.** `src/world/Terrain.cpp`'s
own Milestone-84 comment already documents that the generated grid's `r`
("shallow water") code is "essentially always coastal water... (real
river fords never survived downsampling)" and was *deliberately* made
passable with no boat required — an already-shipped design decision this
milestone doesn't revisit. So the diagnostic that actually matters is:
does a road's straight-line path cross true impassable `~`/`!` tiles (the
ones `GameLoop::tryMoveOverworld` gates behind `GameState::hasBoat`), and
separately, how long a contiguous run of `r` does it cross — a couple of
tiles reads as an ordinary ford (a real road bridging a stream), a couple
dozen reads as a paved highway across a named bay.

A throwaway script replayed every `ROAD_PAIRS` pair's exact line against
the classified (pre-road) grid, and each finding was cross-checked
against cropped, pixel-marked renders of `References/dragonlancemap2.png`
at the real position — not just the coded terrain char, since the same
script's own comments already note this quantization produces "many
near-identical [ocean] buckets from background noise." Of 15 pairs, 11
were already clean (0 true-water tiles, longest `r` run 4 tiles — an
ordinary ford). Four were not:

| Pair | true `~`/`!` | longest `r` run | Resolution |
|---|---|---|---|
| `darken_wood`-`qualinesti` | 1 tile | 1 | Noise pixel at the White-face River's mouth, inside an already-`r` field. Patched. |
| `xak_tsaroth`-`plains_of_dust` | 3 | **27** | Cuts across all of New Bay *and* unrelated land beyond it ("The New Coast," confirmed visually) — also redundant, `xak_tsaroth` already reaches `plains_of_dust` via `haven`→`darken_wood`→`qualinesti`→`pax_tharkas`. **Removed from `ROAD_PAIRS`.** |
| `solace`-`high_clerist_tower` | 3 | 11 | The 109-tile path is genuinely overland the whole way (matches the source map's own drawn inland trail through Kayolin/Southlund) except an 11-tile clip through the Strait of Schallsea near Restglen — comparable in kind, not degree, to the 3 other already-shipped short `r` fords in this list. Also the *only* connection between the Abanasinia/Kharolis cluster and the Solamnia cluster. **Kept**; its 3 true-water tiles patched (2 are edge pixels of the same already-`r` strait field; the third is unrelated inland noise near the Tower itself, in the Vingaard Mountains with no water feature anywhere nearby). |
| `solace`-`silvanesti` | 10 | **25** | Crosses Good Bay/New Bay for 25 contiguous tiles right after leaving Solace, near Xak Tsaroth/New Ports — same category of problem as the `xak_tsaroth` pair. Matches Silvanesti's own DESC ("sealed off... No human is known to have set foot here") and Ice Wall Castle's existing "No road reaches it — only the sea does" precedent. **Removed from `ROAD_PAIRS`**; Silvanesti keeps its `POS` (still walkable to cross-country, just undrawn — see the BFS note below). |

`tools/generate_overworld.py` gained `MANUAL_TERRAIN_OVERRIDES`, a small
`(x, y) -> terrain key` dict applied to the classified grid before
`ROAD_PAIRS` are drawn, patching the four true-water noise tiles above
(three to `river`, matching their already-`r`-coded neighbors; the
Tower-area one to `forest`, matching the majority of its 8 neighbors).
Each entry cites the real feature it corrects, same style as
`INDEX_TO_TERRAIN`'s own comments.

**Why no new port-city location** (the user asked to cross-reference
`References/portcities.txt`): neither real fix above needed one — both
resolve by deleting a bogus/redundant straight-line road, not by adding a
sea lane. Existing modeled ports (Palanthas, Kalaman — both named in
`portcities.txt` — plus Tarsis's `BOAT` grant and Sancrist Isle) remain
the game's only port infrastructure. `portcities.txt` is a geography
reference the user compiled, not itself a verified canon source, so
adding any of its other entries (New Ports, Caergoth, Flotsam, Sanction,
...) as a real location would need the same "check the actual novels/
sourcebooks first" pass Thorbardin/Sancrist Isle got before Milestone 86
— folded into `docs/MILESTONES.md`'s NEXT UP rather than decided here.

**Verification**: re-ran the line-walk check against all 13 remaining
`ROAD_PAIRS` — 0 tiles of `~`/`!` crossed anywhere. A throwaway BFS over
the regenerated grid confirmed every location (including Silvanesti, Ice
Wall Castle, and Sancrist Isle) stays reachable by foot with no boat —
unsurprising and unchanged from before this milestone, since raw terrain
walking (not roads) has always been what determines reachability; roads
only affect travel time and appearance (`docs/ARCHITECTURE.md`). The
"no road reaches it" locations were never a hard gate, just flavor for
"nobody bothers going this way."

## Crossing (Milestone 93) and deleting the Solace-Tower road

The user spotted `("solace", "high_clerist_tower")`'s road crossing open
water north of Solace and asked for it to be deleted, plus a port city
placed there if the sourcing supported it. That road was added at
Milestone 35, before this project had `dragonlancemap2.png` as a source
— its straight line was never actually checked against the real map, and
Milestone 87's water-crossing audit (which caught two other bogus roads)
kept this one on the theory that it was the only link between the
Abanasinia/Kharolis cluster and the Solamnia cluster, patching its 3
true-water noise pixels rather than questioning the road itself.

**Checking the actual map settles it.** Cropping and grid-overlaying
`References/dragonlancemap2.png` around the strait (same method as every
placement since Milestone 85) shows **no drawn road crosses the Strait of
Schallsea anywhere**. The map's own roads hug each shore — Restglen ->
Harrying -> Edgerton on the west, Firstward -> Castle Di Estra -> Port
O'Call on the east — and meet a real, clearly labeled ferry town sitting
on a peninsula in the middle of the strait: **"Crossing,"** with "North
Keep" immediately south of it. Solace's road to the Tower wasn't a rough
approximation of something real; it was invented wholesale, and it
happened to cut across exactly the water the source map goes out of its
way to route around.

This also answers the "port city" half of the ask directly — a real
settlement already exists at the right spot, so nothing needed inventing.
`("solace", "high_clerist_tower")` was removed from `ROAD_PAIRS`
outright (not patched, unlike the Milestone 87 cases, since there's no
short/fordable version of this crossing to keep — the map draws none).
Removing it doesn't strand anything: `data/overworld.grid`'s shallow
coastal water (`r`) around the strait was already deliberately made
foot-passable without a boat (Milestone 87), and a throwaway BFS
confirmed `high_clerist_tower` — and its whole downstream chain (Kalaman,
Palanthas, Godshome, Neraka) — stays foot-reachable from Solace with the
road gone, same as every other location. Regenerating needed no
glacier-patch reapplication; that caveat was retired at Milestone 85.

**"Crossing"'s icon reads at pixel-derived grid `(200, 185)`** — the same
crop-and-grid-overlay method used throughout this file, cross-checked
against the strait's other three legible labels (Castle Di Estra, Port
O'Call, North Keep) in the same crop. `data/overworld.grid` at `(200,185)`
is `^` (hills) — passable, no nudge needed, sitting on the small
contiguous landmass the map itself draws there, confirmed by a direct BFS
from Solace rather than assumed. `REGION Abanasinia` follows `TSR 2143
Player's Guide to the Dragonlance Campaign`'s own framing of Abanasinia as
the land "south of the Straits of Schallsea."

**No `ROAD_PAIRS` entry.** The map's own coastal road through Crossing
continues to North Keep and Staughton, neither of which is a modeled
location in this project — there's nothing sourced to draw a road to
without inventing more content, so Crossing stays a foot-reachable,
road-free stop, same restraint as every other deliberate gap in this
file. Its zone (`data/zones/crossing.txt`) carries no `PRESENCE`/
`TIMELINE_ANCHOR` either — none of the three sourced novels ever mention
this place, only the map does, same treatment Thorbardin's zone already
got for its generic NPC.

## Southern Ergoth (Milestone 95)

Placed the same crop-and-grid-overlay method as every placement since
Milestone 85, cropping a wide region of `References/dragonlancemap2.png`
between the already-placed Ice Wall Castle (`POS 150 305`) and Sancrist
Isle (`POS 55 154`) to find it. Unlike Darken Wood or Ice Wall's own
first placement, this one turned out **directly legible** — the map
labels the whole peninsula "Southern Ergoth" in large diagonal text, with
"Elderwild Wood" (a forest region whose name maps neatly onto the
Kaganesti/"Wilder Elves" the source novel places there) and two
refugee-camp settlements, "Silvamori" and "Qualimori," sitting just south
of it — a striking match to `.research/dwn_full.txt`'s own description of
Silvanesti and Qualinesti refugees sharing this coast (see
`docs/TIMELINE_NOTES.md`).

`POS 91 193` was chosen on the coastal fringe of Elderwild Wood itself
(pixel-derived, then confirmed directly against the live `data/
overworld.grid` rather than assumed): the tile is `%` (forest), with open
water (`r`, shallow coastal water) immediately to its west and solid
forest to its east — a shipwrecked-on-the-shore placement, not a nudge
into the settlements further south (Silvamori/Qualimori are where the
party is being escorted *to*, not the wreck site itself). No `ROAD_PAIRS`
entry, no hand-terrain-patch needed — `SEA_LOCKED`, same as Ice Wall
Castle and Sancrist Isle, and the existing generated grid already
classifies the chosen tile and its neighbors as solid, passable forest,
confirmed by direct inspection rather than assumed.

**Corrects a standing error in `docs/TIMELINE_NOTES.md`'s Ice Wall
section (Milestone 36):** that section says the ship carrying Sturm,
Flint, Tasslehoff, and Laurana toward Sancrist "only sails *past*"
Southern Ergoth and "the party never lands," citing `.research/
dwn_full.txt` lines 5920-5927. That citation is accurate as far as it
goes but stops mid-scene — continuing to read shows a white dragon
attacks that same ship roughly 200 lines later and drives it onto
Southern Ergoth's rocks for real. See `docs/TIMELINE_NOTES.md`'s new
"Southern Ergoth" section for the full, corrected sourcing.

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
