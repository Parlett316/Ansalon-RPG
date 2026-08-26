#!/usr/bin/env python3
"""Offline content-pipeline tool: turns the reference Ansalon map image into
a walkable overworld terrain grid for the C++ game to load at runtime.

This script is NOT part of the shipped game -- the C++ code only ever reads
the plain-text data/overworld.grid it produces, never the source image or
Pillow. Run it manually, or re-run it if the reference image or GRID_WIDTH
changes. Re-running OVERWRITES data/overworld.grid, including any hand
corrections made after a previous run -- see docs/MAP_NOTES.md.

Two-phase, iterative by design (see docs/MAP_NOTES.md for why): the source
map is a painted/textured image (mountains drawn as icon clusters, forests
as tree-icon clusters, etc.), not flat color-coded regions, so there's no
reliable automatic way to know "this shade of green means forest, not
grassland" without a human looking at it once.

  Phase 1 (discovery): run with INDEX_TO_TERRAIN empty. Downsamples the
  image, reduces it to NUM_COLORS dominant colors, and writes an inspectable
  preview PNG plus a printed list of each color's coverage. Nothing else
  happens -- data/overworld.grid is NOT written yet.

  Phase 2 (generation): after inspecting the preview and filling in
  INDEX_TO_TERRAIN (mapping each discovered color index to a terrain key),
  re-run. This time it writes data/overworld.grid, with road tiles ('#')
  drawn between the location pairs in ROAD_PAIRS (read from
  data/locations.txt).

Usage:
    pip install pillow
    python tools/generate_overworld.py
"""

from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parent.parent
SOURCE_IMAGE = REPO_ROOT / "References" / "dragonlancemap2.png"
OUTPUT_GRID = REPO_ROOT / "data" / "overworld.grid"
PREVIEW_IMAGE = REPO_ROOT / "data" / "overworld_preview.png"  # inspection aid only, gitignored
LOCATIONS_FILE = REPO_ROOT / "data" / "locations.txt"

GRID_WIDTH = 480  # tiles across the continent's full bounding box; height follows the image's aspect ratio
NUM_COLORS = 32   # dominant colors to reduce the map to during discovery

# Terrain keys -> the single ASCII character baked into data/overworld.grid.
# This table is the shared vocabulary between this script and
# src/world/Terrain.cpp, which attaches glyph/color/passability/time-cost to
# each character -- keep the two in sync if a terrain key is added here.
TERRAIN_CHARS = {
    "ocean": "~",
    "river": "r",
    "blood_sea": "!",
    "glacier": ":",
    "mountain": "A",
    "hills": "^",
    "forest": "%",
    "bog": "*",
    "salt_flat": "_",
    "savannah": '"',
    "grassland": ".",
    "road": "#",
    "unknown": "?",  # should not appear in the final grid; a leftover means INDEX_TO_TERRAIN is incomplete
}

