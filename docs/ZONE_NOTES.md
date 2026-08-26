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
TALK_AFTER <char> <dialogue...>          optional -- shown instead of
                                        TALK/TALK_AGAIN the first time this
                                        POI is talked to once every canon
                                        character scheduled at this zone's
                                        effective timeline location has
                                        fully moved on (see "Aftermath
                                        dialogue" below); same "must already
                                        have a TALK line" rule as SAY_IF
TALK_BEFORE <char> <dialogue...>         optional -- shown instead of
                                        TALK/TALK_AGAIN on every talk while
                                        every canon character scheduled at
                                        this zone's effective timeline
                                        location hasn't arrived yet (see
                                        "Anticipation dialogue" below); same
                                        "must already have a TALK line" rule
                                        as TALK_AFTER
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
SUBJECT <char> <keywords> <dialogue...> optional, zero or more per POI --
                                        a free-text-askable subject (see
                                        "Ask about anything" below);
                                        <keywords> is one whitespace-free,
                                        comma-separated token, matched
                                        case-insensitively against words
                                        the player types; same
                                        POI-and-TALK-must-exist rule as
                                        SAY_IF/TOPIC
SUBJECT_UNKNOWN <char> <dialogue...>    optional, at most one per POI --
                                        shown when a typed subject matches
                                        no SUBJECT above; omitting this
                                        falls back to a generic engine line
SHOP <char>                             optional, marks a POI as
                                        browsable -- pressing 'p' on that
                                        tile opens the shop screen (see
                                        "Shops: POIs you can buy from"
                                        below); must reference a char
                                        already declared via POI, same
                                        rule as TALK
BOAT <char> <destination-location-id>   optional, marks a POI as granting a
  <hours>                                one-time scripted sea voyage to
                                        <destination-location-id>, <hours>
                                        in-game hours long, the first time
                                        it's talked to (see "Boats: POIs
                                        that grant sea travel" below);
                                        same "must already have a POI
                                        **and** a TALK line" rule as
                                        SAY_IF/TOPIC. The destination id
                                        is validated against the loaded
                                        World in ZoneCatalog::loadForWorld,
                                        not here (ZoneLoader can't see
                                        World) -- same deferred-validation
                                        shape QUEST uses for QuestCatalog.
GRANTS_ITEM <char> <item-id>            optional, marks a POI as granting
  <display-name...>                      a character::ItemKind::QuestItem
                                        the first time it's talked to (see
                                        "Quest items: POIs that grant a
                                        DELIVER object" below); same "must
                                        already have a POI **and** a TALK
                                        line" rule as BOAT
BED <char>                              optional, marks a POI as a bed --
                                        pressing 'z' on that tile fully
                                        heals and advances 8 hours (see
                                        "Beds: POIs for complete bed-rest"
                                        below); must reference a char
                                        already declared via POI, same
                                        rule as SHOP (no TALK required)
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
QUEST <char> <quest-id>                 optional, at most one per POI --
                                        marks a POI as a quest-giver (see
                                        "Quests: POIs that offer them"
                                        below); same "must already have a
                                        POI **and** a TALK line" rule as
                                        BOAT
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

**As of Milestone 43, having a `TALK` line changes what happens on
arrival too, not just what `t` does.** A scenery POI (no `TALK` line)
still prints its full `description` the moment the player steps onto it,
same as always -- there's no other way to reveal it. An NPC POI (`TALK`
present, i.e. `dialogue` non-empty) instead only announces `"<name> is
here."`; its full `description` now shows via Look (`;`,
`game::GameLoop::lookZone`/`pickAndLook`) instead of auto-printing. If
more than one NPC/talkable thing is present at once, Look asks which one
via the same picker `t` already uses for "Talk to whom?" -- see
`docs/ARCHITECTURE.md`'s "Frameless overworld/zone layout + NPC 'Look'".

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

## Ask about anything: free-text subjects

`SUBJECT`/`SUBJECT_UNKNOWN` (see grammar above) let the player type any
subject at all to ask a talkable POI about, not just pick from the curated
`TOPIC` menu -- the zone-native counterpart to `data/timeline.txt`'s own
`SUBJECT`/`SUBJECT_UNKNOWN` lines. `game::GameLoop::talkTo` is the single,
source-agnostic executor for both (same "always was source-agnostic; what
was missing was purely loader parsing and a field to hold the parsed
value" precedent `TOPIC`/`SAY_IF` themselves already established at
Milestone 26) -- see `docs/TIMELINE_NOTES.md`'s "Ask about anything" for
the full mechanism (keyword matching, the free-text input frame, and why
it deliberately avoids mixing `std::cin` into `GameLoop`'s `_getch()`-based
input loop).

