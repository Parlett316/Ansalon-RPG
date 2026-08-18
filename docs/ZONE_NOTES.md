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
`MapRenderer::kMinViewportWidth` × `kMinViewportHeight` (44×16 — the
widest/tallest currently-authored zone, `solace_inn.txt`, is exactly at
this floor). This is a real, current limitation, not just a style
guideline — a bigger zone will render past the edge of what a normal
terminal shows. A dungeon big enough to need its own scrolling camera is
future work.

**Deliberately the *minimum*, not `kViewportWidth`/`kViewportHeight`
themselves.** As of Milestone 33 (adaptive layout — see
`docs/ARCHITECTURE.md`), those two are sized to the player's actual
console window at startup and can be as small as 44×16 on a minimal
supported terminal, or as large as 78×30 on a roomy one. A zone has to
work at the *smallest* size this game will still run at, not whatever a
given run's larger/preferred layout happens to be — so `kMinViewportWidth`/
`kMinViewportHeight` are the real authoring ceiling, not the two
adaptive members.

This is still an upper bound only. As of Milestone 29 (the Caves of
Qud-style wide layout — see `docs/ARCHITECTURE.md`), `drawZoneFrame`
always renders the full `kViewportWidth`×`kViewportHeight` frame
(whatever that run's adaptive size is) regardless of a zone's actual
size: a zone smaller than that (e.g. Solace's 42-column-wide town
square) now wall-pads out to fill it, so the side-by-side log panel's
left edge sits at the same screen column no matter which zone is
showing. There's nothing to author differently for this — it falls out
of `Zone::tileCodeAt`/`poiAt` already returning a wall/no-POI for any
out-of-bounds tile.

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
TALK <char> <dialogue...>               optional, one per talkable POI;
                                        the default/first-meeting line
                                        shown when the player presses 't'
                                        on that tile -- must reference a
                                        char already declared via POI
                                        (see "NPCs: POIs you can talk to"
                                        below)
TALK_AGAIN <char> <dialogue...>         optional -- shown on every talk
                                        after the first, instead of the
                                        generic recognition fallback; same
                                        POI-must-exist rule as TALK
SAY_IF <char> <condition> <dialogue...> optional, zero or more per POI --
                                        a reactive variant of TALK, shown
                                        instead of it on the first talk if
                                        the player's character matches
                                        <condition> (checked in authored
                                        order, first match wins); the char
                                        must already have a POI **and** a
                                        TALK line to react against (see
                                        "NPCs: POIs you can talk to" below)
TOPIC <char> "<label>" <dialogue...>    optional, zero or more per POI --
                                        if any exist, a topic-picker menu
                                        follows the greeting/again line;
                                        same POI-and-TALK-must-exist rule
                                        as SAY_IF
SHOP <char>                             optional, marks a POI as
                                        browsable -- pressing 'p' on that
                                        tile opens the shop screen (see
                                        "Shops: POIs you can buy from"
                                        below); must reference a char
                                        already declared via POI, same
                                        rule as TALK
TIMELINE_ANCHOR <char>                  optional, at most one -- marks a
                                        POI as where present canon
                                        characters are found/talkable
                                        (see "Zone-interior encounters"
                                        below); same POI-must-exist rule
                                        as TALK
TIMELINE_LOCATION <location-id>         optional -- overrides which
                                        timeline location this zone
                                        checks presence for; defaults to
                                        this zone's own filename/catalog
                                        id (see "Zone-interior
                                        encounters" below)
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

**POI tiles are always passable**, regardless of what their glyph would
otherwise suggest. `world::zoneTileFor` only recognizes the 5 base terrain
codes below; `GameLoop::tryMoveZone` checks `Zone::poiAt` *before* falling
back to `zoneTileFor`, so a POI character never gets treated as an unknown
(and therefore impassable) tile. This was a real bug until it was fixed
alongside the nested-zone work below — see `docs/GOTCHAS.md`. If a future
POI genuinely needs to block movement (a statue viewed only from outside,
say), that needs a new, explicit mechanism — don't rely on the glyph alone.

## NPCs: POIs you can talk to

`POI` alone is scenery -- a description shown when the player stands on
that tile, nothing more. Adding a `TALK <char> <dialogue...>` line for
that same char turns it into something the player can press `t` to talk
to (`game::GameLoop::handleTalk`), which shows the dialogue text in its
own dedicated frame (`render::MapRenderer::drawDialogueFrame`) rather than
under the ordinary standing-here description. `TALK` **must** reference a
char that already has a `POI` declaration -- `ZoneLoader` fails fast if
not, the same rule `PORTAL` already follows.

`data/zones/solace_inn.txt`'s `O`/`Y` tiles (Otik Sandeth, Tika Waylan)
are the first example: split out from the original shared `K "The Bar"`
POI (which stays as ambient scenery, unchanged) so each has their own
standable, talkable tile.

**Reactive dialogue and topics, as of Milestone 26.** Zone NPCs can carry
`SAY_IF <char> <condition> <text>` and `TOPIC <char> "<label>" <text>`
lines the same way the 8 Heroes of the Lance do on the overworld (see
`docs/TIMELINE_NOTES.md`) -- the runtime executor (`GameLoop::talkTo`)
was always source-agnostic; what was missing was purely `ZoneLoader`
parsing and `PointOfInterest` fields to hold the parsed values, both
added in Milestone 26. Grammar note: zone files use an explicit `<char>`
on every `SAY_IF`/`TOPIC` line (unlike `data/timeline.txt`'s implicit
"whichever `PRESENCE` block we're inside" convention) because zone
footer lines (`TALK`, `SHOP`, `PORTAL`, ...) are flat and
order-independent, not nested inside a block -- `SAY_IF`/`TOPIC` follow
that same convention rather than `TimelineLoader`'s. A `SAY_IF`/`TOPIC`
line's POI must already have a `TALK` line to react against, or
`ZoneLoader` fails fast at load time -- without a base greeting, reactive
content would parse cleanly but never actually be reachable in play
(`handleTalk` only treats a POI as talkable when its `TALK` dialogue is
non-empty). Five zone NPCs have this content so far: Otik and Tika
(`solace_inn.txt`), the Seeker Guard (`haven.txt`), the Forestmaster
(`darken_wood.txt`), and the Fortress Guard (`pax_tharkas.txt`) -- see
each zone's own section below for what's grounded in.

**"Have I talked to them before" tracking is real, as of Milestone 18,**
via `TALK_AGAIN` as of Milestone 19. `GameLoop::talkTo` shows the
authored `TALK` line the first time, then `TALK_AGAIN` (if authored) or a
short generic recognition line on every later visit (see
`docs/TIMELINE_NOTES.md`'s "Talking to a canon character" for the full
mechanism -- it's shared code, not duplicated per zone/timeline). Every
zone NPC with a `TALK` line has a matching `TALK_AGAIN` as of Milestone
19. Zone NPCs don't have their own stable id the way timeline characters
do (`PointOfInterest` only has a `char` local to its own zone file), so
`GameLoop` synthesizes one as `"<zoneId>:<char>"` (e.g. `"solace_inn:O"`
for Otik) -- no zone-file grammar change needed for this.

## Zone-interior encounters (Milestone 23)

`TIMELINE_ANCHOR <char>` layers a *third* kind of ability onto a POI,
alongside `TALK` and `SHOP`: it marks the tile where any canon character
the timeline schedules here today is found and talkable, the zone-interior
counterpart to standing on a location's overworld tile. Like `SHOP`, it
carries no payload of its own -- who's actually present, and what they
say, comes entirely from `data/timeline.txt` (see `docs/TIMELINE_NOTES.md`
for the full mechanism, condition/topic behavior, and the met-tracking-id
exception this feature needs). At most one anchor per zone; it must
reference an already-declared POI, same rule `TALK`/`SHOP` follow.

By default a zone checks presence for its own filename/catalog id (true
for every top-level zone -- `haven.txt`, `xak_tsaroth.txt`,
`qualinesti.txt` all match a real `LOCATION` id exactly, same as the
existing zone/Location filename-matching rule above). A portal-only zone
like `solace_inn.txt` has no matching `LOCATION` of its own, so it needs
an explicit `TIMELINE_LOCATION solace` line to check the same schedule
the Solace overworld tile does -- otherwise it would default to checking
presence for `"solace_inn"`, a location id nothing in `data/timeline.txt`
will ever match.

Not every zone needs (or should have) an anchor: `data/zones/solace.txt`
(the town square) deliberately has none, since the Heroes' real reunion
scene is inside the Inn specifically -- see `docs/TIMELINE_NOTES.md`.

Six of the eight zones have an anchor as of Milestone 24: `solace_inn.txt`
(`F`, the Great Fireplace), `haven.txt` (`K`, Market Stalls),
`xak_tsaroth.txt` (`C`, the Old Well Shaft), `qualinesti.txt` (`H`, the
Hall of the Sky), `darken_wood.txt` (`F`, A Faded Trail -- its
already-written "trees seem to have rearranged themselves" flavor fits
the wood's canon disorienting magic exactly), and `pax_tharkas.txt` (`M`,
the Tharkadan Mine Entrance -- ties into the fortress's slave-mine plot).
`plains_of_dust.txt` and `tarsis.txt` have none, since neither location
has any timeline schedule content yet to show -- see
`docs/TIMELINE_NOTES.md`'s "Plains of Dust and Tarsis stay out of scope"
note.

## Shops: POIs you can buy from

`SHOP <char>` marks a POI as a place the player can press `p` to browse
and buy from (`game::GameLoop::handleShop`, `character::Equipment`) --
same "TALK layers an ability on top of an existing POI" pattern, just for
buying instead of talking. Unlike `TALK`, `SHOP` carries no payload of its
own (no dialogue text) -- what's for sale is decided entirely by
`character::availableShopItems` from the player's class/steel/gear, not
authored per zone. The shop screen's on-screen title is the POI's own
`name`, not a hardcoded string, so this reads correctly no matter which
POI it's attached to.

As of Milestone 28, three POIs carry `SHOP`: `data/zones/solace.txt`'s
`G "General Store"` (the original), `data/zones/haven.txt`'s
`K "Market Stalls"` (which was already the zone's `TIMELINE_ANCHOR` --
`SHOP` and `TIMELINE_ANCHOR` are independent flags on the same POI, no
conflict), and `data/zones/tarsis.txt`'s `S "An Old Sailor"` (which
already had `TALK`/`TALK_AGAIN`). All three sell from the identical
catalog -- see `docs/CHARACTER_NOTES.md`'s "Equipment" section for the
full catalog, sourcing, and the sell-back mechanic added the same
milestone.

## Portals: a zone can lead into another zone

`PORTAL <char> <target-zone-id>` (a footer line, alongside `POI`/`END`)
marks a tile as a doorway into another zone — e.g. the Inn of the Last
Home's door tile in `data/zones/solace.txt` portals into
`data/zones/solace_inn.txt`. Pressing Enter on a portal tile steps into the
target zone at *its* `ENTRY` point; pressing Enter on that zone's `ENTRY`
tile steps back out to the exact tile the player portaled in from (see
`game::GameState::zoneStack` / `game::GameLoop::handleEnter`), not
necessarily the overworld — leaving a doubly-nested zone would pop back to
its immediate parent zone, not skip straight to the top.

A portal char **must** also have a `POI` declaration (`ZoneLoader` fails
fast if not) — a portal tile still needs a description either way.

Unlike top-level zones, a zone reached *only* via a `PORTAL` has no matching
`LOCATION` in `data/locations.txt` — `ZoneCatalog::loadForWorld` follows
`PORTAL` links (transitively) after its normal location-matching pass
specifically to find these. Practical implication: `solace_inn.txt`'s
filename stem doesn't need to match anything in `locations.txt`, only the
`PORTAL I solace_inn` line in `solace.txt` needs to reference it correctly.

## Why zones don't cost travel time

Walking around inside `data/zones/solace.txt` does not advance
`hoursElapsed` (see `game::GameLoop::tryMoveZone`) — only overworld travel
does. Indoor shuffling a few tiles between the notice board and the inn
door isn't meaningful in-world time; treating it as free keeps the day/hour
counter meaningful for the thing it's actually tracking (progress across
the continent, which matters for the future timeline/encounter engine).

## The Solace zone specifically

`data/zones/solace.txt` is a 40×16 town square: two clusters of vallenwood
trees framing the north edge and a tree-lined south edge with a gap at the
entry road, two named vallenwood trees, a notice board (a small, deliberate
nod to "rumors of war" flavor for the future timeline engine to eventually
make literal), a general store, Flint Fireforge's smithy, and the door to
the Inn of the Last Home (`PORTAL I solace_inn` — see above). Its layout is
original — not a copy of any published map — consistent with the same
non-infringing, inspired-by-canon approach used for location descriptions
elsewhere in this project.

## The Inn of the Last Home specifically

`data/zones/solace_inn.txt` is the Inn's ground floor (44×16, well under the
78×20 viewport cap), reached only through Solace's town square. Canon-wise
this is the single most important interior in the setting — where the
Heroes of the Lance reunite in the opening chapters of *Dragons of Autumn
Twilight* — so it's built as a real walkable room, not just a bigger POI
description: a central vallenwood-trunk block (reusing the `%` base tile,
already impassable, so no new tile type was needed) with the bar (Otik
Sandeth, Tika Waylan — each now their own talkable `O`/`Y` tile right at
the bar, see "NPCs: POIs you can talk to" above), the great fireplace, a
scarred corner table (an atmospheric nod to the setting without quoting
any book scene directly), Otik's kitchen doorway, and a stair up to
private rooms that aren't a modeled zone yet (described as flavor text
only — future work, same pattern as other deliberately-deferred content
in this project).

