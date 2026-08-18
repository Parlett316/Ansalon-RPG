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
SOURCE_IMAGE = REPO_ROOT / "References" / "DragonLance_-_Continent_of_Ansalon_-_Age_of_Despair.jpg"
OUTPUT_GRID = REPO_ROOT / "data" / "overworld.grid"
PREVIEW_IMAGE = REPO_ROOT / "data" / "overworld_preview.png"  # inspection aid only, gitignored
LOCATIONS_FILE = REPO_ROOT / "data" / "locations.txt"

GRID_WIDTH = 480  # tiles across the continent's full bounding box; height follows the image's aspect ratio
NUM_COLORS = 16   # dominant colors to reduce the map to during discovery

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
# data/overworld_preview.png against the reference map. Maps a discovered
# palette index (printed by Phase 1) to a key from TERRAIN_CHARS above.
# Assigned by inspecting data/overworld_preview.png against the reference
# map (see docs/MAP_NOTES.md for the reasoning): 8-12/14/15 are all subtly
# noisy shades of the same dark teal ocean background (JPEG compression
# noise splits what should be one color across several quantization
# buckets); 13 is the Blood Sea's solid maroon fill; 3/6 are two shades of
# the forest fill; 7 is the lighter coastal-shallows blue (thin river lines
# mostly don't survive this downsample resolution, so this is treated as
# fordable shallow water, not literal rivers). Glacier/bog/salt_flat did not
# emerge as distinct colors at NUM_COLORS=16 -- they simply won't appear in
# this generated grid; a higher NUM_COLORS or hand-editing the output would
# be needed to add them.
INDEX_TO_TERRAIN: dict[int, str] = {
    0: "grassland",
    1: "savannah",
    2: "grassland",
    3: "forest",
    4: "hills",
    5: "mountain",
    6: "forest",
    7: "river",
    8: "ocean",
    9: "ocean",
    10: "ocean",
    11: "ocean",
    12: "ocean",
    13: "blood_sea",
    14: "ocean",
    15: "ocean",
}

# Location id pairs to connect with a drawn road ('#') in the generated
# grid. Roads are baked into terrain, not represented as a graph at runtime
# (see docs/ARCHITECTURE.md) -- this list is the one place that road
# connectivity still needs to be specified, since data/locations.txt no
# longer carries CONNECT lines.
ROAD_PAIRS = [
    ("solace", "darken_wood"),
    ("darken_wood", "haven"),
    ("darken_wood", "qualinesti"),
    ("haven", "xak_tsaroth"),
    ("qualinesti", "pax_tharkas"),
    ("pax_tharkas", "plains_of_dust"),
    ("xak_tsaroth", "plains_of_dust"),
    ("plains_of_dust", "tarsis"),
    ("solace", "high_clerist_tower"),
    ("solace", "silvanesti"),
    ("high_clerist_tower", "kalaman"),
]


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
    """Bresenham's line algorithm -- draws a road as a simple straight path
    between two tile coordinates. Roads are a cosmetic/gameplay convenience
    baked into terrain at generation time, not a precise routed path, so a
    straight line (rather than pathfinding around mountains) is an
    intentional simplification -- hand-edit data/overworld.grid afterward if
    a road cuts through terrain it obviously shouldn't.
    """
    dx = abs(x1 - x0)
    dy = -abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    x, y = x0, y0
    while True:
        if 0 <= y < len(grid) and 0 <= x < len(grid[0]):
            grid[y][x] = char
        if x == x1 and y == y1:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x += sx
        if e2 <= dx:
            err += dx
            y += sy


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
