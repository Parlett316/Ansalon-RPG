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
POS <x> <y>                two non-negative integers -- tile coordinates in the
                          SAME space as overworld.grid (see below for how chosen)
DESC <text>              one-line description, rest of line
END                      closes the block
```

Every field is single-line; `WorldLoader` throws a `file:line: message`
error for anything malformed. There is no `CONNECT`/road field anymore —
travel time now comes from actually walking the terrain
(`world::Terrain::hoursToCross`), not a graph. Roads between locations are
instead baked into `data/overworld.grid` as `#` tiles by
`tools/generate_overworld.py`'s `ROAD_PAIRS` list.

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