As of Milestone 26, Otik and Tika each have a `SAY_IF`/`TOPIC` pair
grounded directly in their real DAT dialogue (verified via `pdftotext`
against the actual novel, not written from memory): Otik's wry,
lowered-voice line about theocrats needing their bellies filled same as
anyone (`"Even theocrats need to fill their bellies..."` in the source)
becomes his `TOPIC "Keeping the Peace"`; Tika's real unease about war
rumors and "strange, hooded men" around the High Theocrat becomes her
`TOPIC "Strange Talk"`, deliberately distinct from Otik's own
dismissiveness of the same rumor in the book.

## Haven and Xak Tsaroth

Two more top-level zones, both grounded in `TSR 9130 DL1 Dragons of
Despair.pdf` for tone (freshly written text, not transcribed — same
non-infringing approach as everywhere else in this project):

- `data/zones/haven.txt` — the Lordcity of Haven's plaza: marble towers
  strung along the city's low decorative wall, the Seeker Temple (a
  single exterior POI, not a walkable interior of its own — same
  "described, not modeled" treatment Solace's other unentered buildings
  get), market stalls, and one talkable NPC (a Seeker Guard, wary and a
  little overworked, deliberately not referencing DL1's specific Blue
  Crystal Staff plot). As of Milestone 26 he has a `SAY_IF cleric` line
  (his wariness sharpening toward an actual cleric, on the "hollow
  imitation of real faith" angle) and a `TOPIC "The Highseeker's Line"`
  (grousing texture on his existing "hearing petitioners" line, still no
  new plot facts).
- `data/zones/xak_tsaroth.txt` — a small overlook/plaza slice of the
  sunken ruins, not the full multi-level dungeon DL1 describes (this
  game doesn't model that crawl). Broken towers, a waterfall, the old
  masonry well shaft, and the huge hanging chain DL1 describes dropping
  out of the mist overhead — all scenery, deliberately no talkable NPC
  since the ruins are abandoned.

Both were picked because they already have `PRESENCE` stops on Tanis and
Raistlin's timeline schedule (see `docs/TIMELINE_NOTES.md`), so a player
walking the same route the timeline already tracks now has somewhere
real to step into along the way.

## The rest of the continent: Darken Wood, Qualinesti, Pax Tharkas, Plains of Dust, Tarsis

Every named overworld location now has a walkable interior. The last five,
same freshly-written-not-transcribed treatment as everywhere else:

- `data/zones/darken_wood.txt` — a small clearing within the wood, not
  the whole forest. Grounded in DL1's description of the wood as a place
  that plays tricks on memory and hides old ruins; includes the
  Forestmaster (a unicorn, DL1's actual guardian of the wood) as a
  talkable, cryptic NPC. As of Milestone 26 she has `SAY_IF good`/`SAY_IF
  evil` lines (deliberately no race/class condition — her established
  characterization is about moral worth, not identity) grounded in DAT's
  banquet scene where she judges the party by what they give rather than
  take, plus a `TOPIC "Guardian of the Wood"` describing her role in
  general terms — deliberately not the Sturm/Huma material, which stays
  unique to Sturm's own Milestone 25 topic.
- `data/zones/qualinesti.txt` — Qualinost, the elven capital: four
  silver towers, a golden centerpiece tower, and the Hall of the Sky
  (the central square), all drawn from DL2's description of the city.
  One talkable NPC, an Elven Sentinel, polite but wary — doesn't
  reference DL2's specific Laurana/kidnapping plot.
- `data/zones/pax_tharkas.txt` — the fortress courtyard (not the
  Sla-Mori tunnels or the mines beneath it, which DL2 describes but this
  project doesn't model as their own zone). Deliberately doesn't commit
  to who currently holds the fortress — DL2's "occupied slave-fortress"
  moment is tied to a war-clock this project doesn't track, so the guard
  NPC stays neutral rather than confirming a draconian garrison. As of
  Milestone 26 he has a `SAY_IF fighter` line (soldier-to-soldier
  wariness) and a `TOPIC "A Fortress Between Masters"` — deliberately
  **no** race-based `SAY_IF` (dwarf/elf), since a warm or hostile
  reaction from this specific guard risks implying an occupier identity
  the project has deliberately left open; don't "fix" this gap without
  re-reading why it's here.
- `data/zones/plains_of_dust.txt` — a Plainsfolk camp. Deliberately an
  unnamed tribe's camp, not Que-Shu specifically, since DL3 establishes
  Que-Shu as destroyed by the time of that module's story — using an
  original tribe sidesteps that conflict entirely.
- `data/zones/tarsis.txt` — the old harbor district, stranded since the
  Cataclysm. Not directly covered in DL1-3 (the adventure ends before
  reaching it), so this one leans on general established Dragonlance
  lore and this project's own existing `locations.txt` description
  (beached ship hulls, a dry stone quay, a xenophobic-toward-outsiders
  Old Sailor NPC) rather than a specific module citation.

## Adding a new zone

1. Create `data/zones/<location-id>.txt` matching an existing `LOCATION` id.
2. Design a `GRID` no larger than the current viewport size (see above).
3. Pick an `ENTRY` point, generally near wherever the "road in" would
   logically be.
4. Add `POI` lines for anything worth describing, and make sure every
   non-base-terrain character in `GRID` has one.
5. If a POI is a person the player should be able to talk to, add a
   matching `TALK <char> <dialogue...>` line (see "NPCs: POIs you can talk
   to" above). If a POI should be browsable/buyable, add a matching
   `SHOP <char>` line instead (see "Shops: POIs you can buy from" above).
6. Build and check the load succeeds (a malformed zone file fails fast with
   a clear error at startup, not partway through play).