# Filled in by hand after running Phase 1 and inspecting
# data/overworld_preview.png against the reference map (see docs/MAP_NOTES.md
# for the full writeup of this pass, done against the higher-fidelity
# References/dragonlancemap2.png at NUM_COLORS=32). Assigned by index-mask
# inspection (highlighting one bucket at a time in red against the rest,
# rather than judging raw RGB triples by eye):
#   0 is a genuinely distinct warm off-white that traces the Icewall
#     Glacier, a Sancrist/Ergoth-area highland, and the "Northern Wastes"
#     barren patch near "City of Lost Names" -- glacier is recoverable this
#     time (conflated with one barren-wasteland patch that has no game
#     content near it, an accepted minor imprecision).
#   16 is a solid, isolated maroon that traces exactly the Blood Sea's
#     spiral outline and nothing else -- notably, this map does NOT
#     reproduce the old JPEG's mountain-shadow-into-Blood-Sea
#     misclassification documented in docs/MAP_NOTES.md's High Clerist's
#     Tower section.
#   17 is a solid dark brown that traces every major mountain range
#     (Vingaard, Kharolis/Thorbardin, Khalkist/Taman Busuk, the western
#     island chains) cleanly.
#   12/13/14/15 are a lighter teal that traces coastal-shallows fringes
#     plus real inland river-line squiggles -- same "fordable shallow
#     water, not literal rivers" treatment the old map used for its
#     equivalent color.
#   1/2/5 are pale olive-tan -> grassland/savannah; 4/8/9 are darker
#     brown-olive (G<=R) -> hills; 3/6/7/10/11 are green-leaning (G>R)
#     -> forest. This is still an approximation, same as the old map's
#     classification -- painted/textured map art doesn't cleanly separate
#     "forest green" from "plains green" by flat color at any practical
#     NUM_COLORS, index-mask inspection confirmed these buckets are mostly
#     scattered shading/hatch-line noise rather than one coherent region
#     each.
#   18-31 are the remaining dark teal shades (ocean, confirmed by mask
#     inspection to trace the coastline cleanly, including the old map's
#     "many near-identical buckets from background noise" pattern).
# Bog and salt_flat again did not emerge as distinct colors at
# NUM_COLORS=32 -- see docs/MAP_NOTES.md.
INDEX_TO_TERRAIN: dict[int, str] = {
    0: "glacier",
    1: "savannah",
    2: "savannah",
    3: "forest",
    4: "hills",
    5: "savannah",
    6: "forest",
    7: "forest",
    8: "hills",
    9: "hills",
    10: "forest",
    11: "forest",
    12: "river",
    13: "river",
    14: "river",
    15: "river",
    16: "blood_sea",
    17: "mountain",
    18: "ocean",
    19: "ocean",
    20: "ocean",
    21: "ocean",
    22: "ocean",
    23: "ocean",
    24: "ocean",
    25: "ocean",
    26: "ocean",
    27: "ocean",
    28: "ocean",
    29: "ocean",
    30: "ocean",
    31: "ocean",
}

# Location id pairs to connect with a drawn road ('#') in the generated
# grid. Roads are baked into terrain, not represented as a graph at runtime
# (see docs/ARCHITECTURE.md) -- this list is the one place that road
# connectivity still needs to be specified, since data/locations.txt no
# longer carries CONNECT lines.
#
# Milestone 87 removed two entries that used to be here --
# ("xak_tsaroth", "plains_of_dust") and ("solace", "silvanesti") -- because
# their straight-line paths cut across real open bays (New Bay / Good Bay)
# for 25-27 contiguous tiles, not a fordable pixel or two. The former was
# also redundant (xak_tsaroth already reaches plains_of_dust via
# haven -> darken_wood -> qualinesti -> pax_tharkas); the latter matches
# Ice Wall Castle's existing "no road reaches it" precedent and Silvanesti's
# own "sealed off" DESC. See docs/MAP_NOTES.md's "Fixing roads that crossed
# open water" for the full diagnosis (including why the remaining roads'
# short crossings of 'r' shallow water are fine, and MANUAL_TERRAIN_OVERRIDES
# below for the handful of true-'~'-noise pixels patched so no kept road
# touches truly impassable terrain).
#
# A later milestone removed a third entry, ("solace", "high_clerist_tower")
# -- unlike the two above, its straight-line path was never checked against
# the real reference map when it was added (Milestone 35, before this
# project had dragonlancemap2.png as a source). The map draws no road
# across the Strait of Schallsea at all -- only coastal roads meeting a real
# ferry town, "Crossing", added as its own LOCATION instead. See
# docs/MAP_NOTES.md's "Crossing" section.
ROAD_PAIRS = [
    ("solace", "darken_wood"),
    ("darken_wood", "haven"),
    ("darken_wood", "qualinesti"),
    ("haven", "xak_tsaroth"),
    ("qualinesti", "pax_tharkas"),
    ("pax_tharkas", "plains_of_dust"),
    ("plains_of_dust", "tarsis"),
    ("high_clerist_tower", "kalaman"),
    ("high_clerist_tower", "palanthas"),
    ("kalaman", "godshome"),
    ("godshome", "neraka"),
    ("pax_tharkas", "thorbardin"),
    ("neraka", "flotsam"),
    ("flotsam", "port_balifor"),
    ("kalaman", "dargaard_keep"),
]