This mechanic's proof of concept originally shipped entirely on the
timeline side (Raistlin's `PRESENCE solace 0 1` window, Milestone 71),
same "one location first" restraint every reactive-dialogue feature in
this project has followed. Milestone 79 widened it to all 21 talkable
zone-native NPCs at once (Otik, Tika, every zone-native guard/knight/
warder, etc.), each with 2 `SUBJECT` entries and a `SUBJECT_UNKNOWN`,
generalized from that NPC's own existing `TALK`/`TOPIC`/`SAY_IF`/POI-
description text -- no engine change, pure content, same technique
Milestone 76 used for the Heroes. Two POIs with a `TALK` line were
deliberately left out: Solace's Notice Board (`solace.txt:B`) and Pax
Tharkas's Ore Cart (`pax_tharkas.txt:O`) are objects, not people --
"ask about anything" doesn't fit an object the same way it fits a
conversation, so neither one got a `SUBJECT` pool. Godshome has no
talkable NPC at all (deliberately sparse, Milestone 45) and stays out of
scope for the same reason it's out of scope for `TALK_AFTER`.

Milestone 82 followed up on a real gap this left: several NPCs' own
`TOPIC` entries already covered material (the Tower siege, the Golden
General, Cyan Bloodbane, the Knights' Trial, and similar) that had no
matching `SUBJECT` keyword, so asking about it directly fell through to
the generic `SUBJECT_UNKNOWN` brush-off even though the NPC clearly had an
opinion. 11 of the 21 got 1-2 more `SUBJECT` entries each, every one
paraphrased from that same POI's own already-shipped `TOPIC` (never
`TALK_AFTER` -- see the day-range-gate note below) or, for Haven's Seeker
Guard, `TALK_BEFORE`. The other 10 were deliberately left untouched: High
Clerist's Tower's Sword/Circle/Rose Knights have no `TOPIC` at all --
their terseness is a characterization choice ("keeps to himself for now"),
not a content gap -- and the rest (Plains of Dust's Rider, Qualinesti's
Sentinel, Xak Tsaroth's Scavenger, both Tarsis NPCs, Solace's blacksmith)
had no unaddressed `TOPIC` material to draw from. The same pass fixed two
latent parsing bugs found while re-reading every `SUBJECT` line in scope:
`neraka.txt`'s `temple,dark queen` and `xak_tsaroth.txt`'s
`ruins,city,xak tsaroth` each had a space inside the keyword-list token,
which `ZoneLoader`'s `iss >> keywordList` read (whitespace-delimited)
silently truncates -- the word after the space leaked into the *displayed
dialogue text* instead of becoming a matchable keyword. Both are now
comma-joined (`temple,dark,queen` / `ruins,city,xak,tsaroth`).

**Zone-file `SUBJECT` has no day-range gate**, unlike `data/timeline.txt`'s
character-level `SUBJECT_WHEN` (Milestone 72) -- it's reachable on any day
the player visits. Milestone 79's content deliberately stays within what
each NPC's own un-gated `TALK`/`TOPIC` material already treats as always
true (their identity, opinions, and immediate surroundings) rather than
pulling from that same POI's `TALK_AFTER` block, which exists specifically
*because* it's gated to fire only once the relevant canon-character window
has closed -- reusing that material in an ungated `SUBJECT` would leak
retrospective/spoiler content about the Heroes before it's actually
happened in-game.

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

## Boats: POIs that grant a scripted sea voyage (Milestone 36, reworked Milestone 88, extended Milestone 91, decline option + third leg Milestone 92)

`BOAT <char> <destination-location-id> <hours>` marks a POI whose `TALK`
interaction offers the player a Board/"Not yet" choice (a `drawPickerFrame`
picker, added Milestone 92 -- mirrors `offerOrTurnInQuest`'s Accept/Decline
picker) the first time it's ever accepted moving the player straight to
`<destination-location-id>`'s overworld `POS`, exits back to the
overworld, and advances `hoursElapsed` by `<hours>` (see
`docs/ARCHITECTURE.md`'s "Sea travel"). Same "layers an ability on top of
an existing POI" pattern as `SHOP`/`TIMELINE_ANCHOR`, but tied to
*talking* rather than a separate key -- there's no dedicated "board the
ship" key, reusing `t` and the existing dialogue/log-message flow keeps
this minimal. Unlike `SHOP`, `BOAT` requires its POI to already have a
`TALK` line (same rule `SAY_IF`/`TOPIC` follow): without one, the POI
would never actually be a talk candidate, and the grant would be
unreachable dead content. Unlike `SHOP`/`BED` (plain bools on
`PointOfInterest`), the destination id is an id payload needing
cross-file validation, so it's modelled like `PORTAL`/`QUEST` instead --
stored in its own `Zone`-level map, validated once a `World` exists (see
`ZoneCatalog::loadForWorld`), not by `ZoneLoader` alone.

**Declining** (Milestone 92) leaves the greeting's ordinary topics/
`SUBJECT` picker still reachable instead of ending the conversation --
tracked via `GameState::voyagesTaken`, not `metCharacters` (see
`docs/ARCHITECTURE.md`'s "Sea travel" for why those can't share one flag).
The offer comes back on every later visit until actually boarded.

Four POIs carry `BOAT`, four legs of the same route (Tarsis -> Ice Wall ->
Southern Ergoth -> Sancrist -> Palanthas):

- `data/zones/tarsis.txt`'s `R "A Knight's Runner"`, granting
  `BOAT R ice_wall 48`. Deliberately not Tarsis's existing `S "An Old
  Sailor"` -- his established `TALK`/`TALK_AGAIN` lines say outright that
  the sea "isn't coming back," and Tarsis's harbor is canonically dead (see
  the zone's own section below); routing the boat grant through him would
  contradict flavor already written in Milestone 28. The Runner instead
  represents passage arranged by Derek Crownguard's knights (see
  `docs/TIMELINE_NOTES.md`'s "Ice Wall" section for the citation), keeping
  the Old Sailor's characterization untouched.
- `data/zones/ice_wall.txt`'s `B "An Ice Barbarian Guide"` (Milestone 91,
  repointed Milestone 95), granting `BOAT B southern_ergoth 48` -- was
  `BOAT B sancrist_isle 48` until Milestone 95 found the source text has
  this exact crossing go wrong (a white dragon wrecks the ship on Southern
  Ergoth's coast, not a clean arrival at Sancrist -- see
  `docs/TIMELINE_NOTES.md`'s "Southern Ergoth" section). Repointed rather
  than replaced: same POI char, same `voyagesTaken` key
  (`"ice_wall:B"`), so a save that already recorded this voyage as taken
  keeps reading it that way. Deliberately not the zone's existing `K "A
  Young Knight"` -- he's already that zone's `frostreaver_salvage`
  quest-giver and shouldn't also whisk the player away on first talk (see
  "Quests: POIs that offer them" below). The Guide represents the Ice
  Barbarians who the source text credits with helping the historical party
  escape the castle's collapse -- a different, locally-grounded hook from
  the Runner's Solamnic-knights framing, appropriate since this is a
  different coast and a different culture.
- `data/zones/southern_ergoth.txt`'s `S "A Silvanesti Sentry"` (Milestone
  95), granting `BOAT S sancrist_isle 60`. Closes the new leg Milestone
  95's repoint opened -- represents the Silvanesti-elf-escorted journey
  onward to the Qualinesti refugee elders and then, eventually, Sancrist
  itself (see `docs/TIMELINE_NOTES.md`'s "Southern Ergoth" section). No
  explicit on-page duration exists for this leg, so 60 hours is
  invented-for-pacing, longer than the original 48-hour estimate since it
  now represents an overland escort plus a fresh sea crossing.
- `data/zones/sancrist_isle.txt`'s `E "An Embarkation Officer"` (Milestone
  92), granting `BOAT E palanthas 96`. Closes the reachability gap
  Milestone 91 left open -- see "Sancrist Isle" below. Represents the
  Knights' own army muster for the Palanthas crossing (source-grounded, see
  `docs/TIMELINE_NOTES.md`'s "Sancrist Isle" section), not a generic guard.

