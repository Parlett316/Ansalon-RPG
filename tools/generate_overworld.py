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
OUTPUT_REGIONS = REPO_ROOT / "data" / "overworld_regions.grid"
PREVIEW_IMAGE = REPO_ROOT / "data" / "overworld_preview.png"  # inspection aid only, gitignored
LOCATIONS_FILE = REPO_ROOT / "data" / "locations.txt"

GRID_WIDTH = 480  # tiles across the continent's full bounding box; height follows the image's aspect ratio
NUM_COLORS = 32   # dominant colors to reduce the map to during discovery

# How many tiles of 'river' (shallow coastal water) to keep touching real
# land before reclassifying the rest to 'ocean' -- see the distance-transform
# pass in main() and docs/MAP_NOTES.md's "Shallow water pass" section. The
# 12-15 color buckets classified as 'river' traced real coastline plus real
# river-mouth squiggles *and* the interior of large bays/straits the 32-color
# quantization happened to bucket identically (Good Bay, New Bay, Blood Bay,
# the Strait of Schallsea) -- measured on the pre-this-change grid, 'river'
# covered 17,824 tiles (11.6% of the continent), with a distance-to-nearest-
# land histogram trailing off to 66 tiles from any shore. Only a genuine
# coastal fringe should stay 'river'; 2 tiles keeps ~26% (4,708 tiles) of
# the original total and reclassifies the rest (open water, not coastline) to
# 'ocean'. Tune this single constant if a rendered look-over calls for a
# thinner/thicker fringe.
RIVER_COASTAL_FRINGE_TILES = 2

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
#   1/2/5 are pale olive-tan -> grassland (see below); 4/8/9 are darker
#     brown-olive (G<=R) -> hills; 3/6/7/10/11 are green-leaning (G>R)
#     -> forest. This is still an approximation, same as the old map's
#     classification -- painted/textured map art doesn't cleanly separate
#     "forest green" from "plains green" by flat color at any practical
#     NUM_COLORS, index-mask inspection confirmed these buckets are mostly
#     scattered shading/hatch-line noise rather than one coherent region
#     each.
#   1/2/5 were originally all mapped to "savannah" -- corrected to
#     "grassland" after cross-checking two references never previously
#     consulted, `References/TSR 9400 TM3 World Of Krynn Trailmap.pdf` and
#     `References/TSR 8448 The Atlas of the Dragonlance World.pdf`. The
#     Trailmap's own terrain legend (its PDF page 17 -- a fold-out poster
#     with no printed pagination of its own, unlike the Atlas) draws a
#     clean line between "Grassland" (flat, textureless light green) and
#     "Barren"/"Moors"/"Desert" (all distinct colors); its Estwilde panel
#     (PDF page 21) reads as
#     the former. The Atlas's Que-shu page (its own printed p.16, PDF page
#     37 -- this book's roman-numeral front matter makes the two diverge
#     by ~20) independently places Que-shu "in the central plains of
#     Abanasinia." No sampled region
#     reads as true arid savannah -- Plains of Dust, the one candidate,
#     is already hill-dominant in this grid's own classification (index 4/
#     8/9's "hills" bucket), which matches its canon description as a
#     misleadingly-named rugged highland, not flat plains. `savannah`
#     stays defined in TERRAIN_CHARS/Terrain.cpp for a future region that
#     actually earns it -- not deleted, just not force-used without
#     evidence.
#   18-31 are the remaining dark teal shades (ocean, confirmed by mask
#     inspection to trace the coastline cleanly, including the old map's
#     "many near-identical buckets from background noise" pattern).
# Bog and salt_flat again did not emerge as distinct colors at
# NUM_COLORS=32 -- see docs/MAP_NOTES.md.
INDEX_TO_TERRAIN: dict[int, str] = {
    0: "glacier",
    1: "grassland",
    2: "grassland",
    3: "forest",
    4: "hills",
    5: "grassland",
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
    ("solace", "que_shu"),
    ("hopeful_vale", "thorbardin"),
]