# Hand-corrected tiles (Milestone 87): pixels the NUM_COLORS=32 quantization
# bucketed into "ocean" even though they aren't part of any real open-sea
# polygon -- confirmed by cropping References/dragonlancemap2.png at each
# point and checking immediate-neighbor terrain majority. Applied to the
# classified grid before ROAD_PAIRS are drawn, so a road stamped over one of
# these tiles never turns truly-impassable terrain into a fake, boat-free
# bridge. See docs/MAP_NOTES.md for the full per-tile writeup.
MANUAL_TERRAIN_OVERRIDES: dict[tuple[int, int], str] = {
    # White-face River's mouth (on the darken_wood-qualinesti road):
    # a 1-tile '~' pixel sitting inside an otherwise-solid 'r' (coastal
    # shallow) field -- the same feature, misclassified by one pixel.
    (187, 210): "river",
    # Strait of Schallsea (on the solace-high_clerist_tower road): two
    # edge pixels of the same already-'r'-coded strait field.
    (191, 187): "river",
    (192, 176): "river",
    # Inland noise near High Clerist's Tower itself (Vingaard Mountains,
    # no water feature anywhere nearby) -- majority of its 8 neighbors
    # are forest, so that's what it's corrected to.
    (194, 98): "forest",
    # Blood Bay's edge (on the neraka-flotsam and flotsam-port_balifor
    # roads, Milestone 96): four tiles at the bay's inner fringe, majority
    # '~'/'r' neighbors -- the same "short ford across real but minor
    # water" treatment as the Strait of Schallsea crossing above, not a
    # misclassification.
    (363, 150): "river",
    (364, 150): "river",
    (364, 151): "river",
    (370, 153): "river",
    # Single-pixel noise in the coastal mountains south of Blood Bay --
    # 6 of 8 neighbors are mountain, so that's what it's corrected to,
    # same rule as the High Clerist's Tower tile above.
    (368, 156): "mountain",
}


def load_location_positions() -> dict[str, tuple[int, int]]:
    """Minimal parser: pulls just LOCATION id + POS x y out of
    data/locations.txt. Not a full implementation of the grammar in
    docs/MAP_NOTES.md -- WorldLoader.cpp is the authoritative parser; this
    only needs enough to place roads.
    """
    positions: dict[str, tuple[int, int]] = {}
    current_id: str | None = None
    for raw_line in LOCATIONS_FILE.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("LOCATION "):
            current_id = line.split(maxsplit=1)[1].strip()
        elif line.startswith("POS ") and current_id is not None:
            _, x_str, y_str = line.split()
            positions[current_id] = (int(x_str), int(y_str))
        elif line == "END":
            current_id = None
    return positions


def draw_line(grid: list[list[str]], x0: int, y0: int, x1: int, y1: int, char: str) -> None:
    """Bresenham's line algorithm, adapted so every two consecutively-drawn
    tiles are orthogonally (4-directionally) adjacent -- never diagonal-only.
    Standard Bresenham can advance x and y in the same step, drawing two
    tiles that only touch corner-to-corner; the game's WASD movement is
    strictly 4-directional (src/render/Console.cpp), so a
    diagonal-only road segment was a real player-facing dead end (see
    docs/MAP_NOTES.md's road-connectivity fix). Whenever a step would move
    both axes at once, the x-move and y-move are taken as two sequential
    stamps (a 1-tile "staircase" corner) instead of one combined jump. The
    error-term math and axis selection are untouched -- only stamp timing
    changed -- so this still draws a straight-ish path, not a pathfinder
    (hand-edit data/overworld.grid afterward if a road cuts through terrain
    it obviously shouldn't).
    """
    dx = abs(x1 - x0)
    dy = -abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    x, y = x0, y0

    def stamp(px: int, py: int) -> None:
        if 0 <= py < len(grid) and 0 <= px < len(grid[0]):
            grid[py][px] = char

    stamp(x, y)
    while x != x1 or y != y1:
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x += sx
            stamp(x, y)
        if e2 <= dx:
            err += dx
            y += sy
            stamp(x, y)