**Milestone 36 originally modeled this as a permanent `GameState::hasBoat`
flag** that let the player cross any ocean tile anywhere, forever, once
granted -- Milestone 88 replaced it with the scripted jump described above
after the user found it let the PC "just sail around" the whole continent
regardless of where they stood, which the source material (one specific
ship's route) never supported. Sancrist Isle relied on that same global
flag and had no `BOAT` voyage of its own until Milestone 91 -- see
`docs/MAP_NOTES.md`'s "Sancrist Isle reachability gap."

## Quest items: POIs that grant a DELIVER object (the DELIVER milestone)

`GRANTS_ITEM <char> <item-id> <display-name...>` marks a POI whose `TALK`
interaction, the first time it happens, adds a `character::ItemKind::
QuestItem` to the player's inventory -- the exact same "layers an ability
on top of an existing POI, tied to talking" pattern `BOAT` established
(same "must already have a TALK line" rule, same "granted once, never
again" guard), just handing over a carried item instead of flipping a
flag. `<item-id>` is a stable id matched against a quest's `DELIVER
<item-id> <count> <label>` objective (`quest::Objective::targetId`, see
`docs/QUEST_NOTES.md`'s "DELIVER"); `<display-name...>` is free text shown
in the inventory screen and journal (`character::InventoryItem::
questItemName`). The grant and the quest that wants the item are two
independently-existing pieces of state -- deliberately not a special link
between them, same "objective is a query, not a counter" shape as every
other objective kind.

As of the DELIVER milestone, exactly one POI carries `GRANTS_ITEM`:
`data/zones/pax_tharkas.txt`'s `O "An Ore Cart"`, granting
`raw_tharkadan_ore` -- see `docs/QUEST_NOTES.md`'s "Shipped:
ore_for_the_forge" and this file's own Pax Tharkas section below for why
it's a new POI rather than added to the existing Mine Entrance.

## Beds: POIs for complete bed-rest (Milestone 41)

`BED <char>` marks a POI as a bed the player can press `z` at to fully heal
and advance 8 in-game hours (`game::GameLoop::handleBedRest`,
`world::PointOfInterest::isBed`) -- the Inn-gated healing tier
`docs/CHARACTER_NOTES.md`'s "Rest and spell memorization" section deferred
at Milestone 40. Same "layers an ability on top of an existing POI" pattern
as `SHOP`, with the same no-`TALK`-prerequisite rule (unlike `BOAT`, which
requires one): `BED` only marks an already-declared POI, it carries no
payload of its own.

`game::GameLoop::handleRest` (ordinary Rest, `'r'`) and `handleBedRest`
(`'z'`) share `character::Character::lastRestDay` as the same "one
overnight action per in-game day" gate -- a player can't use both in the
same day, in either order. Bed rest costs the same 8 hours as ordinary
Rest but heals fully to `maxHp` instead of 1 hp, a deliberate simplification
of the DMG's literal "complete bed-rest" rule -- see `docs/CHARACTER_NOTES.md`
for the full sourcing and why the literal multi-day/weekly-bonus version
was dropped.

As of Milestone 41, exactly one POI carries `BED`:
`data/zones/solace_inn.txt`'s `U "The Stairs Up"` -- already flavor-texted
as leading to the Inn's private rooms, and already noted (see "The Inn of
the Last Home specifically" below) as "not a modeled zone yet." No other
zone has an authored Inn/lodging POI, so no other zone got one.

## Quests: POIs that offer them (Milestone 51)

`QUEST <char> <quest-id>` marks a POI as a quest-giver: talking to it
offers, updates, or turns in the referenced `quest::Quest`
(`data/quests.txt` — full grammar and design model in
`docs/QUEST_NOTES.md`). Modelled on `PORTAL` rather than `SHOP`/`BOAT`/`BED`
because, like `PORTAL`, it carries an id payload that needs cross-file
validation — but unlike `PORTAL` (validated by `ZoneCatalog` against its
own loaded zones), a quest id is validated in `main.cpp`, the first point
both a loaded `ZoneCatalog` and a loaded `quest::QuestCatalog` exist
together (`ZoneLoader` itself can't see `QuestCatalog` and shouldn't — see
`docs/ARCHITECTURE.md`'s dependency direction). Same "must already have a
POI **and** a TALK line" prerequisite as `BOAT`: a quest is offered through
talking, so a POI with no `TALK` line could parse cleanly but never
actually be reachable in play.

At most one quest per POI in v1 — a deliberate scope cut, not a grammar
limit that will obviously widen; see `docs/QUEST_NOTES.md`'s cut list.

As of Milestone 51, exactly one POI carried `QUEST`:
`data/zones/solace.txt`'s `B "Notice Board"`, offering `road_wolves`. Its
description ("armies on the move in the east") already existed as the
"future timeline engine to eventually make literal" hook noted below —
turned out to be the quest engine that made it literal, not the timeline
one, but the same planted flavor either way.

Milestone 52 (the real content pass) added four more, each reframing a
POI's already-existing `TALK` flavor rather than inventing a new hook from
scratch — see `docs/QUEST_NOTES.md`'s "Shipped quests" for the full list:
`data/zones/solace_inn.txt`'s `O` (Otik), `data/zones/high_clerist_tower.txt`'s
`K` (the Garrison Knight), `data/zones/kalaman.txt`'s `G` (the City
Watchman), and `data/zones/silvanesti.txt`'s `W` (the Silvanesti Warder) —
the last of these also the first `QUEST` gated by a `REQUIRE` other than
`knight` (`REQUIRE elf`).

Milestone 53 added a fifth, `data/zones/high_clerist_tower.txt`'s new POI
`S` ("A Sword Knight") — placed next to `K` since `K` already carries
`word_for_the_tower` and v1 allows only one `QUEST` per POI (see below).
`S` reframes the zone's own Muster Yard (`Y`) flavor text rather than
inventing new lore; see `docs/QUEST_NOTES.md`'s "Shipped quests" for the
full writeup.

The Dragonlance magical items milestone added a sixth,
`data/zones/high_clerist_tower.txt`'s new POI `L` ("A Knight of the
Circle"), offering `solamnic_armor` — a third POI at the same zone (after
`K` and `S`, both already spoken for), placed near the existing Muster
Yard/Sword Knight cluster. This is also this project's first `QUEST`
whose turn-in grants an item (`REWARD_SOLAMNIC_ARMOR`) rather than steel/
XP/a rank — see `docs/QUEST_NOTES.md`'s "Shipped quests" and
`docs/CHARACTER_NOTES.md`'s "Magic items".

The Order of the Rose milestone added a seventh,
`data/zones/high_clerist_tower.txt`'s new POI `R` ("A Rose Knight"),
offering `measure_of_roses` — a fourth POI at the same zone (`K`/`S`/`L`
all already spoken for), same Muster Yard row. Completes the Crown->
Sword->Rose chain; see `docs/QUEST_NOTES.md`'s "Shipped quests" and
`docs/CHARACTER_NOTES.md`'s "Knights of Solamnia".

The Frostreaver milestone added an eighth, reusing an *existing* POI
rather than adding a new one: `data/zones/ice_wall.txt`'s `K` (the Young
Knight, see "Ice Wall Castle" below), offering `frostreaver_salvage`. Like
`solamnic_armor`, its turn-in grants an item (`REWARD_FROSTREAVER`) rather
than steel/XP/a rank; unlike every prior `QUEST`, it's gated by `REQUIRE
str_13`, a raw ability-score check rather than race/class/knight-rank —
see `docs/QUEST_NOTES.md`'s "Shipped quests" and `docs/CHARACTER_NOTES.md`'s
"Magic items".

## Aftermath dialogue: POIs that react once the Heroes have moved on

`TALK_AFTER <char> <dialogue...>` marks a POI's reaction to the Heroes of the
Lance having already come and gone from this zone's effective timeline
location -- shown instead of the ordinary `TALK`/`TALK_AGAIN` greeting the
first time this POI is talked to once every character `data/timeline.txt`
schedules here has a `PRESENCE` window whose `dayEnd` has passed (checked via
the new `timeline::Timeline::latestDayEnd`, see `docs/TIMELINE_NOTES.md`).
Same "layers a reaction on top of an existing POI" pattern as `SAY_IF`/
`TOPIC`, and the same "must already have a `TALK` line to react against" load-
time validation -- but keyed on elapsed in-game time rather than the player's
own race/class/alignment.

**Shown exactly once, independent of "have I talked to this NPC before".**
Unlike `SAY_IF` (checked only the very first time a POI is ever talked to),
aftermath dialogue is tracked under its own synthesized id
(`"<talk-candidate-id>:after"` in `GameState::metCharacters` --
`game::GameLoop::talkTo`) so it fires correctly even for a player who already
met this NPC *before* the Heroes' window ever opened (e.g. talked to Otik on
day 0, then returns on day 20) -- the ordinary "have I met them" state
(`TALK`/`TALK_AGAIN`) would otherwise mask it entirely. No new save format:
this reuses the existing `MET` line as-is, the same "met-id naming
convention" precedent `docs/TIMELINE_NOTES.md`'s "Met-tracking id exception"
already established for `TIMELINE_ANCHOR`. After the one-time aftermath line,
later visits fall back to ordinary `TALK_AGAIN`/recognition behavior -- there
is no `TALK_AFTER_AGAIN`, a deliberate restraint call: the specific "they were
here, and left" fact only needs saying once.

Deliberately no conditional (`SAY_IF`-style) variant of aftermath dialogue,
and it only applies to zone-native POIs, never a `TIMELINE_ANCHOR` candidate
(a departed canon character already stops appearing there for free, once
`Timeline::presentAt` no longer returns them -- see "Zone-interior
encounters" above) -- same one-flat-mechanism-first restraint `BOAT`/
`GRANTS_ITEM`/`BED` each shipped with.

Shipped on ten POIs so far. `data/zones/solace_inn.txt`'s `O` (Otik) was the
original Milestone 66 proof of concept, referencing the shared `PRESENCE
solace 0 1` window (all 8 Heroes) and the day 2-3 Haven/Darken Wood split
that follows it (`docs/TIMELINE_NOTES.md`) -- deliberately hedged ("some say
... others swear ...") rather than picking one, matching that same window's
own established two-versions-of-one-leg ambiguity. Milestone 70 then widened
it to four more, same "reframe an existing NPC's established voice" approach
the quest-widening pass (Milestone 52) already used successfully:
`data/zones/haven.txt`'s `G` (Seeker Guard, `PRESENCE haven 2 3`),
`data/zones/xak_tsaroth.txt`'s `S` (Ruin-Scavenger, `PRESENCE xak_tsaroth 4
6`), `data/zones/qualinesti.txt`'s `E` (Elven Sentinel, `PRESENCE qualinesti
7 9`), and `data/zones/kalaman.txt`'s `G` (City Watchman, covering both
`PRESENCE kalaman 90 92` and `100 100`, since `latestDayEnd` only fires once
every window at that location has closed). Each ties back into a detail from
that character's own `PRESENCE` flavor text or the POI's own established
voice -- e.g. the Watchman's line callbacks to his own "light fingers" joke
and the zone's existing locked Cartographer's Stall POI, and the Seeker
Guard's line callbacks to Tasslehoff's `PRESENCE` line badgering that exact
guard with theology questions. Milestone 77 then widened it to five more,
the same technique again: `data/zones/darken_wood.txt`'s `U` (the unicorn/
Forestmaster, `PRESENCE darken_wood 2 3`, all eight Heroes -- reframed
through her own established "certainty, not a voice" judging voice),
`data/zones/high_clerist_tower.txt`'s `K` (Garrison Knight, covering both
`76 80` and `81 81` -- Sturm's Knighting and death, Flint's hand on the
ambush lever, Laurana's eulogy), `data/zones/ice_wall.txt`'s `K` (Young
Knight, `38 42` -- Laurana's stillness against the Ice Reaver's fear magic
alongside Sturm/Flint/Tasslehoff), `data/zones/silvanesti.txt`'s `W`
(Silvanesti Warder, `25 30` -- the five-Hero river-crossing subgroup,
without naming Alhana Starbreeze or Tika), and `data/zones/palanthas.txt`'s
`K` (Knight of the Watch, covering both `83 83` and `83 89` -- Raistlin's
ambiguous collapse tied to Laurana's rise to Golden General, a detail his
own pre-existing `TOPIC K "A City Under Watch"` already referenced).
Milestone 80 then closed out the backlog with the last three real
candidates: `data/zones/pax_tharkas.txt`'s `G` (Fortress Guard,
`PRESENCE pax_tharkas 10 12` -- the Sla-Mori/chain-room climax, kept in
his own understated, uninvolved voice), `data/zones/tarsis.txt`'s `S`
(Old Sailor, `PRESENCE tarsis 20 22` -- ties into his own established
wizard/outsider hostility, with Raistlin's library hunt getting the most
attention), and `data/zones/neraka.txt`'s `G` (Deserting Guard,
`PRESENCE neraka 105 107` -- only the characters actually present at
Neraka, not all eight, filtered through his own shaken,
too-busy-running-to-ask voice); Tarsis's other candidate, the Knight's
Runner, was deliberately left alone to keep the established "one
`TALK_AFTER` per zone" pattern intact. Thirteen POIs now, and every real
candidate is done -- Godshome has no talkable zone-native NPC at all and
stays out of scope permanently.

## Anticipation dialogue: POIs that react before the Heroes arrive

`TALK_BEFORE <char> <dialogue...>` marks a POI's reaction to the Heroes of
the Lance not having arrived at this zone's effective timeline location
*yet* -- the mirror image of `TALK_AFTER` above, shown instead of the
ordinary `TALK`/`TALK_AGAIN` greeting while every character
`data/timeline.txt` schedules here has a `PRESENCE` window whose `dayStart`
hasn't been reached (checked via `timeline::Timeline::earliestDayStart`, see
`docs/TIMELINE_NOTES.md`). Added in Milestone 68, prompted directly by
Milestone 67's movement-granularity change: once an ordinary tile-move
stopped costing a flat hour, beelining to a story stop started landing
*before* its window opens far more often than not (confirmed concretely for
Haven -- a direct walk from Solace is 40 hours, but `PRESENCE haven 2 3`
doesn't open until hour 48) -- previously silent, since `Timeline::presentAt`
simply returns nothing early.

**Shown on every visit while the condition holds -- deliberately not the
same "exactly once" treatment as `TALK_AFTER`.** Aftermath dialogue narrates
a specific past event ("they came, and left") that only needs saying once
before falling back to ordinary `TALK_AGAIN`. Anticipation dialogue is the
opposite in nature: an ongoing truth ("they still aren't here") that stays
accurate across every early visit, and the condition itself is
self-expiring -- it simply stops firing on its own once `dayNow` reaches
`earliestDayStart`, with no flag needed to prevent staleness. So unlike
`dialogueAfter`, `dialogueBefore` (`world::PointOfInterest`,
`game::TalkCandidate`) needs no synthesized `GameState::metCharacters`
tracking id at all -- `GameLoop::talkTo` just checks it as a plain
condition, ahead of the ordinary `alreadyMet`/greeting branches (same
"shown instead of" precedence `TALK_AFTER` already has).
`state_.metCharacters.insert(id)` for the NPC itself still fires
unconditionally afterward, exactly as it always has, so the NPC is
correctly "met" either way.

Deliberately no conditional (`SAY_IF`-style) variant, and it only applies to
zone-native POIs, never a `TIMELINE_ANCHOR` candidate -- same reasoning
`TALK_AFTER` already established: a not-yet-arrived canon character already
fails to appear there for free, once `Timeline::presentAt` doesn't return
them yet.

As of this feature's introduction, exactly one POI carries `TALK_BEFORE`:
`data/zones/haven.txt`'s `G` (the Seeker Guard), reacting to the shared
`PRESENCE haven 2 3` window. Not Otik/Solace -- Solace's own window
(`PRESENCE solace 0 1`) starts at day 0, the game's first possible day, so
there's no "before" period to demonstrate there at all. Written in the
Guard's already-established tired/wary voice, general unease about
travelers on the roads rather than naming any Hero (he has no way to know
who's coming), same non-infringing, inspired-not-transcribed standard as
every other line in this file. Widening to other zones is natural follow-up
work, not done this pass -- same "one proof-of-concept first" precedent
`TALK_AFTER` itself just set.

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
entry road, two named vallenwood trees, a notice board, a general store,
Flint Fireforge's smithy, and the door to the Inn of the Last Home
(`PORTAL I solace_inn` — see above). Its layout is original — not a copy
of any published map — consistent with the same non-infringing,
inspired-by-canon approach used for location descriptions elsewhere in
this project.

The notice board (`POI B`) was originally authored as "a small, deliberate
nod to 'rumors of war' flavor for the future timeline engine to eventually
make literal" — as of Milestone 51 it's the game's first quest-giver
(`QUEST B road_wolves`, see "Quests: POIs that offer them" above), reading
the same "armies on the move in the east" description it always had.

Flint Fireforge's smithy (`POI S`) was pure scenery until the DELIVER
milestone, which gave it a `TALK S` for an unnamed journeyman who keeps
the forge running — written evergreen, deliberately never claiming to
*be* Flint (his own tracked `PRESENCE` schedule may have him elsewhere,
or already dead by Godshome, depending on the game day — see
`docs/TIMELINE_NOTES.md`). `QUEST S ore_for_the_forge` — see
`docs/QUEST_NOTES.md`'s "Shipped: ore_for_the_forge".

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
private rooms that aren't a modeled zone of their own (the rooms
themselves are described as flavor text only, same deliberately-deferred
treatment as everywhere else this project describes-not-models an
interior) — though as of Milestone 41 the stair's own tile is functional,
not purely decorative: it carries `BED U`, letting the player press `z`
there for complete bed-rest (see "Beds: POIs for complete bed-rest"
above).

As of Milestone 26, Otik and Tika each have a `SAY_IF`/`TOPIC` pair
grounded directly in their real DAT dialogue (verified via `pdftotext`
against the actual novel, not written from memory): Otik's wry,
lowered-voice line about theocrats needing their bellies filled same as
anyone (`"Even theocrats need to fill their bellies..."` in the source)
becomes his `TOPIC "Keeping the Peace"`; Tika's real unease about war
rumors and "strange, hooded men" around the High Theocrat becomes her
`TOPIC "Strange Talk"`, deliberately distinct from Otik's own
dismissiveness of the same rumor in the book.

Otik also carries this project's first `TALK_AFTER` line -- see "Aftermath
dialogue: POIs that react once the Heroes have moved on" below -- describing
the reunion having already happened and the company moving on toward Haven
or Darken Wood, the same two-versions-of-one-leg ambiguity
`docs/TIMELINE_NOTES.md` already documents for that stretch of the schedule.

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
  out of the mist overhead — all scenery. Originally deliberately had no
  talkable NPC, since the ruins themselves are abandoned; the Staff of
  Striking/Curing milestone (`docs/QUEST_NOTES.md`) added one anyway — a
  Ruin-Scavenger (`S`), someone passing through rather than a resident,
  the same distinction that's kept the ruins' "no one lives here" premise
  intact while still grounding a quest giver in the zone's existing relic-
  hunting flavor (DLA's own Bupu's Emerald is sourced to these same
  ruins).

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
  re-reading why it's here. The DELIVER milestone added a second POI, `O
  "An Ore Cart"` (abandoned near the Mine Entrance, in keeping with the
  zone's existing "war ... over who controls what's dug from it" flavor
  — see "Quest items" above), carrying `GRANTS_ITEM O raw_tharkadan_ore
  Raw Tharkadan Ore`. Deliberately its own tile rather than added to `M`
  (the Mine Entrance) — `M` is this zone's `TIMELINE_ANCHOR`, and no zone
  shipped so far combines a `TIMELINE_ANCHOR` tile with its own
  zone-native `TALK` line (anchors are always pure scenery in every zone
  to date), so the grant stayed off that tile rather than exercising an
  untested combination.
- `data/zones/plains_of_dust.txt` — a Plainsfolk camp. Deliberately an
  unnamed tribe's camp, not Que-Shu specifically, since DL3 establishes
  Que-Shu as destroyed by the time of that module's story — using an
  original tribe sidesteps that conflict entirely.
- `data/zones/tarsis.txt` — the old harbor district, stranded since the
  Cataclysm. Not directly covered in DL1-3 (the adventure ends before
  reaching it), so this one leans on general established Dragonlance
  lore and this project's own existing `locations.txt` description
  (beached ship hulls, a dry stone quay, a xenophobic-toward-outsiders
  Old Sailor NPC) rather than a specific module citation. As of
  Milestone 35 it gained a `TIMELINE_ANCHOR D` (the Old Dock) once real
  timeline content finally existed for it — see `docs/TIMELINE_NOTES.md`.
  As of Milestone 36 it also has `R "A Knight's Runner"`, the zone's
  `BOAT`-carrying POI (see "Boats: POIs that grant sea travel" above) —
  the party's actual route off this landlocked city, since the Old
  Sailor's own dialogue rules out a local ship.

## The High Clerist's Tower (Milestone 35)

`data/zones/high_clerist_tower.txt` — the fortress's outer courtyard, the
first zone built for *Dragons of Winter Night* content. An original
layout (not copied from any published floorplan), grounded in on-page
details from the actual siege/Knighting/death chapters (verified via
`pdftotext -layout` against the novel, not written from memory): the
Chapel of the High Clerist (`C`, where Sturm keeps his knighting vigil,
described generically enough to read sensibly whether visited before or
after his window — the zone itself has no day-gating, so its flavor text
can't presuppose a scheduled event has already happened), the Tower's
sealed inner doors (`D`, "no one but a High Clerist may enter" — described,
not modeled, same treatment as the Inn's upstairs rooms and Pax Tharkas's
Sla-Mori/mines), a stair to the battlements (`B`, where Sturm's death
takes place in the novel — flavor-only, since this engine has no
scripted-death combat), the Muster Yard (`Y`, the `TIMELINE_ANCHOR` — kept
as a distinct scenery POI rather than the talkable Knight's own tile,
matching every other anchored zone's pattern of anchoring on scenery, not
the zone-native NPC), and one generic talkable Knight (`K`). The Knight is
deliberately unnamed rather than Derek Crownguard/Lord Alfred/Lord
Gunthar — all three have canon fates (two die during the siege, one stays
on Sancrist) this engine has no mechanism to represent for a permanent
NPC, same reasoning `pax_tharkas.txt`'s deliberately unaffiliated
"Fortress Guard" already established. His `TOPIC`s fold in the Sancrist
Knights' Trial (Sturm's real vindication scene, and the Gunthar/Derek
factional split it caused) as retrospective dialogue, since Sancrist
Isle itself isn't modeled — see `docs/TIMELINE_NOTES.md` for why.

Milestone 53 added a second talkable Knight, `S` ("A Sword Knight"), next
to the Muster Yard -- see "Quests: POIs that offer them" above and
`docs/QUEST_NOTES.md`'s "Shipped quests" for why `K` couldn't carry a
second `QUEST` line.

The Dragonlance magical items milestone added a third, `L` ("A Knight of
the Circle"), for the same reason -- `K` and `S` were both already
spoken for. Placed on the same row as `Y`/`S`, a few tiles east, still
inside the Muster Yard's open ground rather than off in its own corner --
this Knight's whole reason for being there is the Circle convening on
that same ground. Offers `solamnic_armor`; see "Quests: POIs that offer
them" above and `docs/QUEST_NOTES.md`'s "Shipped quests".

The Order of the Rose milestone added a fourth, `R` ("A Rose Knight"), same
row again, same reasoning -- `K`/`S`/`L` were all already spoken for, and
the Muster Yard is already established as where the Order convenes. Offers
`measure_of_roses`, the capstone of the Crown->Sword->Rose chain; see
"Quests: POIs that offer them" above and `docs/QUEST_NOTES.md`'s "Shipped
quests".

## Ice Wall Castle (Milestone 36)

`data/zones/ice_wall.txt` — the castle's ruined outer hall, reached via
the scripted sea voyage granted by Tarsis's Knight's Runner (see "Boats:
POIs that grant a scripted sea voyage" above and `docs/ARCHITECTURE.md`).
An original layout, grounded in the actual
dragon-orb-quest chapters of *Dragons of Winter Night* (verified via
`pdftotext -layout`, not written from memory): an ice-bound silver dragon
with a mysterious rider (`D`, deliberate foreshadowing the novel itself
doesn't pay off yet, so this project doesn't invent a payoff either — same
"described, not modeled" restraint as the Tower's sealed doors), the spot
where the dark elf Feal-thas fell defending the orb (`F`, the
`TIMELINE_ANCHOR` — scenery, not the talkable Knight's own tile, matching
every other anchored zone's established pattern), and one generic talkable
Knight (`K`, one of Derek Crownguard's two unnamed knights — Derek himself
stays off-stage, same reasoning the Tower already established for not
naming him directly). The Knight's `TOPIC`s fold in the Thanoi
("walrus-men") and a brief Southern Ergoth sail-past mention as
retrospective dialogue rather than inventing new locations/monsters for
either — see `docs/TIMELINE_NOTES.md` for why both stay unmodeled this
milestone. Thanoi are flavor-only text, not added to `data/monsters.txt`.

**Milestone 91** added a second talkable POI, `B "An Ice Barbarian Guide"`,
placed one tile past the doorway `ENTRY` (not on it -- see "Boats" above
for why the Young Knight `K` couldn't double as this zone's travel-granter,
now that he's also the `frostreaver_salvage` quest-giver). Grants
`BOAT B sancrist_isle 48`, the second leg of the Tarsis → Ice Wall →
Sancrist voyage.

## Silvanost, Silvanesti's capital (Milestone 37)

`data/zones/silvanesti.txt` — same `LOCATION`-is-the-nation/zone-is-the-
capital relationship `qualinesti.txt`/Qualinost already established. An
original layout, grounded in the actual dragon-orb-crisis chapters of
*Dragons of Winter Night* (verified via `pdftotext -layout`, not written
from memory): the Ferry Landing (`F`, the actual crossing point the text
describes), the Tower of the Stars (`T`, the `TIMELINE_ANCHOR` — Lorac
Caladon enthroned beside the dragon orb, described, not an
enterable/resolvable puzzle, same restraint as the Tower's sealed doors
and Ice Wall's ice-bound dragon), the Twisted Gardens (`G`, the
"trees weep blood" corruption imagery, written evergreen since the source
material itself never resolves Lorac's fate cleanly), and one generic
talkable Warder (`W`, third use of this project's "unnamed sentinel"
pattern after the Fortress Guard and the Tower/Ice Wall Knights).
Deliberately **not** Alhana Starbreeze by name, despite her being a major
on-page character here — she has extensive ongoing plot significance
beyond this book, the same reasoning that's kept Derek Crownguard and
Lord Gunthar off-stage as named NPCs.

## Kalaman (Milestone 39)

`data/zones/kalaman.txt` — the harbor district and market square, this
project's first *Dragons of Spring Dawning* zone. An original layout at
the 44×16 ceiling (matching the Tower/Silvanesti's own 40×16 grids),
grounded in the actual festival and reunion chapters of *Dragons of Spring
Dawning* (verified via `pdftotext -layout`, not written from memory): the
Harbor (`H`, "white-winged ships lay at anchor"), the Lord's Keep (`L`,
where the victory speeches happen), a stair to the city wall (`W`, where
the whole city gathers to watch the Dark Lady's arrival), a locked
Cartographer's Stall (`C`, grounding Tasslehoff's actual sourced
pickpocketing/lockpicking beat — flavor-only, not a `SHOP`, since the
scene is specifically about a locked, unattended stall, not a functioning
merchant), the Market Square (`M`, the `TIMELINE_ANCHOR` — both sourced
story beats happen here or pass through it), and one generic talkable
City Watchman (`G`, fourth use of the "unnamed sentinel" pattern after the
Fortress Guard, the Tower/Ice Wall Knights, and Silvanesti's Warder). The
Dragon Highlord who delivers the ultimatum ("the Dark Lady," i.e. Kitiara)
and the Golden General (Laurana) both stayed unnamed throughout this
zone's text at the time it was written — Laurana is named as of
Milestone 49 (see `docs/TIMELINE_NOTES.md`'s "Named vs. off-stage canon
characters"); Kitiara remains off-stage. See `docs/TIMELINE_NOTES.md` for
the two day-windows this zone's `TIMELINE_ANCHOR` serves.

## Palanthas (Milestone 44)

`data/zones/palanthas.txt` — a 40×16 slice of the wheel-shaped city (same
scale as Kalaman), grounded in the actual Astinus/Shoikan-Grove/map-room
chapters of *Dragons of Spring Dawning* (a fresh `pdftotext -layout`
extraction, re-verified independently against the live text a second time
before writing any dialogue). Seven POIs: the Harbor (`H`), the Lord's
Palace (`P`, Amothus's map room), the Great Library (`L`), the Old City
Wall (`W`), the Tower of High Sorcery (`T`), a Knight of the Watch (`K`),
and the Shoikan Oak Grove (`O`).

**Astinus is a permanent, timeline-independent `TALK` NPC** (like Otik/
Tika), not gated to any day window — sourced directly ("he is not known to
leave his Library in Palanthas," *Players Guide to the Dragonlance
Campaign*) rather than invented. He's also the richest-voiced zone-native
NPC so far (a `SAY_IF mage` reaction plus two `TOPIC`s), justified by how
much real, quotable characterization the source material actually gives
him (the novel's "I am the world" speech, the Player's Guide's "distant...
reticent... restating the obvious" description) — more investment than the
generic "unnamed sentinel" archetype (the Knight here, sixth use of that
pattern after the Fortress Guard, the Tower/Ice Wall Knights, Silvanesti's
Warder, and Kalaman's City Watchman) gets, deliberately, since there's no
comparable individual voice to draw on for a generic guard.

**The Library tile doubles as the zone's `TIMELINE_ANCHOR`, sharing a tile
with Astinus's own `TALK` — not a conflict, and not a coincidence.**
`Zone` has exactly one `timelineAnchorPoi` per zone (a hard constraint, not
a style choice), and a POI's own `dialogue` and the anchor's
`timeline_.presentAt(...)` results are two independent candidate sources
that both feed `GameLoop::pickAndTalk`/`pickAndLook` — the same layering
Kalaman's Market Square already established. Here it's also the *sourced*
choice: Tasslehoff's witnessed beat in the book is watching Raistlin
carried into this exact library, so the evergreen Astinus and the
day-gated Heroes belong on the same tile because the text puts them there
together, not because it was convenient.

**The Tower of High Sorcery and the Shoikan Oak Grove are pure scenery** —
no `TALK`, no `PORTAL`, no interior. Standing on the Tower's tile *is* the
interaction (every POI tile is walkable by design, regardless of what it
represents — see the Milestone-era bugfix in `docs/GOTCHAS.md`), matching
this project's established "described, not modeled" treatment of
unenterable structures (the Tower of the Stars, the High Clerist's Tower's
own sealed interior). **The Lord's Palace is scenery too — Amothus is
named directly in its description but never becomes a `TALK` NPC**, same
precedent as Lorac Caladon/Feal-thas: named in flavor text without being
made talkable, reserved for a character with neither the extensive ongoing
canon plot that (at the time this zone was written) kept Kitiara, Laurana,
Alhana, Derek, and Gunthar off-stage, nor a strong enough sourced voice
sample to draw dialogue from (unlike Astinus). Laurana is named as of
Milestone 49, though as a schedule-tracked `data/timeline.txt` character,
not a zone-native NPC like Amothus — a different axis than this paragraph.

The airborne dragon battle and Bakaris's capture, later in this same book,
are confirmed (by direct text search) to happen away from the city near
Vingaard Keep — correctly out of scope here, same "don't invent to fill a
gap, and don't drag in an off-site subplot" restraint as Dargaard Keep
staying unmodeled at Kalaman.

## Godshome (Milestone 45)

`data/zones/godshome.txt` — a 40×16 grid, same footprint as every other
top-level zone despite the "small, self-contained" framing (confirmed by
measuring every existing zone before writing this one: "small" means a
sparse POI count, not a smaller grid — see `docs/MAP_NOTES.md`). Three
POIs only, matching the source's own "overwhelming desolation and
emptiness" (*Dragons of Spring Dawning*): the Narrow Cleft (`T`, the
crawl-through tunnel entrance), the Circle of Standing Stones (`C`,
flavor-only), and Where Flint Fell (`F`).

**No full `#` border, unlike Palanthas/Kalaman's walled-room grids** —
same open-floor choice Xak Tsaroth already made, since this is a natural
mountain bowl under open sky, not a room with walls. `Zone::tileCodeAt`'s
existing bounds-safe out-of-range behavior (returns `#`) still keeps the
edges a hard boundary without anything drawn there.

**`TIMELINE_ANCHOR` is `F`, not `C`.** The book's own geography puts
Flint's collapse and the farewell scene on the far side of the bowl from
the standing stones — the circle is where his body is carried *after*,
not where the scene the player finds happens. `C`'s own description
(the black, star-filled pool) is deliberately evergreen, same "true
regardless of when a player visits" treatment as the Tower's Muster Yard
and Silvanesti's Tower of the Stars — nothing about it changes based on
whether day 103 has passed.

**Not the same Godshome as the "Ruins of Godshome" a few days' walk
northwest on the reference map** — a real, separate, legible label
confirmed directly on the map image, matching Tasslehoff's own
"no, not that Godshome" line in the source text. The ruined city is not
built; it exists only as one flavor beat in `data/timeline.txt`'s
dialogue, not a second `LOCATION`.

Berem and Fizban (the old mage) are both present in the source scene but
stayed unnamed in this zone's text and in the Heroes' dialogue at the
time this zone was written — Fizban is named as of Milestone 48 (see
`docs/TIMELINE_NOTES.md`'s "Named vs. off-stage canon characters"); Berem
never gets his own `CHARACTER` block.

## Neraka (Milestone 46)

`data/zones/neraka.txt` — a 40×16 grid with a **full `#` border**, unlike
Godshome's open bowl, matching the source text's own "walled Temple
compound" and the established Pax Tharkas/Ice Wall bordered-fortress
idiom directly (same bottom-wall three-tile gap convention as both).
Four POIs: the Temple of the Dark Queen (`T`, flavor-only, deliberately
ambiguous about what happened inside — same restraint as Godshome's
Circle of Standing Stones), the Dungeons (`D`, flavor-only), a Deserting
Guard (`G`, the zone's one talkable NPC, `TALK`/`TALK_AGAIN`/one `TOPIC`,
same shape as Pax Tharkas's Fortress Guard and Ice Wall's Young Knight),
and the Temple Square (`S`, the `TIMELINE_ANCHOR`).

**`TIMELINE_ANCHOR` is `S`, not `T` or `D`.** Unlike Godshome's `F`
("Where Flint Fell" — literally where the scripted event happens), none
of the three surviving tracked Heroes actually die or resolve anything
at a single fixed spot inside Neraka; the anchor instead represents the
aftermath/reunion scene the source text stages just outside the ruined
walls (the campfire, the whole surviving party regrouping). `S`'s own
POI description is deliberately evergreen — true whether a player visits
before, during, or after the tracked Heroes' `105`–`107` window — same
treatment as Godshome's `C` and every other evergreen-flavor POI in this
project.

**The Deserting Guard, not a fixed named NPC.** Every prior
bordered-fortress zone (Pax Tharkas, Ice Wall) put its one talkable POI
on whoever's currently holding the place; Neraka's guard is written as a
soldier whose own chain of command is actively collapsing around him
(grounded in the source's own "harried-looking draconian... probably a
deserter" beat), giving a non-Hero-voiced window onto the dragonarmies'
collapse without inventing a named Highlord or staging Kitiara/Ariakas
directly.

Kitiara, Laurana, Ariakas, and Lord Soth are all real presences in the
source chapters covered here but stayed entirely unnamed/off-stage in
this zone's own text at the time it was written. Laurana is named as of
Milestone 49, with real dialogue at this same `neraka 105 107` window
(see `docs/TIMELINE_NOTES.md`'s "Named vs. off-stage canon characters");
Kitiara, Ariakas, and Lord Soth remain off-stage. See
`docs/TIMELINE_NOTES.md` for the full reasoning, including why Raistlin's
own real reunion scene with Caramon here does **not** add a
`PRESENCE neraka` entry to Raistlin's own `CHARACTER` block.

## Thorbardin (Milestone 86)

`data/zones/thorbardin.txt` — a 40×16 grid with a full `#` border
(carved dwarven halls, not open sky — closer to Neraka's walled-compound
precedent than Godshome's open bowl). Three POIs: the Great Hall of
Audience (`H`, the `TIMELINE_ANCHOR` — the Hammer of Kharas ceremony's
actual setting, the seven Thane-thrones folded into its own description
rather than given a separate POI), Southgate (`G`, flavor-only, the
sixty-foot gate mechanism), and the Refugee Quarter (`R`, the zone's one
talkable NPC).

**The Refugee Quarter's NPC is deliberately unnamed, not Hederick.**
*Dragons of Winter Night*'s Council-of-Highseekers scene has Hederick
himself voice the "we're grateful, but we're not dwarves" complaint this
NPC's dialogue is grounded in — but Hederick is an established DAT-era
character (Solace's corrupt Theocrat) this project hasn't sourced
speaking at Thorbardin specifically, and giving him new dialogue here
without a fresh, direct citation for it would be inventing rather than
adapting. A generic refugee voices the same real sentiment instead, same
"non-Hero-voiced window" precedent Neraka's Deserting Guard and Xak
Tsaroth's Ruin-Scavenger already established.

See `docs/TIMELINE_NOTES.md`'s "Thorbardin" section for the full
sourcing and the `PRESENCE thorbardin 13 19` content shipped for all 8
Heroes.

## Sancrist Isle (Milestone 86, gained a talkable NPC + departure Milestone 92, topic depth Milestone 97)

`data/zones/sancrist_isle.txt` — a 40×16 grid, full `#` border (a real
castle, same fortress idiom as Pax Tharkas/Ice Wall/Neraka). Three
flavor-only POIs: the Great Hall of Castle Uth Wistan (`T`, the
`TIMELINE_ANCHOR` — the Knights' Trial's actual setting, the vacant
Grand Master/High Clerist seats folded into its own description), the
Guest Quarters (`Q`, where the text confirms Flint and Tasslehoff stayed
during the trial), and the Yule Fire Room (`F`, the small room Gunthar
takes Sturm to afterward for his private blessing — evergreen, true
whether a player visits before, during, or after the tracked Heroes'
window, same treatment every other zone's atmosphere-only POI gets).

**Milestone 86 originally shipped with no talkable NPC** — unlike every
other fortress zone in this project (Pax Tharkas's Fortress Guard, Ice
Wall's Young Knight, Neraka's Deserting Guard), Sancrist Isle's only
sourced on-page dialogue belonged to Sturm, Flint, Tasslehoff, and
Laurana themselves, all four already covered via `TIMELINE_ANCHOR T`.
Inventing a fifth, generic castle guard just to have a zone-native NPC
would have been adding content the source doesn't call for, not filling a
real gap. That call held until Milestone 91 made Sancrist Isle a real sea
voyage *destination* — at which point "no talkable NPC" also meant "no
way to ever leave," a genuine reachability gap this project's own
restraint principle doesn't ask for. **Milestone 92 added one talkable
POI**, `E "An Embarkation Officer"` (one tile east of `ENTRY`, matching
Milestone 91's "not on the entry tile itself" placement rule), granting
`BOAT E palanthas 96` — tied to the *specific* sourced detail that Sturm's
army mustered at Sancrist to sail for Palanthas
(`.research/dwn_full.txt` lines ~10628-10731), the same "grounded in one
concrete textual detail, not a generic guard" precedent the Runner and Ice
Barbarian Guide already established, not a reversal of Milestone 86's
restraint so much as an application of it to a fact that only became
relevant once Sancrist was reachable at all. See `docs/ARCHITECTURE.md`'s
"Sea travel" and "Boats" above for the mechanism.

This reverses this project's own prior "deliberately not modeled" call
for Sancrist Isle — see `docs/TIMELINE_NOTES.md`'s "Sancrist Isle"
section for the full reasoning, the reversal, and complete sourcing.

**Milestone 97 split the Embarkation Officer's `derek,alfred,brightblade,
sturm` `SUBJECT` group into three** (`derek,crownguard,rose` /
`alfred,markenin,sword` / `brightblade,sturm,crown`), each re-sourced from
the same trial/muster passage rather than one shared reply — found during
this project's first real interactive playtest of this POI, which
surfaced the original grouping (correct by Milestone 92's own precedent,
but noticeably thinner once actually played) as worth expanding.
`palanthas,tower,army` is unchanged.

## Crossing (Milestone 93)

`data/zones/crossing.txt` — a 31×12 grid, well under the viewport floor,
open ground with a full `#` border and a 3-tile gap in the south wall for
`ENTRY`. Two POIs: the Quay (`Q`, flavor-only — the strait's ferry piers)
and the Ferry Keeper (`K`, the zone's one talkable NPC), with two
`TOPIC`s ("The Strait," "The War") and a `SUBJECT` on the neighboring,
unmodeled coastal keeps (North Keep, Restglen) her dialogue name-drops.

**No `TIMELINE_ANCHOR`, no `PRESENCE`, no canon-character content at
all** — unlike every other zone this project has added, Crossing wasn't
sourced from any of the three novels; it's sourced entirely from the
reference map's own "Crossing" label (see `docs/MAP_NOTES.md`'s
"Crossing" section) and `TSR 2143 Player's Guide`'s "Straits of
Schallsea" geography. The Ferry Keeper is a fully generic local, same
"non-Hero-voiced, invented-but-flagged flavor" treatment already given to
Thorbardin's Refugee Quarter NPC, not a stand-in for anyone specific.

Deliberately no `BOAT` grant either — resolved with the user before
building: the strait's shallow water was already made foot-passable
without a boat at Milestone 87, so Crossing doesn't need the sea-travel
mechanism to do its job; it's a plain waypoint, reached and left on foot
like every other location.

## Southern Ergoth (Milestone 95)

`data/zones/southern_ergoth.txt` — a 40×16 grid, open ground with no
border at all (an outdoor wilderness camp, same borderless idiom as
Godshome, not the walled-fortress idiom of Ice Wall/Sancrist/Neraka).
Three POIs: the Wrecked Shore (`W`, flavor-only — the salvaged wreckage
proving someone made it to land), a Silvanesti Sentry (`S`, the zone's
one talkable NPC), and the Wilder Elves' Camp (`G`, the
`TIMELINE_ANCHOR` — where Silvara and Fizban's tracked `PRESENCE`
windows resolve, alongside Sturm/Flint/Tasslehoff/Laurana's own).

**Deliberately sparse**, same restraint Godshome's "three POIs, not a
sprawling city" precedent already set — this is a refugee wilderness
camp, not a settlement, and the source material compresses cleanly into
a shore/checkpoint/camp shape without inventing extra locations to fill
space.

The Sentry carries this project's now-standard two-`SUBJECT` "ask about
anything" treatment (the three elven kindreds sharing this coast; the
wreck and the stranger things that have washed up since) plus a `TOPIC`
covering the same ground at more length, and the new `BOAT S
sancrist_isle 60` grant — see "Boats" above. No `SHOP`, `BED`, or
`QUEST` — none of those are sourced here, and this project doesn't add
them speculatively.

Reached only via the repointed `data/zones/ice_wall.txt` `BOAT B
southern_ergoth 48` (see "Boats" above) — no `ROAD_PAIRS` entry, same
`SEA_LOCKED` treatment as Ice Wall Castle and Sancrist Isle themselves.
See `docs/MAP_NOTES.md`'s "Southern Ergoth" section for the placement and
`docs/TIMELINE_NOTES.md`'s "Southern Ergoth" section for the full
sourcing, including the correction to this project's own prior
"deliberately not modeled" call.

## Port Balifor and Flotsam (Milestone 96)

`data/zones/port_balifor.txt` — a 40×16 walled town grid (Kalaman's
border-plus-exit-gap idiom, since this is an ordinary inhabited town,
not an open camp). Three POIs: the Pig & Whistle (`W`, William
Sweetwater, the zone's one talkable NPC and the `TIMELINE_ANCHOR`), the
Harbor (`H`, flavor-only — establishes why the Heroes can't simply buy
passage here), and a Draconian Patrol (`R`, flavor-only, atmosphere for
the occupation). William gets the standard `TALK`/`TALK_AGAIN`, two
`TOPIC`s (Raistlin's illusion act; Goldmoon's quietly-spreading healing
reputation), and three `SUBJECT` entries — deliberately no `SHOP`,
`BED`, or `QUEST`, matching this project's "don't add grammar the
source doesn't call for" rule (see Southern Ergoth's Sentry, above).

`data/zones/flotsam.txt` — a 40×16 walled grid, same idiom. Four POIs:
the Refugees' Inn (`I`, flavor-only, the `TIMELINE_ANCHOR` — where the
four Heroes waiting on Tanis are actually found), the Saltbreeze Inn
(`S`, flavor-only — deliberately *not* talkable; see below), a Back
Alley (`A`, flavor-only, echoing the ambush scene without restaging it),
and the Harbor (`H`, a generic dockhand, this zone's one talkable NPC —
`TALK`/`TALK_AGAIN`, a `TOPIC` on the Perechon, three `SUBJECT`
entries). The dockhand's dialogue names Maquesta Kar-thon and gestures at
her "helmsman who can read the Blood Sea's moods... simple as a child"
without naming Berem — the same kind of unstaged foreshadowing this
project has used before naming a character outright (compare Fizban's
unnamed appearances before Milestone 48).

**The Saltbreeze Inn is deliberately flavor-only, no NPC standing in for
Kitiara.** Milestone 50 already decided she stays off the talk/topic
picker entirely, surfacing only as retrospective dialogue inside a
tracked Hero's own `TOPIC` — giving the Saltbreeze an innkeeper NPC who
could describe her directly would work around that precedent instead of
respecting it. Tanis's own new `PRESENCE flotsam` dialogue (see
`docs/TIMELINE_NOTES.md`) stays deliberately evasive about what actually
happens there, for the same reason his existing `TOPIC "A Debt He Won't
Name"` at Kalaman stays coy — this zone's content is the live version of
that same restraint, not a contradiction of it.

Both zones connect by road (`neraka`-`flotsam`-`port_balifor`, see
`docs/MAP_NOTES.md`) rather than sitting `SEA_LOCKED` — unlike Ice
Wall/Sancrist/Southern Ergoth, nothing about either place's own source
material requires a boat to reach it from the rest of the continent.
See `docs/MAP_NOTES.md`'s "Port Balifor and Flotsam" section for
placement and `docs/TIMELINE_NOTES.md`'s own section for the full
sourcing and day-range reasoning.

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
   If a POI is a bed, add a matching `BED <char>` line instead (see
   "Beds: POIs for complete bed-rest" above). If a talkable POI should
   offer a quest, add a matching `QUEST <char> <quest-id>` line, and make
   sure that id exists in `data/quests.txt` (see "Quests: POIs that offer
   them" above and `docs/QUEST_NOTES.md`). If a talkable POI should react
   once the Heroes have moved on from this zone, add a matching
   `TALK_AFTER <char> <dialogue...>` line (see "Aftermath dialogue" above).
6. Build and check the load succeeds (a malformed zone file fails fast with
   a clear error at startup, not partway through play).