# Milestone 131's other two new waypoints, Qualimori and Mount Nevermind,
# deliberately get NO entry here -- their straight-line path to the nearest
# already-placed location (southern_ergoth, sancrist_isle) crosses several
# tiles of real open bay (Harkun Bay / Thalan Bay), not a fordable pixel or
# two, same "don't fake a bridge across real water" rule Milestone 87
# established. Both are reachable on foot from that neighbor's own tile
# (confirmed by BFS over the classified grid) and are modeled as PORTAL-only
# zones with no LOCATION/POS of their own instead -- same pattern Milestone
# 127 used for Foghaven Vale. See docs/MAP_NOTES.md.

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

    # Blood Sea over-classification (found this session, cross-checking
    # `References/TSR 8448 The Atlas of the Dragonlance World.pdf`'s
    # "World View: Post-Cataclysm" map): docs/MAP_NOTES.md's High Clerist's
    # Tower section already flagged mountain-shadow bleeding into the same
    # color bucket as the Blood Sea's maroon fill, but never fixed it. A
    # flood-fill over every '!' tile found exactly one real component
    # (5,129 tiles, bbox x349-462 y55-154) matching the Blood Sea's actual
    # position on the Atlas map -- left untouched -- plus 64 stray tiles in
    # small disconnected clusters, each reclassified below by neighbor-
    # majority vote (same method as the hand-picked overrides above).
    #
    # The larger, more consequential cluster (27 tiles, x203-238 y274-298)
    # sits in the hills south of Tarsis, on the approach to Ice Wall
    # Castle -- a player wandering off-road there was hitting nonsense
    # "Blood Sea" tiles nowhere near the real one. All 27 resolve to hills.
    (203, 274): "hills",
    (203, 275): "hills",
    (204, 276): "hills",
    (205, 277): "hills",
    (206, 278): "hills",
    (207, 279): "hills",
    (212, 292): "hills",
    (213, 293): "hills",
    (214, 294): "hills",
    (215, 295): "hills",
    (219, 297): "hills",
    (221, 297): "hills",
    (222, 297): "hills",
    (225, 296): "hills",
    (226, 296): "hills",
    (227, 296): "hills",
    (228, 296): "hills",
    (229, 296): "hills",
    (230, 295): "hills",
    (230, 296): "hills",
    (231, 293): "hills",
    (231, 296): "hills",
    (232, 290): "hills",
    (233, 297): "hills",
    (235, 298): "hills",
    (236, 298): "hills",
    (238, 298): "hills",

    # The remaining 37 stray tiles cluster around the Blood Sea's own
    # edges and the small Mithas/Kothas island chain east of it -- far
    # from any current game content, but real noise all the same.
    (362, 120): "river",
    (362, 121): "river",
    (393, 83): "hills",
    (398, 123): "hills",
    (404, 98): "hills",
    (409, 123): "hills",
    (410, 123): "hills",
    (412, 82): "hills",
    (413, 82): "hills",
    (414, 82): "hills",
    (416, 79): "mountain",
    (416, 80): "mountain",
    (417, 81): "hills",
    (424, 92): "hills",
    (424, 93): "hills",
    (425, 92): "hills",
    (425, 93): "hills",
    (443, 81): "mountain",
    (444, 81): "mountain",
    (445, 81): "mountain",
    (446, 25): "hills",
    (446, 27): "forest",
    (446, 81): "mountain",
    (447, 24): "hills",
    (447, 28): "forest",
    (447, 82): "mountain",
    (447, 109): "mountain",
    (447, 110): "mountain",
    (447, 111): "mountain",
    (451, 27): "mountain",
    (451, 30): "forest",
    (456, 33): "forest",
    (457, 29): "mountain",
    (457, 34): "hills",
    (458, 26): "mountain",
    (458, 27): "mountain",
    (459, 25): "forest",

    # Xak Tsaroth approach bog (found this session, sourced to the Atlas's
    # "The Cursed Lands" page, its own printed p.19 (PDF page 40)): the
    # companions' route from Haven
    # crosses "a swamp -- the Cursed Lands of Newsea" to reach the ruins,
    # and that page's own map legend lists "Marsh" as a distinct category.
    # This backs up the "swampy lowlands" flavor text this project's docs
    # already assert for Xak Tsaroth (POS 202,203) but never actually
    # placed as grid terrain. Kept deliberately tight -- only the open
    # ground within a few tiles of the ruins, not a guessed regional
    # extent -- and only tiles that were plain open ground (not the
    # existing forest/hills/river tiles nearby, which likely represent the
    # real tree-cover and water features the Atlas map shows mixed into
    # the same swamp).
    (199, 200): "bog",
    (199, 202): "bog",
    (199, 203): "bog",
    (199, 204): "bog",
    (200, 200): "bog",
    (200, 201): "bog",
    (200, 202): "bog",
    (200, 203): "bog",
    (201, 202): "bog",
    (201, 203): "bog",
    (201, 206): "bog",
    (202, 200): "bog",
    (202, 201): "bog",
    (202, 202): "bog",
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


