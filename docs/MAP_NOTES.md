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
