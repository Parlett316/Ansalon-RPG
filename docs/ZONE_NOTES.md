# Zone notes

## What a zone is

A "zone" is a small, hand-authored, walkable ASCII interior for a named
overworld location — a town square, eventually a dungeon or building
interior. Unlike `data/overworld.grid` (generated from the reference map
image — see `docs/MAP_NOTES.md`), zones are drawn by hand, tile by tile, the
same way a MUD room or a small roguelike level would be. Not every location
has one; standing on Solace's overworld tile and pressing Enter works
because `data/zones/solace.txt` exists, and pressing Enter on, say, Tarsis
currently does nothing because `data/zones/tarsis.txt` doesn't exist yet.

## Size constraint: zones must fit the viewport

A zone is rendered in full every frame — no camera, no scrolling (unlike
the overworld). That means a zone's `GRID` block must be no larger than
`MapRenderer::kViewportWidth` × `kViewportHeight` (78×20 as of this
writing). This is a real, current limitation, not just a style guideline —
a bigger zone will render past the edge of what a normal terminal shows.
A dungeon big enough to need its own scrolling camera is future work.

## File grammar (`data/zones/<location-id>.txt`)

The filename stem **must exactly match** the corresponding `LOCATION` id in
`data/locations.txt` — `ZoneCatalog` uses that filename to decide which
overworld location a zone belongs to; there's no separate id field to keep
in sync.

```
NAME <text>              display name, shown as the zone's heading
ENTRY <x> <y>              the tile the player starts on when entering, and
                          the tile that must be stood on to leave again --
                          rendered with a distinct '>' marker
GRID
<rows of the zone, one line each>
ENDGRID
POI <char> "<name>" <description...>    zero or more; declares what a
                                        specific character in the GRID means
END
```

`GRID`/`ENDGRID` is a **literal raw-text block**: every line between them is
taken exactly as written (no trimming, no comments) until a line reads
exactly `ENDGRID` — same idea as `data/overworld.grid`'s format, just
embedded inside an otherwise keyword-per-line file. Every row must be the
same width; `ZoneLoader` fails fast with a `file:line` error if not.

Base terrain characters (defined in `world/ZoneTile.cpp`, not this file):

| char | meaning | passable |
|------|---------|----------|
| `.`  | open ground | yes |
| `%`  | vallenwood tree | no |
| `#`  | wall | no |
| `~`  | water feature | no |
| `+`  | doorway | yes |

Any other character used in `GRID` **must** have a matching `POI` line, or
the file fails to load — this is deliberate: an unrecognized character is
almost certainly a typo, and failing at load time beats silently rendering
it as "unknown" during play (same philosophy as `WorldLoader`'s validation
of `data/locations.txt`).

`POI <char> "<name>" <description...>` follows the same quoted-field
convention as the road descriptions in Milestone 1's `CONNECT` lines: the
name is quoted (may contain spaces), everything after the closing quote is
the free-text description shown when the player stands on that tile.

## Why zones don't cost travel time

Walking around inside `data/zones/solace.txt` does not advance
`hoursElapsed` (see `game::GameLoop::tryMoveZone`) — only overworld travel
does. Indoor shuffling a few tiles between the notice board and the inn
door isn't meaningful in-world time; treating it as free keeps the day/hour
counter meaningful for the thing it's actually tracking (progress across
the continent, which matters for the future timeline/encounter engine).

## The Solace zone specifically

`data/zones/solace.txt` is the first (and currently only) zone, a 40×16
town square: two clusters of vallenwood trees framing the north edge and a
tree-lined south edge with a gap at the entry road, the Inn of the Last
Home, two named vallenwood trees, and a notice board (a small, deliberate
nod to "rumors of war" flavor for the future timeline engine to eventually
make literal). Its layout is original — not a copy of any published map —
consistent with the same non-infringing, inspired-by-canon approach used
for location descriptions elsewhere in this project.

## Adding a new zone

1. Create `data/zones/<location-id>.txt` matching an existing `LOCATION` id.
2. Design a `GRID` no larger than the current viewport size (see above).
3. Pick an `ENTRY` point, generally near wherever the "road in" would
   logically be.
4. Add `POI` lines for anything worth describing, and make sure every
   non-base-terrain character in `GRID` has one.
5. Build and check the load succeeds (a malformed zone file fails fast with
   a clear error at startup, not partway through play).