def shrink_river_to_coastal_fringe(grid: list[list[str]]) -> None:
    """Reclassifies 'river' (shallow coastal water) tiles farther than
    RIVER_COASTAL_FRINGE_TILES from any land tile to 'ocean', in place.

    A multi-source BFS (4-directional, same connectivity rule the game's own
    movement and every reachability check in docs/MAP_NOTES.md uses) computes
    each tile's distance to the nearest genuinely-land tile -- 'ocean',
    'river', and 'blood_sea' don't count as land, everything else does
    (including 'road', which is always physically on the ground). See
    RIVER_COASTAL_FRINGE_TILES's own comment for why this pass exists and how
    the threshold was chosen.
    """
    height = len(grid)
    width = len(grid[0])
    water_chars = {TERRAIN_CHARS["ocean"], TERRAIN_CHARS["river"], TERRAIN_CHARS["blood_sea"]}
    river_char = TERRAIN_CHARS["river"]
    ocean_char = TERRAIN_CHARS["ocean"]

    from collections import deque

    dist = [[-1] * width for _ in range(height)]
    queue: deque[tuple[int, int]] = deque()
    for y in range(height):
        for x in range(width):
            if grid[y][x] not in water_chars:
                dist[y][x] = 0
                queue.append((x, y))

    while queue:
        x, y = queue.popleft()
        d = dist[y][x]
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < width and 0 <= ny < height and dist[ny][nx] == -1:
                dist[ny][nx] = d + 1
                queue.append((nx, ny))

    for y in range(height):
        for x in range(width):
            if grid[y][x] == river_char and dist[y][x] > RIVER_COASTAL_FRINGE_TILES:
                grid[y][x] = ocean_char


def smooth_terrain(grid: list[list[str]], self_weight: int = 2) -> None:
    """Denoises the raw per-pixel classification in place: every tile becomes
    the plurality of itself (counted self_weight times) plus its 8 immediate
    neighbors. Fixes the salt-and-pepper misclassification the box-downsample
    in main() doesn't fully catch -- confirmed against the real grid this was
    genuine noise, not deliberate small features, and prototyped at self_
    weight=2/one pass against the reference map before landing on these
    defaults (see docs/MAP_NOTES.md).

    Must run on the *raw* classified grid, before MANUAL_TERRAIN_OVERRIDES is
    applied -- those ~90 entries are independently pixel-verified fixes for
    specific known misclassifications, and would just get smoothed back to
    the wrong value if this ran after. 'road' is skipped both as a target and
    as a vote: defensive, since roads aren't drawn onto the grid until after
    this and MANUAL_TERRAIN_OVERRIDES both run, but keeps this function safe
    if that ordering ever changes.
    """
    road_char = TERRAIN_CHARS["road"]
    height = len(grid)
    width = len(grid[0])
    source = [row[:] for row in grid]

    for y in range(height):
        for x in range(width):
            if source[y][x] == road_char:
                continue
            counts: dict[str, int] = {source[y][x]: self_weight}
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    if dx == 0 and dy == 0:
                        continue
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < width and 0 <= ny < height:
                        nc = source[ny][nx]
                        if nc == road_char:
                            continue
                        counts[nc] = counts.get(nc, 0) + 1
            grid[y][x] = max(counts, key=counts.get)