def main() -> None:
    if not SOURCE_IMAGE.exists():
        sys.exit(f"Reference image not found: {SOURCE_IMAGE}")

    img = Image.open(SOURCE_IMAGE).convert("RGB")
    grid_height = round(GRID_WIDTH * img.height / img.width)

    # Box-downsample first: each output pixel becomes the average color of
    # its source region, which smooths individual map icons (tiny tree or
    # mountain glyphs) into an overall regional color -- exactly the biome
    # signal wanted per tile, not per-pixel icon detail.
    small = img.resize((GRID_WIDTH, grid_height), Image.BOX)
    quantized = small.quantize(colors=NUM_COLORS, method=Image.MEDIANCUT)
    palette = quantized.getpalette()[: NUM_COLORS * 3]
    indices = list(quantized.getdata())

    if not INDEX_TO_TERRAIN:
        print(f"Discovered {NUM_COLORS} colors from a {GRID_WIDTH}x{grid_height} downsample:\n")
        counts = [0] * NUM_COLORS
        for idx in indices:
            counts[idx] += 1
        total = len(indices)
        for i in range(NUM_COLORS):
            r, g, b = palette[i * 3 : i * 3 + 3]
            pct = 100 * counts[i] / total
            print(f"  index {i:2d}: rgb({r:3d},{g:3d},{b:3d})  {pct:5.1f}% coverage")

        preview = quantized.convert("RGB").resize((GRID_WIDTH * 4, grid_height * 4), Image.NEAREST)
        PREVIEW_IMAGE.parent.mkdir(parents=True, exist_ok=True)
        preview.save(PREVIEW_IMAGE)
        print(f"\nPreview saved to {PREVIEW_IMAGE}")
        print("Inspect it against the reference map, fill in INDEX_TO_TERRAIN in this script, and re-run.")
        return

    grid: list[list[str]] = []
    for y in range(grid_height):
        row = [TERRAIN_CHARS[INDEX_TO_TERRAIN.get(indices[y * GRID_WIDTH + x], "unknown")] for x in range(GRID_WIDTH)]
        grid.append(row)

    for (ox, oy), terrain_key in MANUAL_TERRAIN_OVERRIDES.items():
        grid[oy][ox] = TERRAIN_CHARS[terrain_key]

    positions = load_location_positions()
    road_char = TERRAIN_CHARS["road"]
    for from_id, to_id in ROAD_PAIRS:
        if from_id not in positions or to_id not in positions:
            print(f"WARNING: skipping road {from_id}-{to_id}, missing POS in {LOCATIONS_FILE.name}")
            continue
        x0, y0 = positions[from_id]
        x1, y1 = positions[to_id]
        draw_line(grid, x0, y0, x1, y1, road_char)

    unknown_char = TERRAIN_CHARS["unknown"]
    unknown_count = sum(row.count(unknown_char) for row in grid)
    if unknown_count:
        print(f"WARNING: {unknown_count} tiles are 'unknown' -- INDEX_TO_TERRAIN is missing an entry.")

    OUTPUT_GRID.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_GRID.write_text("\n".join("".join(row) for row in grid) + "\n", encoding="ascii")
    print(f"Wrote {GRID_WIDTH}x{grid_height} grid to {OUTPUT_GRID}")


if __name__ == "__main__":
    main()