def fix_spurious_water_islands(grid: list[list[str]]) -> None:
    """Reclassifies small, landlocked 'ocean'/'blood_sea' pockets in place,
    fixing a real player-facing bug found this session: a player walking the
    Qualinesti forest near Bianost got blocked mid-forest by "ocean" tiles
    that the reference map draws as plain trees.

    Root cause: Qualinesti and Silvanesti are drawn with unusually dark,
    densely-hatched tree-icon shading -- dark enough that a meaningful
    fraction of it quantizes into the same 32-color-palette indices already
    assigned to real mountain (index 17) and real ocean (indices 18-31)
    elsewhere on the map (see the INDEX_TO_TERRAIN comment above). The result
    is both elven-forest landmasses riddled with fake "ocean"/"mountain"
    tiles. `ocean`/`blood_sea` is impassable (src/world/Terrain.cpp) --
    that's what actually blocks movement; `mountain` is passable (just
    slower, riskier) so it doesn't reproduce the bug and is deliberately left
    alone here (see docs/MAP_NOTES.md for why a mountain-side fix needs a
    fuzzier judgment call and isn't done in this pass).

    A 4-connected flood fill over every ocean/blood_sea tile map-wide finds
    one dominant real component (the world ocean/Blood Sea, tens of
    thousands of tiles) and many far smaller ones (the largest non-dominant
    component found this session was 239 tiles) -- confirmed by visual
    inspection of the largest several dozen that every one sits inside a
    forest, never a real lake. Every component except the dominant one is
    reclassified here by a majority vote over its bordering neighbor tiles
    (the same neighbor-majority-vote method already used by hand for the
    Blood Sea's 64 stray tiles at Milestone 128 -- see docs/MAP_NOTES.md --
    just automated and applied map-wide instead of to one feature).

    The vote deliberately excludes `mountain` as a valid answer, not just
    `ocean`/`blood_sea` -- verified empirically that without this exclusion,
    pockets sitting next to Qualinesti's *also*-miscoded mountain patches
    would "fix" into fake mountain instead of forest, self-reinforcing the
    same bug instead of fixing it. `river` is excluded too, for a more
    basic reason: it's shallow coastal water, impassable exactly like
    ocean/blood_sea (src/world/Terrain.cpp) -- voting a pocket into 'river'
    would swap one impassable terrain for another and leave the reported
    bug just as reproducible, whether or not a real river happens to be
    nearby. A small number of components end up with no valid neighbor at
    all (every neighbor is itself ocean/blood_sea/mountain/river) -- these
    sit deep inside real mountain ranges or real coastline with no reliable
    land signal nearby, so they're left unchanged rather than guessed at;
    found this session to be a negligible residual (max 23 tiles per
    pocket).

    Must run after smooth_terrain() (denoise first) and before
    MANUAL_TERRAIN_OVERRIDES (those hand-verified exceptions still get the
    final say, same ordering rationale every other pass here already
    documents) and before shrink_river_to_coastal_fringe()/ROAD_PAIRS are
    drawn.
    """
    from collections import deque

    water_chars = {TERRAIN_CHARS["ocean"], TERRAIN_CHARS["blood_sea"]}
    excluded_from_vote = water_chars | {TERRAIN_CHARS["mountain"], TERRAIN_CHARS["river"]}
    height = len(grid)
    width = len(grid[0])

    comp_id = [[-1] * width for _ in range(height)]
    components: list[list[tuple[int, int]]] = []
    for y in range(height):
        for x in range(width):
            if grid[y][x] not in water_chars or comp_id[y][x] != -1:
                continue
            cid = len(components)
            cells: list[tuple[int, int]] = []
            queue: deque[tuple[int, int]] = deque([(x, y)])
            comp_id[y][x] = cid
            while queue:
                cx, cy = queue.popleft()
                cells.append((cx, cy))
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = cx + dx, cy + dy
                    if (0 <= nx < width and 0 <= ny < height and grid[ny][nx] in water_chars
                            and comp_id[ny][nx] == -1):
                        comp_id[ny][nx] = cid
                        queue.append((nx, ny))
            components.append(cells)

    if not components:
        return
    dominant_cid = max(range(len(components)), key=lambda i: len(components[i]))

    reclassified_tiles = 0
    reclassified_pockets = 0
    for cid, cells in enumerate(components):
        if cid == dominant_cid:
            continue
        cell_set = set(cells)
        votes: dict[str, int] = {}
        for (x, y) in cells:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)):
                nx, ny = x + dx, y + dy
                if not (0 <= nx < width and 0 <= ny < height) or (nx, ny) in cell_set:
                    continue
                neighbor = grid[ny][nx]
                if neighbor in excluded_from_vote:
                    continue
                votes[neighbor] = votes.get(neighbor, 0) + 1
        if not votes:
            continue  # no reliable land signal nearby -- leave this tiny pocket as-is
        winner = max(votes, key=votes.get)
        for (x, y) in cells:
            grid[y][x] = winner
        reclassified_tiles += len(cells)
        reclassified_pockets += 1

    print(f"fix_spurious_water_islands: reclassified {reclassified_tiles} tiles across "
          f"{reclassified_pockets} spurious ocean/Blood-Sea pockets "
          f"({len(components) - 1 - reclassified_pockets} left untouched, no reliable neighbor)")


def compute_region_layer(grid: list[list[str]], radius: int = 8) -> list[list[str]]:
    """A second, much coarser classification of the same finished grid, used
    only to decide where MapRenderer draws a region-boundary highlight --
    never to pick a tile's own displayed glyph/color (that's still raw
    `grid`, unchanged). Written to its own file, data/overworld_regions.grid.

    A first attempt at this highlight compared each tile directly against
    its raw neighbors and lit up 46.5% of all land tiles -- even after
    smooth_terrain, 27% of horizontal runs in the real grid are still only 1
    tile wide (49% are <=2), so there's no large uniform "region" to trace at
    raw-tile resolution. This function exists to produce that missing
    coarser signal: every tile becomes the plurality of terrain codes across
    its (2*radius+1)^2 window, deliberately *unweighted* (unlike
    smooth_terrain's self-weighted denoise) -- the point here is pure
    neighborhood consensus, ignoring the tile's own possibly-noisy raw value
    entirely, not a corrected per-cell classification. At radius=8 (17x17),
    real-grid border density (adjacent tiles with a differing region label)
    drops to ~11%, and a Pillow mockup rendered directly from this grid
    traces real mountain/forest/grassland boundaries cleanly -- see
    docs/MAP_NOTES.md.

    Must run dead last in main(), after smooth_terrain, MANUAL_TERRAIN_
    OVERRIDES, shrink_river_to_coastal_fringe, and ROAD_PAIRS drawing -- on
    the truly finished grid, same ordering rationale those steps already
    document. Road ('#') is skipped both as a vote target and as a
    contributor, the same defensive pattern smooth_terrain uses -- this
    makes a road tile inherit whatever region it cuts through, so the
    render-time border check lights up a road automatically wherever it
    crosses a real region boundary, with no special-casing needed anywhere.
    """
    road_char = TERRAIN_CHARS["road"]
    height = len(grid)
    width = len(grid[0])

    region = [[""] * width for _ in range(height)]
    for y in range(height):
        y0, y1 = max(0, y - radius), min(height, y + radius + 1)
        for x in range(width):
            x0, x1 = max(0, x - radius), min(width, x + radius + 1)
            counts: dict[str, int] = {}
            for yy in range(y0, y1):
                row = grid[yy]
                for xx in range(x0, x1):
                    c = row[xx]
                    if c == road_char:
                        continue
                    counts[c] = counts.get(c, 0) + 1
            # Falls back to the tile's own raw code only if every tile in its
            # window is road -- not reachable with this map's actual road
            # density (roads are 1 tile wide), kept purely defensive.
            region[y][x] = max(counts, key=counts.get) if counts else grid[y][x]
    return region


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

    smooth_terrain(grid)
    fix_spurious_water_islands(grid)

    for (ox, oy), terrain_key in MANUAL_TERRAIN_OVERRIDES.items():
        grid[oy][ox] = TERRAIN_CHARS[terrain_key]

    shrink_river_to_coastal_fringe(grid)

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

    regions = compute_region_layer(grid)
    OUTPUT_REGIONS.write_text("\n".join("".join(row) for row in regions) + "\n", encoding="ascii")
    print(f"Wrote {GRID_WIDTH}x{grid_height} region layer to {OUTPUT_REGIONS}")


if __name__ == "__main__":
    main()
