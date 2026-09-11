# Quest notes

## What the quest system is

Milestone 51's answer to "there's a world to explore but nothing to do."
Deliberately built as **glue over systems that already existed**, not a new
engine — this project's world already has places (`world::World`,
`visitedLocations`), people (zone POIs and canon characters,
`metCharacters`), and things to kill (`combat::MonsterCatalog`); a quest
just needed a way to be offered (`GameLoop::talkTo`, already the single
funnel for all dialogue) and a way to track being given one
(`game::GameState`, following the `hasBoat`/`metCharacters` precedent of a
flat field with its own save line). See `docs/CURRENT_WORK.md` for the
Milestone 51 writeup and what was decided with the user before building
this (quest givers are ordinary NPCs, the Notice Board, and Knight of the
Sword advancement — deliberately never the canon Heroes, who are weather in
this project's pitch, not employers).

Like `timeline::Timeline`/`combat::MonsterCatalog`, a `quest::QuestCatalog`
is **static content** — loaded fresh from `data/quests.txt` every run by
`quest::QuestLoader`, never mutated during play. The player's *progress*
lives in `game::GameState::quests` (quest id -> `QuestStatus`) and
`monsterKills` (see "Objectives are queries over existing state" below),
both part of the save format.

`quest::Quest.h`/`Quest.cpp`/`QuestLoader.cpp` depend on nothing else in
this project, the same decoupling `world::World`/`timeline::Timeline`
practice — raw id strings and ints, no `character::Character`, no
`GameState`. The logic that actually evaluates a `quest::Objective`
against live game state (`objectiveProgress`/`objectiveMet`/
`allObjectivesMet`) lives in `game::GameLoop.cpp` instead, as free
functions beside `game::conditionMatches` — same reasoning: `quest/` stays
as ignorant of `game::GameState` as `timeline::PresenceWindow` stays of
`character::Character`.

## File grammar (`data/quests.txt`)

```
QUEST <id>                        block opener, lowercase snake_case --
                                  stable key, used by GameState::quests and
                                  the QUEST <char> <quest-id> zone binding
NAME <text>                       required -- journal title
OFFER <text>                      required -- shown the first time a
                                  quest-giver POI is talked to, before the
                                  Accept/Decline picker
ACCEPT <text>                     required -- shown once the player accepts
PROGRESS <text>                   required -- shown on any later talk while
                                  the quest is active but not yet finished
COMPLETE <text>                   required -- shown on turn-in, immediately
                                  before rewards are dispensed
GIVER <text>                      required -- free text naming who/where
                                  to return to (e.g. "the Notice Board"),
                                  used only in the "ready to turn in" log
                                  line and journal note (see "Proactive
                                  readiness notification" below) -- never
                                  mechanically tied to a real zone POI
REQUIRE <condition>                optional -- game::conditionMatches's
                                  vocabulary (good/evil, the 7 races, the 5
                                  classes, knight, sword_knight, and the
                                  compound sword_eligible -- see below); an
                                  unmet REQUIRE means this quest-giver has
                                  nothing to say about this quest at all to
                                  this character, checked fresh on every
                                  talk, not just at first offer
VISIT <location-id> <label>       one or more objectives, any mix, in
TALK <met-id> <label>             authored order -- <label> is the line
SLAY <monster-id> <count> <label> shown in the journal, hand-written rather
DELIVER <item-id> <count> <label> than derived (see "Objectives are queries
                                  over existing state" below). DELIVER's
                                  <item-id> is a character::InventoryItem::
                                  questItemId, granted in the world by a
                                  zone POI's GRANTS_ITEM line (see
                                  docs/ZONE_NOTES.md) -- see "DELIVER" below.
REWARD_STEEL <n>                  optional, default 0
REWARD_XP <n>                     optional, default 0
REWARD_KNIGHT_SWORD                optional, bare flag (no argument) --
                                  promotes the character to
                                  character::KnightOrder::Sword on turn-in;
                                  see "Knight of the Sword advancement"
                                  below. As of Milestone 53, exactly one
                                  quest (named_in_fact) carries it.
REWARD_SOLAMNIC_ARMOR              optional, bare flag (no argument) --
                                  grants a character::ArmorId::SolamnicArmor
                                  plus an ordinary Shield item on turn-in;
                                  see "Dragonlance magical items" below.
                                  As of that milestone, exactly one quest
                                  (solamnic_armor) carries it. Same "named,
                                  specific, compile-time flag" shape as
                                  REWARD_KNIGHT_SWORD above, not a generic
                                  item-reward mapping -- see "Deliberately
                                  not in v1" below.
REWARD_KNIGHT_ROSE                 optional, bare flag (no argument) --
                                  promotes the character to
                                  character::KnightOrder::Rose on turn-in;
                                  see "Order of the Rose advancement"
                                  below. Exactly one quest (measure_of_roses)
                                  carries it. Same "named, specific,
                                  compile-time flag" shape as
                                  REWARD_KNIGHT_SWORD above.
REWARD_WAYRETH_ROBE                optional, bare flag (no argument) --
                                  assigns character::RobeColor by
                                  alignment and narrates the Test of High
                                  Sorcery outcome on turn-in (see
                                  "wayreth_summons, in Solace" below).
                                  Exactly one quest (wayreth_summons)
                                  carries it. Same "named, specific,
                                  compile-time flag" shape as
                                  REWARD_KNIGHT_SWORD above -- the one
                                  reward flag whose real payoff can't live
                                  in COMPLETE, since COMPLETE is fixed
                                  text and this needed to branch by
                                  alignment.
END
```

No quoted fields anywhere: every objective line's machine-readable
arguments are fixed-arity and positional, so the trailing label is
unambiguous — the same reason `PRESENCE <loc> <start> <end> <flavor>`
needs no quotes while `ZoneLoader`'s `POI`/`TOPIC` do (those have a
machine field *and* free text that could otherwise run together).

`QuestLoader` fails fast (`file:line: message`, same idiom as every other
loader — see `docs/ARCHITECTURE.md`): a keyword outside a `QUEST`/`END`
block; `QUEST` opened before the previous block's `END`; a duplicate quest
id; any of `NAME`/`OFFER`/`ACCEPT`/`PROGRESS`/`COMPLETE` missing at `END`;
zero objectives at `END`; a `SLAY`/`DELIVER` count below 1; an objective
missing its label; end-of-file inside an open block.

## Objectives are queries over existing state, not counters

This is the central design decision, and it shapes everything else here.
An objective doesn't start counting from zero when a quest is accepted —
it's a live read of state the game already tracks:

| Kind | Reads |
|------|-------|
| `VISIT <location-id>` | `GameState::visitedLocations` (tracked since Milestone 3) |
| `TALK <met-id>` | `GameState::metCharacters` (tracked since Milestone 18) |
| `SLAY <monster-id> <count>` | `GameState::monsterKills` (added with this system) |
| `DELIVER <item-id> <count>` | `character::Character::inventory` (added the DELIVER milestone) |

`VISIT` and `TALK` needed **zero new tracking** — those sets already
existed for unrelated reasons and simply happened to answer "has the
player done this" already. `SLAY` needed exactly one new field:
`monsterKills`, a lifetime tally incremented in `GameLoop::runCombat` on
every kill, regardless of whether any quest cares about that monster.
`DELIVER` reads `inventory` directly (counting
`character::ItemKind::QuestItem` entries whose `questItemId` matches) —
see "DELIVER" below for where the item itself comes from.

**Consequence, not a bug: a quest can be instantly completable the moment
it's accepted**, if the player already did the thing before ever hearing
about it. This was already unavoidably true for `VISIT`/`TALK` (those sets
predate quests entirely — a well-traveled character could easily have
already visited the target location), so making `SLAY`/`DELIVER` behave
differently would have been the *inconsistent* choice, not the safe one.
The alternative — snapshotting a per-quest kill/possession baseline on
accept — is a second saved map and an ordering rule, for a game whose
whole combat design is already forgiving (knocked out, not killed; see
`docs/COMBAT_NOTES.md`). Author quest flavor with this in mind rather than
fighting it: `road_wolves`'s `ACCEPT` text ("the wolves, presumably, are
still out there") reads fine either way.

**Labels are authored, not derived.** Generating "Kill three timber
wolves" from `wolf`'s id would need `combat::MonsterCatalog::find` (and the
equivalent for `world::World`/`timeline::Timeline` for the other two
kinds) — cross-module lookups `quest::Objective` deliberately doesn't have
access to. Writing the label directly in `data/quests.txt` costs nothing
and matches `PRESENCE`'s own trailing-flavor-text shape.

## DELIVER

The last objective kind deferred since the engine first shipped (see
"Deliberately not in v1" below, and Milestone 51's original scoping) — a
real, carried quest item, not a query over state the game already tracked
for unrelated reasons the way `VISIT`/`TALK`/`SLAY` are. Two independently-
existing pieces of state make it work, deliberately not a special link
between them:

- **The item's origin**: a zone POI's `GRANTS_ITEM <char> <item-id>
  <display-name...>` line (see `docs/ZONE_NOTES.md`) hands over a
  `character::ItemKind::QuestItem` the first time that POI is talked to —
  the exact same mechanism `BOAT` already established for
  `GameState::hasBoat` (Milestone 36), just granting an inventory item
  instead of flipping a flag. Same "must already have a TALK line"
  validation, same "granted once, never again" guard.
- **The quest that wants it**: `DELIVER <item-id> <count> <label>`'s
  `objectiveProgress` (`game::objectiveProgress` in `GameLoop.cpp`) counts
  matching `QuestItem` entries in `character::Character::inventory` — a
  live read, not a counter, same as every other kind.

A `QuestItem` is a real `character::ItemKind` (`Equipment.h`), carrying its
own `questItemId`/`questItemName` rather than looking either up from a
fixed table the way `ArmorId` does — the same "id + free display text"
shape `InventoryItem::weaponName` already has for a `Weapon`, since quest
items are one-off narrative objects, not a small closed catalog. It is
never equippable (`equipInventoryItem` no-ops on it, same as
`Potion`/`Webnet`/`BroochOfImog`) and never sellable (`sellableItems`
marks it unsellable with no resale value, same "never had an established
price" treatment as `SolamnicArmor`). Turning in a quest with a `Deliver`
objective removes `count` matching items from inventory as part of the
turn-in — the literal "handing it over" — via a new
`character::findQuestItemIndex` helper, the same shape as
`firstPotionIndex`/`firstWebnetIndex`.

Save format: a new, purely additive inventory-entry keyword,
`QUESTITEM <item-id> <display-name...>`, alongside `ARMOR`/`SHIELD`/
`POTION`/etc. in the same per-entry dispatch (see "Save format" below) —
not a widened enum bound, since `ItemKind` was never itself a raw
serialized int (each kind gets its own keyword).

### Shipped: `ore_for_the_forge`

The first (and so far only) `DELIVER` quest, proving the mechanism with
real content rather than landing pure engine work with nothing to point
at — same discipline `road_wolves` established for the quest engine
itself. **Source** — `data/zones/pax_tharkas.txt`'s new POI `O`, "An Ore
Cart" (an abandoned cart near the Tharkadan Mine gate, grounded in the
zone's existing "war ... over who controls what's dug from it" flavor;
the mine's interior stays unmodeled, same restraint the zone file's own
header comment already documents), grants `raw_tharkadan_ore`. Deliberately
its own POI rather than added to `M` (the Mine Entrance) — `M` is this
zone's `TIMELINE_ANCHOR`, and no zone shipped so far combines a
`TIMELINE_ANCHOR` tile with its own zone-native `TALK` line (anchors are
always pure scenery), so the grant was kept on a separate tile rather than
exercising that untested combination for a first pass. **Sink** —
`data/zones/solace.txt`'s `POI S`, "Flint's Smithy," previously pure
scenery, gains a `TALK S` for an unnamed journeyman who keeps the forge
running — written evergreen, deliberately never claiming to *be* Flint
(whose own tracked schedule may have him elsewhere, or already dead,
depending on the game day — see `docs/TIMELINE_NOTES.md`), asking for ore
hauled from Pax Tharkas since the mine's supply has dried up amid the
dispute over who holds it. `QUEST S ore_for_the_forge`, a single
`DELIVER raw_tharkadan_ore 1` objective, 35 steel / 80 XP reward (the
`inn_supply_run`/`bazaar_road_raiders` range, not the Knight-advancement
tier). Verified via a throwaway self-test (`QuestLoader` parsing the real
`DELIVER` line plus a malformed-line failure case; `ZoneLoader` parsing
`GRANTS_ITEM` plus its "no TALK line" failure case; the real edited zone
files loading clean; `findQuestItemIndex`/`inventoryItemLabel`/
`sellableItems` on a constructed inventory; a `SaveGame` round-trip
covering `QUESTITEM`), a clean `/W4` rebuild, a direct check that the
user's real `save.txt` still loads cleanly under the new inventory
format, and the standard piped smoke test. **Interactively verified**
(2026-08-20) by the user, including out-of-order pickup: they grabbed
the ore from the Ore Cart before ever talking to the journeyman, and the
quest still turned in cleanly on first contact — confirming the design
intent that the grant and the quest wanting it are independent state,
not a special-cased link (see "DELIVER" above).

**A second payoff added later (per-location shop wares milestone)**:
completing `ore_for_the_forge` now also unlocks Flint's Smithy itself as
a real shop (`SHOP S armory` + `SHOP_LOCKED S ore_for_the_forge` in
`data/zones/solace.txt`) — the first quest reward that gates world state
(a shop opening) rather than granting steel/XP/an item/a title. See
`docs/ZONE_NOTES.md`'s "SHOP_LOCKED" and `docs/CHARACTER_NOTES.md`'s "Six
shops, six catalogs". The quest's own `REWARD_STEEL`/`REWARD_XP` are
unchanged — the shop unlock is a `game::GameLoop::handleShop`-side check
against `QuestStatus::Complete`, not a new `quests.txt` reward keyword.

## The met-id trap

`TALK <met-id>` does **not** take a character's display name or a
zone/POI's own identity directly — it takes whatever string
`GameState::metCharacters` actually stores for that character, which is
one of two shapes:

- A canon (timeline) character's own stable id — `tanis`, `flint`, etc.
  (the same id `data/timeline.txt`'s `CHARACTER <id>` line declares).
- A zone-native NPC's synthesized `"<zoneId>:<POI char>"` id — `haven:G`
  for Haven's Seeker Guard, `solace_inn:O` for Otik.

Get this wrong (a display name, a bare POI char, a typo) and the objective
parses cleanly, the quest loads without error, and it simply **never
completes** — `QuestLoader` cannot see `data/zones/*.txt` or
`data/timeline.txt` to catch this at load time, the same silent-failure
class as an unvalidated `PRESENCE` location id (see
`docs/TIMELINE_NOTES.md`). Check the exact id in the relevant zone file or
`data/timeline.txt` before authoring a `TALK` objective.

## Zone binding

Covered in full in `docs/ZONE_NOTES.md`'s "Quests: POIs that offer them" —
short version: `QUEST <char> <quest-id>` on a zone POI that already has a
`TALK` line marks it as that quest's giver. The quest id itself is
cross-validated against the loaded `QuestCatalog` in `main.cpp` (the
established place for cross-loader checks this project already uses for
the saved `ZONE` id and starting-location grid bounds) — `ZoneLoader`
can't see `QuestCatalog` and shouldn't (`world/` stays decoupled from
everything above it in the dependency order — see `docs/ARCHITECTURE.md`).

At most one quest per POI in v1 — see "Deliberately not in v1" below.

## Turn-in flow

Hooked into `GameLoop::talkTo` in the same "talking mutates state" slot
the `grantsBoat` precedent (Milestone 36) established: after
`metCharacters.insert(id)`, before the topic-picker loop. `talkTo`'s own
greeting/`TALK_AGAIN`/`SAY_IF` text always plays first; `offerOrTurnInQuest`
runs after, so the quest beat reads as something the NPC brings up once
the ordinary greeting is done.

| `GameState::quests[id]` | `REQUIRE` | Behavior |
|---|---|---|
| absent (not started) | unmet | nothing — this quest-giver has nothing to say to this character |
| absent (not started) | met or none | `OFFER` text, then an Accept/Decline picker |
| `Active` | — | `PROGRESS` text |
| `ReadyToTurnIn` | — | `COMPLETE` text, rewards dispensed, status set to `Complete` |
| `Complete` | — | nothing extra — the ordinary greeting/again line already played |

Accepting is the one place this goes beyond the `grantsBoat` precedent (a
silent one-way flag flip): a quest shows up in a journal and reads as a
commitment, so it gets an explicit Accept/Decline picker
(`render::MapRenderer::drawPickerFrame`, same shape `pickAndTalk`/topic
menus already use). Declining leaves nothing recorded — the quest is
offered again next time the giver is talked to.

Note `Active` here means *not yet ready* — by the time `talkTo` reaches
`offerOrTurnInQuest`, `checkQuestReadiness` (see below) has already
promoted anything actually finished to `ReadyToTurnIn`, so
`offerOrTurnInQuest` itself never has to (and no longer does)
recompute `allObjectivesMet` live.

## Proactive readiness notification

Added shortly after the engine first shipped, in response to real
playtesting: turning in a quest used to require the player to guess they
were done and go check by walking back to the giver, or open the journal
speculatively. Now the moment every objective is satisfied, the game says
so — a log line naming the quest and where to collect the reward, e.g.
*"The Wolves on the Solace Road is ready to turn in -- return to the
Notice Board to collect your reward."*

This needed a third `QuestStatus` value, appended (not inserted) so old
saves keep meaning what they meant:

```cpp
enum class QuestStatus {
    Active = 0,
    Complete = 1,
    ReadyToTurnIn = 2,  // added after Active/Complete already shipped
};
```

`GameLoop::checkQuestReadiness()` walks every `Active` entry in
`GameState::quests`, checks `allObjectivesMet`, and promotes/logs once for
any that just became satisfiable — status flipping away from `Active` is
what makes this naturally one-shot; nothing re-checks an already-
`ReadyToTurnIn` quest, so killing a fourth wolf after the third doesn't
repeat the message. It's called from exactly **four** places, not a
generic subscriber system: right after `visitedLocations.insert` in
`tryMoveOverworld` (a `VISIT` objective), right after
`metCharacters.insert` in `talkTo` (a `TALK` objective), right after
`monsterKills[...] += 1` in `runCombat` (a `SLAY` objective), and right
after accepting a quest in `offerOrTurnInQuest` (catching the "already did
it before being asked" case from "Objectives are queries over existing
state" above — without this fourth call, a quest accepted in an
already-satisfied state would sit at `Active` forever, since nothing else
would ever re-check it). See `docs/ARCHITECTURE.md`'s "Quest system"
section for why this is a small, enumerable poll-at-known-mutation-points
pattern rather than the event bus this project has deliberately never
built.

The journal reflects the new state too: a `ReadyToTurnIn` quest is
grouped with active quests (not yet "Completed"), with a
"Ready to turn in! Return to `<giver>`." note above its (now
all-`[x]`) objective lines.

Turning in is also the **first call site of `character::applyPendingLevelUps`
outside `GameLoop::runCombat`** — a big XP reward can level the player up
mid-conversation. Its messages go to the persistent scrolling log (`log_`),
not the dialogue box that just closed, same as everywhere else level-up
flavor is surfaced.

## The journal (`g`)

New key, bound to `'g'`/`'G'` in both of `render::Console::readKey`'s
switches (Windows and the non-Windows fallback). **Not `'j'`** — that's
already South in the `hjkl` movement scheme, and `'q'`/`'l'` are
Quit/East, so every other single-letter mnemonic was already taken; same
kind of collision `'z'` (not `'b'`) resolved for bed rest at Milestone 41.
See `docs/GOTCHAS.md`.

`GameLoop::showJournal` is a one-keypress-block screen
(`render::MapRenderer::drawJournalFrame`), same shape as
`showCharacterSheet`/`showHelp` — not a nested loop, no scrolling. Lists
every quest in `GameState::quests` (active first, then a "Completed"
section), each objective line pre-marked `[x]`/`[ ] (n/count)` by
`GameLoop`, never by `MapRenderer` itself — `render/` stays as ignorant of
`quest::Quest` as it is of `timeline::PresenceWindow`; it only ever
receives a fully-resolved `JournalEntry`.

No scrolling in v1 — a handful of quests fits; `drawInventoryFrame` has
had the identical unbounded-height limitation since it shipped, and
nothing has needed fixing it yet.

## Save format

Two new repeated keywords, one line per entry, no count prefix (unlike
`VISITED`/`MET`, which pack bare ids onto one line — these are id+int
pairs, and packing pairs onto one line would need its own delimiter):

```
QUEST <quest-id> <statusInt>    0 = Active, 1 = Complete, 2 = ReadyToTurnIn
KILL <monster-id> <count>
```

A third keyword, `QUESTITEM <item-id> <display-name...>`, was added with
DELIVER — but unlike the two above it's not a new top-level line, just a
new per-entry keyword inside the existing `INVENTORY` block (alongside
`ARMOR`/`SHIELD`/`POTION`/etc. — see `character::ItemKind::QuestItem` and
"DELIVER" above). Purely additive: `ItemKind` was never itself a raw
serialized int (each kind gets its own keyword, not a shared `ITEMKIND
<n>` line), so no bound-widening or backward-compatibility shim was
needed the way `ARMOR`/`KNIGHTORDER`'s bound-widening has needed before.

Written immediately after `MET` (Milestone 92's `VOYAGED` line now sits
between them), before `ZONE`/`ZONESTACK` — `SaveGame.cpp` documents that
`load()`'s `ZONESTACK` consumption relies on `save()` always writing
`ZONESTACK` last with nothing after it, so anything new has to land
earlier than that, not appended at the end.

Both lines are optional on load, defaulting to empty maps — exactly "no
quest started, nothing killed yet," which is the correct read for every
save written before this milestone. Verified directly: a throwaway
self-test round-tripped `quests`/`monsterKills` through `save()`/`load()`,
confirmed a pre-Milestone-51-shaped save (no `QUEST`/`KILL` lines at all)
still loads with empty maps, and confirmed a scratch copy of the actual
in-progress `save.txt` from this repo loads clean under the new code.

`QuestStatus` is stored as a raw enum int, the same convention
`RACE`/`CLASS`/`ALIGNMENT` use — but append-only-safe here, unlike those:
nothing depends on a fixed ordering among quest statuses, so a third value
could be added later without corrupting old saves the way reordering
`RaceId` would.

**Still no arbitrary stage index.** `ReadyToTurnIn` (see "Proactive
readiness notification" above) is a real third value, added when a real
need showed up — proof the append-only design worked exactly as planned,
not evidence the door is now open for a general stage counter. The save
line (`QUEST <id> <int>`) would look identical either way, but a
freestanding multi-stage quest is still deferred until something actually
needs one.

## Shipped quests

### A third DELIVER quest: `what_the_tide_kept`, at Port O'Call

A user-requested quest content pass, but the "unforced hook" method
`reason_worth_giving`'s sweep used (check every `TALK`-having POI's
existing dialogue for an unmined thread) came back genuinely dry on
re-check: every zone either already has a quest, was already
checked-and-rejected in that sweep (Crossing, Palanthas's Knight/
Astinus, Sancrist's Embarkation Officer, Flotsam, Port Balifor, Tarsis's
Sailor/Runner, Pax Tharkas's Fortress Guard, Qualinesti's Elven
Sentinel, Neraka's Deserting Guard), or has zero talkable NPCs at all
(Dargaard Keep, Foghaven Vale, Godshome, Hopeful Vale, Mount Nevermind,
Qualimori, Que-shu — deliberately pure scenery/ruin/sacred-site zones).
The one zone added since that sweep (Port O'Call, Milestone 122) has
only a Dockmaster with the same deliberately-mundane, no-thread
dialogue as every rejected boat-logistics NPC.

Put to the user directly: accept the well is dry, or add one new small
NPC/hook the way `ore_for_the_forge`'s journeyman and Southern Ergoth's
Kaganesti Lookout both did. They chose the latter. Rather than inventing
a genuinely new character, this gives voice to one that was already
half-there: Port O'Call's Beachcomber's Stall (`data/zones/
port_ocall.txt` POI `B`) was a non-talkable shop-only POI whose own
description already implied both a voice and a hook ("the old woman
minding it swears a few pieces still carry a working charm, salvaged
off ships that never made land") — nothing new invented, just paid off.

**Sink** — POI `B` gains `TALK`/`TALK_AGAIN` (her salt-worn,
superstitious voice, straight from her existing description) and
`QUEST B what_the_tide_kept`. `SHOP B magic` is untouched and stays
open unconditionally — unlike Flint's Smithy's `SHOP_LOCKED`, she's
already an active shop, not scenery-until-now, so there's no reason to
gate her ordinary trade behind the quest. **Source** — a new POI, `W`
"The Storm-Wrack" (driftwood/wreckage, matching the town's own "what the
strait washes up" flavor), with `TALK W`/`TALK_AGAIN W` (the same "you
dig through it and come away with..." phrasing `ore_for_the_forge`'s Ore
Cart established) and `GRANTS_ITEM W drowned_sailors_locket A Drowned
Sailor's Locket`. `QUEST what_the_tide_kept` (`data/quests.txt`): one
`DELIVER drowned_sailors_locket 1` objective, no `REQUIRE` (broadly
offered, matching `ore_for_the_forge`/`seed_for_thorbardin`'s
precedent), `REWARD_STEEL 35`/`REWARD_XP 80` — the same tier as both
prior DELIVER-only quests. Port O'Call has no `PRESENCE`/
`TIMELINE_ANCHOR` (map-only, like Crossing), so the stricter
novel-citation bar doesn't apply here — same footing Crossing's Ferry
Keeper and Plains of Dust's Rider already have. Zero `.cpp`/`.h`
changes — pure data content, same as every prior DELIVER quest.

Verified via a throwaway self-test (`QuestLoader` parsing the real,
now-18-quest `data/quests.txt` confirming the new quest's shape;
`ZoneLoader` parsing the edited `port_ocall.txt`, confirming the new `W`
POI, `B`'s new `TALK`/`QUEST` lines, and the `GRANTS_ITEM` all parse
correctly), a clean `/W4` rebuild (zero new warnings, no source
changes), and the piped character-creation smoke test (real `save1.txt`
moved aside, restored after) confirming `main.cpp`'s cross-validation
accepts the new `QUEST B what_the_tide_kept` zone binding. Interactive
verification (talking to the Beachcomber, finding the Storm-Wrack,
delivering the locket, confirming the reward and journal entry) still
needs the user's own keyboard, the same `_getch()` limitation flagged
for every quest milestone so far.

### A fourth DELIVER quest: `a_widows_due`, at Kalaman — the give-a-voice move, proven a second time

The user asked for another pass at exactly the move `what_the_tide_kept`
proved out: give voice to an already-described-but-silent POI rather
than invent a wholly new character. A fresh scan of every `data/
zones/*.txt` file for POIs with a description but no `TALK` line, this
time not restricted to zones added since the last sweep, turned up
mostly deliberately-mute scenery (ruins, sacred sites) or deliberately
off-stage political seats (Kalaman's own Lord's Keep; Qualimori's
Speaker's House, which is also a `TIMELINE_ANCHOR`, same reasoning as
Sancrist's Great Hall staying closed). One real standout: Kalaman's
Curiosities Cart (`data/zones/kalaman.txt` POI `P`), shop-only since
Milestone 141's equipment-expansion pass, whose own description already
names a specific person and a specific backstory ("a war widow's trade,
picked up piece by piece from refugees who needed steel more than
keepsakes") — nothing new invented, just paid off, same as the
Beachcomber's Stall.

The hook ties into dialogue this same zone already shipped rather than
inventing new lore: the Kalaman City Watchman (POI `G`, `bazaar_
road_raiders`'s own giver) already complains, in his existing `TALK`
line, about "light fingers" he can never catch in the bazaar. The widow
lost her late husband's wedding band to exactly that; a trader working
the market's fringes has it now.

**Sink** — POI `P` gains `TALK`/`TALK_AGAIN` (her cagey, transactional
voice, straight from her existing description), two `SUBJECT` entries
(cart/wares/authenticity; refugees/war/widow, the latter also surfacing
the husband detail the `OFFER` draws on), `SUBJECT_UNKNOWN`, and `QUEST P
a_widows_due`. `SHOP P magic` is untouched and stays open
unconditionally, same reasoning as the Beachcomber's Stall — she's
already an active shop, not scenery-until-now. **Source** — a new POI,
`F` "A Furtive Trader" (someone working the market's fringes in goods of
uncertain origin, matching the Watchman's own complaint), with `TALK
F`/`TALK_AGAIN F` (the same "you spot it among his wares and it's gone
before he notices" register `ore_for_the_forge`'s Ore Cart and `what_
the_tide_kept`'s Storm-Wrack both established) and `GRANTS_ITEM F
soldiers_wedding_band A Soldier's Wedding Band` — deliberately distinct
item name from `what_the_tide_kept`'s `drowned_sailors_locket`. `QUEST
a_widows_due` (`data/quests.txt`): one `DELIVER soldiers_wedding_band 1`
objective, no `REQUIRE`, `REWARD_STEEL 35`/`REWARD_XP 80` — the same
tier as every prior DELIVER-only quest. Kalaman already carries a
`TIMELINE_ANCHOR` (on POI `M`, unrelated to either `F` or `P`), so unlike
Port O'Call/Plains of Dust this zone *is* novel-anchored — but the widow
and the trader are original, invented-not-sourced characters, same
footing as `ore_for_the_forge`'s journeyman or `reason_worth_giving`'s
Rider: the stricter novel-citation bar applies to the tracked Heroes'
own timeline content, not to every NPC standing in an anchored zone.
Zero `.cpp`/`.h` changes — pure data content, same as every prior DELIVER
quest.

Verified with a clean `/W4` rebuild (zero new warnings, no source
changed) and the piped character-creation smoke test (real `save1.txt`
left alone — an empty slot was used instead) confirming `ZoneCatalog` and
`QuestCatalog`'s fail-fast loaders parse the edited `kalaman.txt` and the
new `QUEST a_widows_due` block cleanly end-to-end. **Not yet
interactively walked** — same standing `_getch()` limitation as every
quest milestone so far; see `docs/CURRENT_WORK.md` for the concrete next
playtest step.

### `wayreth_summons`, in Solace — the Test of High Sorcery

Requested directly by the user: "the Towers of High Sorcery need to be
added." The Tower of Palanthas was already in the game (Milestone 44,
`data/zones/palanthas.txt` `POI T`, sealed and non-enterable). The Tower
of Wayreth was the real gap, and it comes with a genuine sourcing
constraint: this project's own already-shipped Raistlin dialogue
(`data/timeline.txt`, Milestone 72) says the Tower "does not stay where it
was the day before... finds you rather than the reverse" —
`References/TSR 2143 PG1 Players Guide to the Dragonlance Campaign.pdf`
confirms this as real sourced lore (an NPC there says outright "Not even I
could find the Tower of Wayreth"). Put to the user directly: how to
reconcile "add it" with "it can't be found." Their answer -- unfindable
for everyone except a Mage, and even then reachable only through a quest
-- is what this quest implements.

Deliberately **no new `LOCATION`/zone file**. A persistent walkable
Wayreth zone would need overworld coordinates, which either breaks
"unfindable" (reachable on foot, by anyone) or risks a real softlock
(unlike the SEA_LOCKED islands, each of which has its own return-boat POI,
Wayreth has no physical place to put a return trip's arrival point).
Instead the whole visit is a scripted round trip bundled entirely into
this one quest's Accept→Complete flow -- narratively, not mechanically, a
voyage.

Offered by a new POI, "A Robed Stranger" (`data/zones/solace.txt`, `POI
R`), gated by a new compound condition `wayreth_eligible`
(`game::conditionMatches`, `GameLoop.cpp`) -- `charClass == Mage && level
>= 3`, the same shape as `sword_eligible`. An unmet `REQUIRE` means the
Stranger has nothing to say about the quest at all, so no other class, and
no Mage below level 3, ever sees a hint Wayreth exists; their base
`TALK`/`TALK_AGAIN` text is deliberately mundane. One objective, `VISIT
palanthas`, reuses already-shipped content for free rather than inventing
a fetch/kill hook that wouldn't suit a mystical summons: the Great
Library's existing `SAY_IF L mage` Test-of-High-Sorcery flavor, and
`.research/dwn_full.txt` (~line 3648)'s own detail that the Towers'
surviving spellbooks were given to "the great library at Palanthas" -- a
real, sourced reason a mage would go there first.

A new reward flag, `REWARD_WAYRETH_ROBE` (bare, same "named, specific,
compile-time flag" shape as `REWARD_KNIGHT_SWORD`), does the real work on
turn-in: assigns `character::RobeColor` by alignment and narrates one of
three White/Red/Black outcome passages. Those three passages aren't new
prose -- they're Milestone 103's own text, **relocated verbatim** from
`character::applyPendingLevelUps` (`Leveling.cpp`) into `GameLoop::
offerOrTurnInQuest`, because a quest's `COMPLETE` field is fixed text and
can't branch by alignment the way this needed to. `Leveling.cpp`'s level-3
Mage branch now only foreshadows ("You feel, faintly, that something has
taken notice of you") instead of resolving the Test outright;
`character.robeColor` stays `RobeColor::None` (already its default, and
already hidden from the character sheet in that state, `MapRenderer.cpp`)
until this quest actually completes -- consistent with the source
material, since not every mage takes the Test the moment they're able to.
See `docs/CHARACTER_NOTES.md`'s "The Wayreth quest" for the full writeup.

Verified via the piped smoke test (confirms `data/quests.txt`'s new block
and `data/zones/solace.txt`'s new `POI R`/`QUEST R wayreth_summons`
binding all parse cleanly, and `main.cpp`'s cross-validation accepts it) and
a clean `/W4` rebuild (zero new warnings). Interactive verification --
reaching level 3 as a Mage, confirming the Stranger stays silent below
that threshold, accepting the quest, satisfying `VISIT palanthas`, turning
in, and confirming the Robe/outcome text matches the character's alignment
-- still needs the user's own keyboard, the same `_getch()` limitation
flagged for every quest milestone so far, and no Mage save currently
exists to test with.

**Redone the same session it shipped elsewhere in this doc's history --
see below**: the original shape above was a single `VISIT palanthas`
objective, no combat, nothing standing between accepting the quest and
turning it in but a walk. Called out directly by the user as reading like
"a standard fetch quest," which it was, despite the framing.
`.research/dla_full.txt` (OCR of *Dragonlance Adventures* pp.34-35, "The
Test of High Sorcery" -- column-garbled but legible around line 2094) was
already the citation behind this quest's design but hadn't actually been
used in full; it lists five real guidelines for what a Test should
contain: "at least three tests of the wizard's knowledge of magic," "the
casting of all of the spells known to the initiate," "at least three
tests that cannot be solved by magic alone," "at least one combat against
a character known to the initiate as an ally," and "at least one solo
combat against an opponent who is two levels higher than the initiate" --
plus the framing line "no one who comes is guaranteed of returning
alive." Milestone 103's three alignment-branched outcome passages
(`GameLoop.cpp`'s `REWARD_WAYRETH_ROBE` switch, unchanged by this pass)
already gesture narratively at repeated trials and an ally-combat beat
("falls apart... more than once," "every trial the Conclave sets you,"
"the illusion finally puts someone you'd call a friend between you...").
The solo-combat guideline was the one with zero representation, mechanical
or narrative, so it's the one this redo closes -- not by inventing a
multi-stage/branching quest engine (this file has repeatedly and
deliberately deferred a stage-index system; see "Still no arbitrary stage
index" above), but by adding a second objective to the existing bag, the
same "mix objective kinds in one quest" pattern `named_in_fact` (VISIT +
SLAY baaz) and `measure_of_roses` (VISIT + SLAY ogre) already established.

The new objective is `SLAY wight 1` (`data/monsters.txt`) -- unused by any
quest before this, no `TERRAIN_BIAS` so it can turn up anywhere at least
25 tiles from a town (matching "the trial finds you" over "go stand in a
specific biome," the same lore this quest's own offer text already leans
on -- Raistlin's shipped dialogue that the Tower "finds you rather than
the reverse"), and at 1,400 XP markedly tougher than every other SLAY
target in the roster except Troll/Ettin -- deliberately the hardest solo
fight offered so far, matching the source's own framing that this fight
is not routine (softened in practice by this project's standing "knocked
out, not killed" combat rule, so no real permadeath risk, same as every
other SLAY quest). Its `DESC` ("a grave-cold shape... a hunger older than
its own death") reads as uncanny rather than mundane, so the quest's own
text frames it explicitly as the Test's own illusion/manifestation, not a
literal undead sentry guarding Wayreth -- avoids inventing unsourced lore,
and matches the existing outcome passages' own repeated "illusion"
language. `OFFER`/`PROGRESS`/`COMPLETE` text was lightly rewritten (not
replaced) to foreshadow and then acknowledge the fight; `ACCEPT`,
`REQUIRE wayreth_eligible`, the `VISIT palanthas` objective, and
`REWARD_WAYRETH_ROBE` are all unchanged. Reward raised from XP-only (150)
to `REWARD_STEEL 75`/`REWARD_XP 250` -- roughly matching/exceeding
`measure_of_roses`'s 100/250 tier since the Wight outweighs that quest's
Ogre, while keeping steel more modest than a pure combat quest would earn
since the Robe itself is still the real reward, the same "item/title is
the reward" precedent `solamnic_armor`/`staff_of_striking_curing`/
`frostreaver_salvage` already established.

Zero `.cpp`/`.h` changes, zero new `REQUIRE`/`REWARD` flags, zero
save-format changes for this `SLAY` pass specifically -- pure content.
`save3.txt` (Serath) already has this quest at `QuestStatus::Complete`
under the old one-objective shape -- that status is terminal and never
re-evaluated, so her save is unaffected, but she also can't be used to
test anything new here (the stranger has nothing further to say to a
`Complete` quest).

**Redone a second time, same session, after further pushback**: the
`SLAY wight 1` fight above closes one DLA guideline (the solo combat)
but the user correctly pointed out the deeper issue -- winning a fight
via ordinary combat RNG isn't what the book is actually testing, and
"what happens if the player falls" needed a real answer that wasn't
"knocked out, try again." Re-researched properly this time, since the
first pass leaned on `.research/dla_full.txt`'s column-garbled OCR
alone: `.research/dla_layout.txt` (~line 2094) and the fuller prose in
*Players Guide to the Dragonlance Campaign* (`References/pg.txt:5417-
5447`) both say the Test explicitly does **not** grade on declared
alignment -- "The wizards are less interested in the applicant's
alignment... than whether he will use the power of magic in a
responsible manner" (PG) -- yet each Robe's own "Minimum Requirements"
in DLA is a parallel-structured check on conduct *during* the Test:
passed "without having committed an act contrary to the laws of"
good/neutrality/evil, respectively. Both books also state flatly, twice
between them, that "failure means death" -- real, but incompatible with
this project's unbroken "knocked out, not killed" invariant
(`docs/COMBAT_NOTES.md`), so permadeath was never on the table; the
resolution had to reinterpret "failure" as something else.

The fix: `GameLoop::offerOrTurnInQuest`'s `q->rewardWayrethRobe` block
(`GameLoop.cpp`) now stages a real scene instead of reading a stat. Once
the Wight falls, it doesn't dissipate -- it reshapes into "a face you'd
trust with your back turned" (a new `drawDialogueFrame`), then a real
`drawPickerFrame` choice (three in-fiction options, no mechanical
labels, matching this project's diegetic-UI convention everywhere else)
decides the outcome, not `character.alignment`. This also folds in the
one other DLA guideline still unaddressed -- "at least one combat
against a character known to the initiate as an ally" -- into the same
scene, rather than needing a second mechanic. Whichever option the
player picks maps to a new small `game::EthicChoice` enum
(`Good`/`Neutral`/`Evil`, declared in `GameLoop.h` next to
`conditionMatches`, same "free function, directly unit-testable"
treatment) and a new free function, `game::withEthic`, that preserves
the character's Lawful/Neutral/Chaotic axis and replaces only the
Good/Neutral/Evil axis -- exact index arithmetic over `character::
Alignment`'s own declaration order (`index % 3` is the law/chaos
component, `index / 3` is the ethic group), no lookup table needed.

**Per the user's explicit decision** (asked directly, since it's a real
fork with a broad blast radius): the chosen ethic updates
`state_.character.alignment` itself, not just the Robe -- the first time
alignment mutates anywhere in this codebase after character creation.
Every other `good`/`evil` `REQUIRE` (`game::conditionMatches`) already
reads `c.alignment` live, so this ripples forward into every other
alignment-gated quest and NPC line for free, no extra wiring needed --
same "objectives are live queries over existing state" principle this
file leans on everywhere else. A `pushLog` line ("Your alignment
shifts...") only fires when the choice actually changes the stored
value, so picking the option that already matches the character's
alignment stays quiet (the outcome passage itself already carries that
narrative weight). The three White/Red/Black outcome passages
(`GameLoop.cpp`) are lightly re-threaded, not rewritten -- almost all of
Milestone 103's original wording survives; they're adjusted only to read
as direct continuations of the one reshaped-illusion scene instead of a
vague, independent "every trial" vignette each.

No changes to `data/quests.txt`, `wayreth_eligible`, the Robed Stranger
POI, `character::robeForAlignment`, or the save format (`ALIGNMENT`/
`ROBE` were already-persisted fields; this just adds a second runtime
mutation site for one of them). Verified via a clean `/W4` rebuild (zero
new warnings) and the piped smoke test; `withEthic`'s arithmetic was
checked by hand across all nine `Alignment` values crossed with all
three `EthicChoice` values rather than a dedicated compiled self-test --
`conditionMatches` and its neighbors in `GameLoop.cpp` have never been
isolated into a self-test target in this project's history either,
since `GameLoop.cpp`'s own dependency graph (world/combat/quest/render,
all of it) makes a truly minimal throwaway target impractical; past
milestones verify this class of logic by rebuild + direct read-through +
interactive play instead, same treatment here. The picker, the setup
scene, and the alignment-shift log line have **no piped-testable
surface at all** -- quest turn-in is well past character creation, where
`_getch()` stops being pipeable -- so seeing any of this still needs a
fresh level-3+ Mage and the user's own keyboard, the same limitation
flagged for every quest milestone so far, more acutely than usual here.

### `reason_worth_giving`, at the Plains of Dust

The Milestone 105 sweep's "the well is nearly dry" finding turned out to
have a gap in it: that sweep only lists Crossing, Palanthas, Sancrist,
Flotsam, Port Balifor, Tarsis, and Pax Tharkas as checked-and-rejected --
it never actually looked at Qualinesti's Elven Sentinel, Neraka's Deserting
Guard, or Plains of Dust's Rider. Re-checked directly this milestone: the
Sentinel's wariness is pure gatekeeping with no errand thread, and the
Deserting Guard's whole characterization is "wants nothing to do with
anyone, mid-desertion" -- neither holds up. The Rider does. Her own
already-shipped `TALK`/`TALK_AGAIN` lines (`data/zones/plains_of_dust.txt`)
carry an explicit, unresolved "prove yourself, and you'll be welcome" arc
("You've got neither [escort nor reason]..." / "You still haven't given me
a good reason, but you also haven't caused trouble, so I suppose that's
reason enough to let you sit a while") that this quest finally pays off.

Plains of Dust is this project's one deliberately *invented* zone (see
`docs/ZONE_NOTES.md` -- an original, unnamed Plainsfolk tribe stands in for
the real Que-Shu, which DL3 confirms is already destroyed by this story's
time), so unlike every other zone's quest content, this needed no novel
citation -- the whole zone already runs on invented-but-flagged tone, not a
transcribed scene, same footing Crossing's Ferry Keeper already has.

The shape: the tribe's burial mounds have been disturbed, and putting the
dead back to rest is the "reason" her dialogue already gestures at. Uses
**Ghoul** (`data/monsters.txt`), the one Monster-Manual-sourced monster no
quest had used yet -- its own `DESC` ("rises from a shallow grave") is a
direct, literal fit for disturbed-mound flavor, and steppe-nomad ancestral
burial-ground reverence is a real, non-infringing cultural texture rather
than invented Dragonlance lore. Also deliberately not a fourth Wolf/Goblin/
Hobgoblin reskin, same repetition the Milestone 105 pass avoided by picking
Gnoll. No `REQUIRE` -- broadly offered, same as `road_wolves`/
`what_the_stones_remember`.

Two content additions beyond the `QUEST R reason_worth_giving` binding
itself (added under the Rider's existing `TALK`/`SUBJECT` block, no new
`TALK` needed): a new flavor-only POI, `M "The Old Mounds"`, giving the
quest a physical anchor the same way Darken Wood's pre-existing Old Ruins
grounded `what_the_stones_remember`; and a new `SUBJECT R
mounds,graves,dead,barrows` entry tying the Rider's "ask about anything"
pool to the new thread. `SLAY ghoul 2`, 40 steel / 90 XP -- mid-tier,
between `bazaar_road_raiders` (2 tougher Hobgoblins) and `new_faces_on_the_
road` (3 weaker Gnolls), matching a Ghoul's own higher per-kill XP (175
each) against a 2-kill count. Verified via a throwaway self-test
(`QuestLoader` against the real, now-16-quest `data/quests.txt` confirming
`reason_worth_giving`'s shape; `ZoneLoader` parsing the edited `plains_of_
dust.txt`, confirming the new `M` POI, the `QUEST R reason_worth_giving`
binding, and the new `SUBJECT` all parse cleanly), a clean `/W4` rebuild (no
new warnings -- pure data, no `.cpp`/`.h` changes), and the piped smoke test
(confirms `main.cpp`'s cross-validation accepts the new zone binding).
Interactive verification -- accepting the quest, killing 2 Ghouls, turning
in, and confirming the new `SUBJECT`/POI read well in a real conversation --
still needs the user's own keyboard, the same `_getch()` limitation flagged
for every quest milestone so far.

### A three-quest content pass: `what_the_stones_remember`, `word_to_the_wilder_kin`, `new_faces_on_the_road`

A user-requested "more quests" pass, picked with the same "unforced hook"
method every prior quest-content milestone used: every `TALK`-having POI
across all 23 zone files was checked for a `QUEST` line, and the
quest-less ones cross-referenced against their own already-written
`TALK`/`TOPIC`/`SUBJECT` flavor text. Most turned out thin (Crossing's
Ferry Keeper, Palanthas's Knight/Astinus, Sancrist's Embarkation Officer,
Flotsam's dockhand, Port Balifor's innkeeper, Tarsis's Sailor/Runner, Pax
Tharkas's Fortress Guard) — real content, but nothing pointed at an
actionable objective without inventing new lore, matching this file's own
"Extending this later" note that remaining hooks are increasingly hard to
find. Three held up. All three are pure data content — no new `REQUIRE`
condition, no new reward flag, no `.cpp`/`.h` changes, no save-format
changes.

**`what_the_stones_remember`** — Darken Wood's Unicorn (`U`, canonically
the Forestmaster, DL1's guardian of the wood — see
`docs/ZONE_NOTES.md`'s Darken Wood section), whose own `TOPIC "Guardian
of the Wood"` already establishes her test-of-worth judgment ("what a
life has given more than it has taken"). Reframes that judgment into a
`SLAY owlbear 1` objective against whatever's denning in the zone's Old
Ruins POI, whose own description already reads "the ground nearby is
oddly bare, as if something is still keeping the forest back" — an
unexplained detail this quest pays off rather than a new one invented.
Owlbear (`data/monsters.txt`, forest `TERRAIN_BIAS`) was previously
quest-unused. No `REQUIRE` — the Unicorn already engages both good and
evil travelers (`SAY_IF good`/`SAY_IF evil`), just differently, so this
stays broadly offered like `road_wolves`. The `COMPLETE` text explains
away the "kept back" ground and has the player find a small, mundane
coin hoard among the disturbed stonework rather than having the Unicorn
hand over steel directly — 45 steel / 110 XP, above the 2-Hobgoblin
`bazaar_road_raiders` tier since a solo HD5+2 Owlbear outweighs that, but
below the item-granting quests' tier since nothing but steel/XP changes
hands. `QUEST U what_the_stones_remember` added under the existing `TALK
U` block in `data/zones/darken_wood.txt` — no new POI, no new dialogue
beyond the quest's own text.

**`word_to_the_wilder_kin`** — Southern Ergoth's Silvanesti Sentry (`S`),
whose own `TOPIC "Three Peoples, One Coast"` already admits real,
already-shipped discomfort about the Kaganesti/"Wilder Elves" sharing
this coast ("we call them primitive when we're being unkind, which is
more often than I'd like to admit"). This quest turns that admitted gap
into an actual `TALK` errand. The Wilder Elves' Camp POI (`G`) was
already placed on the map but is this zone's `TIMELINE_ANCHOR` — and
this project keeps anchor tiles pure scenery (no zone before this one
had combined a `TIMELINE_ANCHOR` with its own zone-native `TALK` line;
`ore_for_the_forge`, Milestone-era, explicitly avoided the combination
for the same reason) — so rather than making `G` itself talkable, a new
POI, `K` ("A Kaganesti Lookout"), was placed two tiles east of it on the
grid, the same "split a talkable NPC off a shared/scenery tile" treatment
`data/zones/solace_inn.txt`'s `O`/`Y` (Otik/Tika, split off the shared
`K "The Bar"`) already established. `K` gets the standard `TALK`/
`TALK_AGAIN`/two-`SUBJECT`/`SUBJECT_UNKNOWN` treatment as a new,
unnamed Kaganesti voice. Unlike `kin_beyond_the_border`, no `REQUIRE
elf` — the Sentry is speaking generally to "another one out of the
water," not gated by kinship, and a non-elf emissary crossing this
specific line is closer to the point. `TALK southern_ergoth:K` is the
objective (the met-id, not a display name — see "The met-id trap"
above). This is also the first POI in the project to carry both `QUEST`
and `BOAT` (the Sentry already grants `BOAT S sancrist_isle 60`) —
`GameLoop::talkTo` already runs quest-offer/turn-in and the boat offer as
two independent, sequential checks (see "Turn-in flow" above and the
boat-offer code right after it), so nothing needed to change; confirmed
by the throwaway self-test and the piped smoke test, not by new code.
Reward: 30 steel / 60 XP, matching `kin_beyond_the_border`/
`word_for_the_tower`'s TALK/VISIT-only tier.

**`new_faces_on_the_road`** — Haven's Seeker Guard (`G`), whose own
`SUBJECT war,dragons,army,armies` and `TALK_BEFORE` lines already read
"the roads are busier than they used to be, and the new faces aren't
pilgrims... it's only going to get worse before it gets better" — an
explicit "roads getting dangerous" hook that had never been paid off
with an actual objective. Haven was otherwise the only major town
without its own `SLAY`-type "clear the roads" quest (Solace, Solace Inn,
Kalaman, Ice Wall, and Xak Tsaroth all have one). `SLAY gnoll 3` reuses a
previously quest-unused monster rather than a fourth Wolf/Goblin/
Hobgoblin quest, and its Monster Manual flavor ("shoulders through the
brush, jaws slack with a wet, laughing snarl") suits "unwelcome new
faces" better than a straight reskin would. No `REQUIRE`, same
broadly-offered shape as `inn_supply_run`. Reward: 35 steel / 80 XP,
between `inn_supply_run` (30/70, 3 weaker Goblins) and
`bazaar_road_raiders` (45/100, 2 tougher Hobgoblins). `QUEST G
new_faces_on_the_road` added under the existing `TALK G` block in
`data/zones/haven.txt` — no new POI.

Verified via a throwaway self-test (`QuestLoader` against the real,
now-15-quest `data/quests.txt` confirming each new quest's requirement/
objective/reward shape; `ZoneLoader` parsing all three edited zone files,
confirming each new `QUEST <char> <quest-id>` binding, the new `K` POI's
grammar at `southern_ergoth.txt`, and that `G` there still has no
dialogue), a clean `/W4` rebuild (zero new warnings, no `.cpp`/`.h`
changes), and the piped smoke test (confirms `main.cpp`'s cross-validation
accepts all three new zone bindings against the loaded `QuestCatalog`).
Interactive verification — actually accepting/completing all three, and
confirming the Sentry's quest-then-boat sequence reads naturally in a real
conversation — still needs the user's own keyboard, the same `_getch()`
limitation flagged for every prior quest milestone.

### A second DELIVER quest: `seed_for_thorbardin`

The equipment-expansion milestone's other content addition, proving
`DELIVER` isn't a one-off after `ore_for_the_forge`. **Sink** --
`data/zones/thorbardin.txt`'s existing `R` POI (the displaced farmer in
the Refugee Quarter, present since Milestone 86), whose own dialogue
already reads "Come spring we're meant to try the mountainside for
crops... I don't know what we'll do the season after that" -- a
DELIVER hook that needed no new lore, just a `QUEST R
seed_for_thorbardin` line. **Source** -- a new POI at `data/zones/
haven.txt`, `F` ("A Farmer's Cart"), reframing the Seeker Guard's own
already-written "Farmers wanting rain blessed" line rather than inventing
a new NPC archetype; `TALK F`/`GRANTS_ITEM F hardy_seed_grain` follows the
exact "dig through it, granted once" phrasing `ore_for_the_forge`'s Ore
Cart established. No `REQUIRE` (matches `ore_for_the_forge`'s
broadly-offered precedent), one `DELIVER hardy_seed_grain 1` objective,
35 steel / 80 XP reward (identical to `ore_for_the_forge` -- the only
other DELIVER-only quest). Verified via a throwaway self-test
(`QuestLoader` parsing the real, now-twelve-quest `data/quests.txt`
including the new quest's shape; `ZoneLoader` parsing both edited zone
files and confirming the new POI/QUEST binding resolve correctly), a
clean `/W4` rebuild (zero new warnings, no `.cpp`/`.h` changes -- pure
data content), the piped smoke test, and a direct check that the user's
real save still loads cleanly (no save-format change was needed here).
Interactive verification (finding the Farmer's Cart, accepting and
completing the quest at Thorbardin) still needs the user's own keyboard,
the same `_getch()` limitation flagged for every quest milestone so far.

### Dragonlance magical items, continued: `frostreaver_salvage`

The third item-granting quest reward, following `staff_of_striking_curing`'s
exact shape. Picked from `docs/MILESTONES.md`'s NEXT UP list -- a real gap
the Dragonlance-magical-items milestone had already sourced (DLA p.94,
visually confirmed) and flagged as buildable but deferred in favor of the
Staff. Offered by Ice Wall's *existing* Young Knight POI (`K`, `data/zones/
ice_wall.txt`) rather than a new NPC -- he already carries established
Thanoi-flavor dialogue (`TALK K`/`TOPIC K "The Walrus-Men"`), and the
quest frames the axe as battlefield salvage ("an Ice Folk raider we found
dead near the wall, killed by thanoi, not us") rather than inventing a
talking Ice Folk character never actually placed at Ice Wall Castle in the
novel's own scene there -- the same restraint that's kept Alhana/Derek/
Gunthar off-stage (see `docs/TIMELINE_NOTES.md`). Gated by `REQUIRE
str_13` (`c.scores.strength >= character::kFrostreaverMinStrength`), a new
`game::conditionMatches` token and this project's first quest requirement
keyed on a raw ability score rather than race/class/knight-rank, so the
quest is never offered to a character who couldn't wield the reward
anyway. One `SLAY thanoi 2` objective -- Thanoi are tough (157 XP each,
`data/monsters.txt`), so 2 is calibrated down from the 3-kill baseline
weaker monsters use, matching `bazaar_road_raiders`'/
`staff_of_striking_curing`'s own 2-kill count against a comparably tough
target, and the Thanoi's own `TERRAIN_BIAS :` (glacier) means the fight
genuinely happens on the same terrain the axe's bonus is gated to. Reward:
50 steel, 120 XP -- same modest tier as `solamnic_armor`/
`staff_of_striking_curing`, since the item is the real reward -- plus a
new bare `REWARD_FROSTREAVER` quest flag granting a `character::
ItemKind::Weapon` InventoryItem named `character::kFrostreaverName` (1d8
base, `weaponMagicBonus = 0` -- its honest off-glacier baseline; see
`docs/CHARACTER_NOTES.md`'s "Magic items" for the terrain-gated +4
mechanism, this milestone's one genuinely new wrinkle beyond the
Solamnic-Armor/Staff pattern). `SaveGame.cpp` touches: none -- the
`MAGICWEAPON` line format is already fully generic over weapon name.
Verified via a throwaway self-test (`QuestLoader` parsing the real,
now-eleven-quest `data/quests.txt`, confirming `frostreaver_salvage`'s
requirement/objective/reward shape and `REWARD_FROSTREAVER`'s fail-fast
trailing-argument case), a clean `/W4` rebuild (zero new warnings), and
the piped smoke test (confirms `main.cpp`'s cross-validation accepts the
new `QUEST K frostreaver_salvage` zone binding). Interactive verification
(accepting the quest as a Strength-13+ character, killing 2 Thanoi,
turning in, equipping the Frostreaver, and confirming the +4 applies on a
glacier tile but not off it) still needs the user's own keyboard, the
same `_getch()` limitation flagged for every quest milestone so far --
and the one place this milestone most needs real playtesting, since the
terrain-gating is new, untested-by-precedent logic.

### Dragonlance magical items, continued: `staff_of_striking_curing`

The second item-granting quest reward, following `solamnic_armor`'s exact
shape. Offered by a new POI at `data/zones/xak_tsaroth.txt` — "A
Ruin-Scavenger" (`S`) — the zone's first talkable NPC (every prior POI
there is pure scenery, see `docs/ZONE_NOTES.md`). Gated by `REQUIRE
cleric`, matching the source's own "common among the clerics" framing (see
`docs/CHARACTER_NOTES.md`'s "Magic items"). One `SLAY skeleton 2`
objective — "clear whatever's nested in the well shaft" — grounded in the
zone's existing ruined/haunted flavor text rather than inventing new lore,
and matching the weight of a real small task the way every other
item-granting quest has one (not an instant handout). Turning it in sets
`REWARD_STAFF_OF_STRIKING_CURING`, granting a `character::ItemKind::Weapon`
InventoryItem named `character::kStaffOfStrikingCuringName` (+3 to-hit and
damage, 1d6 base — see `docs/CHARACTER_NOTES.md`'s "Magic items" for the
full sourcing, including why this project models the item's "curing" side
as a flat once-per-day self-heal rather than the book's 50-charge pool).
Reward: 40 steel, 100 XP — same tier as `solamnic_armor`, since both grant
a single standalone item rather than a rank. Verified via a throwaway
self-test (`ownsStaffOfStrikingCuring`/`staffCureAvailableToday`/
`useStaffCure`'s ownership check, once-per-day gate, and heal-cap
behavior; `QuestLoader` against the real, now-ten-quest `data/quests.txt`
including `REWARD_STAFF_OF_STRIKING_CURING`'s fail-fast trailing-argument
case; a `SaveGame` round-trip covering the new `STAFFCUREDAY` line and a
save with it absent defaulting to -1), a clean `/W4` rebuild, and a direct
piped run confirming the user's real save loads cleanly under the new
save format (see `docs/GOTCHAS.md`'s executable-relative `save.txt` note)
— this run also exercises every data loader including the new quest/zone
content, since it reaches the character-creation EOF-fail point cleanly.
Interactive verification (talking to the Scavenger as a Cleric, clearing
the skeletons, equipping the staff, and using its combat cure action)
still needs the user's own keyboard, the same `_getch()` limitation
flagged for every quest milestone so far.

### Order of the Rose advancement: `measure_of_roses`

The capstone of the Knights of Solamnia chain: Crown (character creation) →
Sword (Milestone 53's `named_in_fact`) → Rose. Offered by a new POI at
`data/zones/high_clerist_tower.txt` — "A Rose Knight" (`R`), on the same
Muster Yard row as `Y`/`S`/`L`, since that's already established as where
the Order convenes and `K`/`S`/`L` are all already spoken for. Gated by
`REQUIRE rose_eligible` (`c.knightOrder == character::KnightOrder::Sword &&
c.level >= 4 && character::meetsKnightOfRoseRequirements(c.scores)`), the
same compound-condition shape as `sword_eligible` — see
`docs/CHARACTER_NOTES.md`'s "Entry requirements for Knight of the Rose" for
the full sourcing, including a second book inconsistency this milestone's
research turned up (resolved via the Rose Knight Advancement Table itself,
which starts at level 4) alongside the p.18/p.19 Sword erratum Milestone 53
already found. Objectives mix `VISIT plains_of_dust` (reusing the same
"farthest mapped location from the Tower" target `named_in_fact` already
established — the book's 500-mile/30-day journey requirement is *identical*
text between Sword and Rose, so reusing the location is the honest choice)
and `SLAY ogre 1` (the book's "defeat of an evil opponent of equal or
higher level... without killing the foes" — Ogre is the highest-XP,
clearly-evil single monster in the roster, deliberately distinct from
Sword's Baaz duel; this project's standing "knocked out, not killed" combat
convention satisfies the no-killing clause for free, same as Sword's). The
book's other four elements (one test of wisdom, three of generosity, three
of compassion, restoring something lost) have no corresponding trackable
state, same as Sword's four — narrated in `COMPLETE` text only, the same
"narrated, not tracked" treatment. Turning it in sets `REWARD_KNIGHT_ROSE`,
promoting `knightOrder` to Rose; reward is 100 steel/250 XP, above Sword's
60/150 — Rose is the capstone rank. `SaveGame.cpp`'s `KNIGHTORDER` bound
moved from 3 to 4, append-only-safe, same precedent as the 2→3 move at
Milestone 53. A new `nextLevel == 4 && knightOrder == Sword` flavor line in
`Leveling.cpp` foreshadows eligibility, mirroring the existing level-3
Crown→Sword line. Verified via a throwaway self-test (ability-score
boundary cases for `meetsKnightOfRoseRequirements`, confirming Sword's own
minimums don't accidentally satisfy Rose's higher bar; `QuestLoader`
parsing the real, now-eight-quest `data/quests.txt` including
`REWARD_KNIGHT_ROSE`'s fail-fast trailing-argument case; a `SaveGame`
round-trip covering the widened `KNIGHTORDER` bound, plus confirming the
old bound's exclusion boundary — `KNIGHTORDER 4` — still fails to load), a
clean `/W4` rebuild, a direct check that the user's real save (the
executable-relative `save.txt` next to the built exe, not the stale
repo-root copy — see `docs/GOTCHAS.md`) still loads cleanly under the new
`KNIGHTORDER` bound, and the standard piped smoke test. Interactive
verification (reaching level 4 as a Sword Knight, confirming the Rose
Knight only offers the quest once eligible, completing the VISIT+SLAY mix)
still needs the user's own keyboard, the same `_getch()` limitation flagged
for every quest milestone so far.

### Dragonlance magical items: `solamnic_armor`

The follow-up to Milestone 53's Sword advancement, and this project's
first item-granting quest reward. Offered by a new POI at
`data/zones/high_clerist_tower.txt` — "A Knight of the Circle" (`L`),
placed near the existing Muster Yard (`Y`)/Sword Knight (`S`) cluster,
since `K` and `S` already carry a quest each and v1 allows only one per
POI. Gated by `REQUIRE sword_knight` (`c.knightOrder ==
character::KnightOrder::Sword`, a new single-fact token distinct from the
existing `knight` token, which also matches Crown) — the source book
(*Dragonlance Adventures*, TSR 2021, p.93) actually gates Solamnic Armor
on the title "Lord," which this project doesn't model, so this is scoped
to the already-shipped Sword rank instead: the Circle recognizing service
already proven, not a new rank tier. One `TALK high_clerist_tower:K`
objective (the Garrison Knight vouching for the candidate) — reuses an id
the player has almost certainly already recorded via `word_for_the_tower`
or ordinary exploration, so, per "Objectives are queries over existing
state" above, this quest can very plausibly be instantly completable the
moment it's accepted; the `OFFER`/`ACCEPT`/`PROGRESS` text is written to
read fine either way, same discipline `road_wolves` established. Turning
it in sets `REWARD_SOLAMNIC_ARMOR`, granting `character::ArmorId::
SolamnicArmor` (AC 0, sourced directly from the book, see
`docs/CHARACTER_NOTES.md`'s "Magic items") plus an ordinary Shield item —
a deliberate simplification of the book's separate "shield +1" (this
engine's shield has no enchantment tiers of its own). Reward: 40 steel,
100 XP. The same milestone also added a "+1" magic weapon per class, sold
at every shop alongside the mundane upgrade — pure equipment, no quest
involved; see `docs/CHARACTER_NOTES.md`'s "Magic items" for the full
sourcing (DLA's own "Magical Items of Krynn" chapter, DMG Table 109) and
why the chapter's unique named artifacts (Wyrmslayer, Staff of Magius,
the Hammer of Kharas) stay out of player reach. Verified via a throwaway
self-test (`magicWeaponFor` per class, shop-catalog shape/index
arithmetic, purchase/equip/sell round-trip including the newly-unsellable
`SolamnicArmor`, `resolvePlayerAttack`'s to-hit/damage magic-bonus wiring,
`QuestLoader` against the real seven-quest `data/quests.txt` including
`REWARD_SOLAMNIC_ARMOR`'s fail-fast trailing-argument case, and a save
round-trip covering the new `MAGICWEAPON` format, the legacy `WEAPON`
format still loading with `magicBonus == 0`, and the widened `ARMOR`
bound), a clean `/W4` rebuild, a direct check that the user's real
`save.txt` still loads cleanly under the new `ARMOR` bound and the
`WEAPON`/`MAGICWEAPON` dual-format parsing, and the standard piped smoke
test. Interactive verification (buying/equipping the magic weapon and
seeing the combat difference; reaching Sword and completing
`solamnic_armor` for the armor) still needs the user's own keyboard, the
same `_getch()` limitation flagged for every quest milestone so far.

### Milestone 53: Knight of the Sword advancement

`named_in_fact` ("In Fact as Well as Blood"), offered by a new POI at
`data/zones/high_clerist_tower.txt` — "A Sword Knight" (`S`), placed next
to the existing Muster Yard (`Y`, a `TIMELINE_ANCHOR`) whose own flavor
text already read "the Order still gathers whenever it has cause to name a
new Knight in fact as well as blood" (written well before this milestone)
— reframing an existing hook again, same discipline Milestone 52
established, not inventing a location from nothing. `K` ("A Garrison
Knight") already carries `word_for_the_tower`, and v1 allows only one
quest per POI, so this needed its own giver rather than reusing `K`.

Gated by `REQUIRE sword_eligible` (`docs/CHARACTER_NOTES.md`'s "Knights of
Solamnia" section has the full sourcing) — this project's first `REQUIRE`
condition that isn't a single-word fact lookup, since eligibility is a
compound check (Crown + level + ability scores) with no existing
single-token analog. Objectives are `VISIT plains_of_dust` (the farthest
mapped location from the Tower, standing in for the book's "journey of no
less than 500 miles and 30 days") and `SLAY baaz 1` (a Baaz Draconian,
already flavored in `data/monsters.txt` as "a draconian, blade drawn"
blocking the path — a natural single-combat duel, and this project's
combat is already "knocked out, not killed"
(`docs/COMBAT_NOTES.md`), which happens to satisfy the book's "victorious
... without necessarily killing" clause for free). This is this project's
first quest to mix two different objective kinds in one quest, proving the
grammar table's "any mix" claim in real content for the first time (every
Milestone 52 quest was single-objective).

The book's other four required quest elements (three tests of wisdom, one
of generosity, one of compassion, restoration of something lost) have no
corresponding trackable state — nothing in `GameState` counts "a test of
generosity" — so rather than inventing fake counters with no other use,
they're narrated in the `COMPLETE` text as things the Council is told
happened along the road, the same "narrated, not tracked" treatment
Milestone 47's "ridge farewell" gave lore beats with no mechanical hook.

Turning it in sets `REWARD_KNIGHT_SWORD` (`character.knightOrder =
KnightOrder::Sword`), which `character::knightOrderName` and the character
sheet (`MapRenderer.cpp`) already display generically — no renderer
change needed. Reward: 60 steel, 150 XP (above Milestone 52's 30-45/60-100
range — the real reward is the title). `SaveGame.cpp`'s `KNIGHTORDER`
bound moved from 2 to 3 values, append-only-safe (old saves' 0/1 stay
valid). Verified via a throwaway self-test (ability-score boundary cases
for `meetsKnightOfSwordRequirements`, and `QuestLoader` parsing the real
six-quest `data/quests.txt` including `REWARD_KNIGHT_SWORD` and its
fail-fast trailing-argument case), a clean `/W4` rebuild, and the piped
smoke test — plus, since this milestone touches the save format, a direct
check that the user's real `save.txt` (a level-1 Human Fighter) still
loads cleanly under the new `KNIGHTORDER` bound before the standard
move-aside/restore smoke test. Interactive verification — actually
reaching level 3 as a Crown Knight, confirming the Sword Knight only
offers the quest once eligible, and completing the `VISIT`+`SLAY` mix —
still needs the user's own keyboard, the same `_getch()` limitation
flagged for Milestones 51 and 52.

### Milestone 52: real content

Four more quests, deliberately picked to prove the objective kinds and
`REQUIRE` vocabulary `road_wolves` didn't exercise, not to pad out the
count — see "What ships" in the approved Milestone 52 plan for the full
reasoning:

| Quest id | Giver (zone:POI) | Objective kind(s) | `REQUIRE` |
|---|---|---|---|
| `inn_supply_run` | Otik, `solace_inn:O` | `SLAY goblin` x3 | — |
| `word_for_the_tower` | the Garrison Knight, `high_clerist_tower:K` | `VISIT palanthas` | — |
| `bazaar_road_raiders` | the City Watchman, `kalaman:G` | `SLAY hobgoblin` x2 | — |
| `kin_beyond_the_border` | the Silvanesti Warder, `silvanesti:W` | `TALK qualinesti:E` | `elf` |

`word_for_the_tower` and `kin_beyond_the_border` are this project's first
`VISIT`-only and `TALK`-only quests, played and turned in for real for the
first time (M51's `road_wolves` was deliberately `SLAY`-only — see below).
`kin_beyond_the_border` is also the first quest gated by a `REQUIRE`
condition other than `knight`, reusing `game::conditionMatches`'s existing
`elf` check with no engine changes. Every quest reframes a hook that
already existed in that POI's `TALK`/`TOPIC` flavor text (the Tower's
stalled Vingaard supplies, Kalaman's watchman worried about more than
pickpockets, Silvanesti's own established "we shut the gate even to our
Qualinesti kin" line from `TOPIC W "A Land That Keeps to Itself"`) rather
than inventing new lore. `DELIVER`/item objectives and Knight-of-the-Sword
advancement were both still out of scope for this pass — none of these
four needed an item-possession objective, and Sword advancement is a
big-enough unit of work (a new `KnightOrder` value, a new gating
condition) to warrant its own milestone. Verified the same way as M51: a
throwaway `QuestLoader` self-test against the real, five-quest
`data/quests.txt`, a clean `/W4` rebuild (zero `.cpp`/`.h` changes — pure
data content), and the piped character-creation smoke test (proves
`main.cpp`'s cross-validation accepts all four new `QUEST <char>
<quest-id>` zone bindings). Interactive verification — actually accepting
each quest, confirming `REQUIRE elf` really gates `kin_beyond_the_border`,
and watching the "ready to turn in" notification fire for a `VISIT`/`TALK`
quest for the first time — still needs the user's own keyboard, the same
`_getch()`-can't-be-piped limitation M51 flagged.

### Milestone 51: the `road_wolves` proof of concept

One quest shipped with the engine itself, to prove the pipeline end to end
rather than land pure mechanism with nothing to point at:
`data/quests.txt`'s `road_wolves`, offered by Solace's Notice Board
(`data/zones/solace.txt`'s `POI B`, see `docs/ZONE_NOTES.md`) — kill 3
`wolf` (Timber Wolf, `data/monsters.txt`), reward 40 steel + 90 XP. Chosen
deliberately as a `SLAY`-only quest: `VISIT`/`TALK` objectives just read
sets that already existed and were already exercised by every earlier
milestone, so the highest-value thing to prove live is the one genuinely
new piece — `monsterKills` actually being written by combat and read back
correctly at turn-in.

Verified via a throwaway self-test (`QuestLoader` against this real file,
plus save round-trip/backward-compat — see "Save format" above) and a
piped character-creation smoke test (proves `QuestCatalog` loads alongside
`World`/`ZoneCatalog`/`Timeline`/`MonsterCatalog` without throwing) before
ever reaching a human. Interactive keypresses genuinely can't be automated
for this project (`_getch()` can't be piped, no tmux/PTY driver exists for
this Windows console app — see `docs/GOTCHAS.md`), so that path was left
explicitly unverified at first release. **The user then played through it
for real**: their actual `save.txt` showed `MET solace:B` (talked to the
board), `QUEST road_wolves 0` (accepted) with a kobold kill logged, and
later `QUEST road_wolves 1` (turned in) with `KILL wolf 3` and the other
five monster types tallied alongside it — the whole offer/accept/track/
turn-in loop working exactly as designed, confirmed from the save file
alone without needing to watch it happen. That same playtest also
surfaced the request that became "Proactive readiness notification"
above, and separately a real crash bug (unrelated to quests — see
`docs/COMBAT_NOTES.md`'s "Bug fixed" section).

## SFML port (Milestone 183)

Everything above describes `GameLoop.cpp`/`ansalon_rpg`'s implementation.
`ansalon_sfml_phase1` ported all of it in Milestone 183 (2026-09-11),
found missing entirely — not just unconfirmed — by an audit run after the
user asked to reach real quest/mechanic parity before a final release.
`src/quest/Quest.cpp`/`QuestLoader.cpp` simply weren't in
`ansalon_sfml_phase1`'s `CMakeLists.txt` source list, so no
`quest::QuestCatalog` was ever constructed on that side at all — every
zone POI's `QUEST`/`SHOP_LOCKED` id silently went nowhere.

The port follows this migration's established pattern rather than
sharing code directly: `objectiveProgress`/`objectiveMet`/
`allObjectivesMet` and `EthicChoice`/`withEthic` (pure functions,
declared in `GameLoop.h`) are copied verbatim into `sfml_phase1/main.cpp`,
since `GameLoop.cpp` itself isn't a link target there (it depends on
`render::Console`). Turn-in/reward logic is reimplemented as new
`DialogueUiState` values (`QuestOfferText`, `QuestAcceptDecline`,
`QuestAcceptText`, `QuestProgressText`, `QuestCompleteText`,
`WayrethIntro`, `WayrethChoice`) inside the existing `DialogueSession`
state machine, reached from a Greeting dismissal ahead of `BoatOffer` —
the same precedence `GameLoop::talkTo` itself uses — rather than the
console's blocking `readKey()` loop. `GameState::quests`/`monsterKills`
already round-tripped through `SaveGame.cpp` before this port (already
linked into the SFML target); this was pure UI/logic wiring, not a
save-format change. `docs/MILESTONES.md`'s entry 183 has the full list of
what changed, including a real pre-existing bug this surfaced (ordinary
overworld walking never populated `visitedLocations` at all in that
build, silently breaking every `VISIT` objective).

## Deliberately not in v1

- **A *generic* item-reward mapping.** Mapping an arbitrary data string to
  a concrete `InventoryItem` needs a name<->enum reverse lookup this
  project's raw-int save design deliberately avoids everywhere else. Still
  true — but `REWARD_SOLAMNIC_ARMOR` (see "Dragonlance magical items"
  below) shows the narrower, defensible form: a single named, compile-time
  bare flag, exactly like `REWARD_KNIGHT_SWORD`, not a string-driven
  mapping. Extend this way (one more named flag) rather than reopening a
  generic `REWARD_ITEM <string>` keyword.
- **Stage index, quest chains, prerequisites, abandonment, failable or
  timed quests, multiple quests per POI.**
- **Per-objective "you just did the thing!" pop-ups** (as opposed to the
  whole-quest "ready to turn in" notification, which *does* exist now —
  see "Proactive readiness notification" above). Telling the player
  exactly which objective advanced, mid-fight or mid-walk, would need
  finer-grained hooks than the four call sites `checkQuestReadiness` uses;
  not justified yet with only `VISIT`/`TALK`/`SLAY` and no multi-objective
  authored quest to make the distinction matter.
- **Journal scrolling, map markers, a HUD quest tracker.**
- **Per-quest kill baselines** (see "Objectives are queries over existing
  state" above for why this was a deliberate non-goal, not an oversight).

## Extending this later

Milestone 52 shipped the ordinary-NPC content pass, Milestone 53 shipped
Knight of the Sword advancement, the Dragonlance magical items milestone
shipped `solamnic_armor`, the Order of the Rose milestone shipped
`measure_of_roses` (completing the Crown→Sword→Rose chain), and the
DELIVER milestone shipped `ore_for_the_forge` (see "DELIVER"/"Shipped
quests" above) — VISIT, TALK, SLAY, DELIVER, a mixed-kind quest, REQUIRE
conditions beyond `knight`, and an item-granting reward are all proven
live in real, played content. Every objective kind from the original
design is now shipped; what's still open:

- **DLA's "Magical Items of Krynn" chapter (pp.91-99) is now fully
  mined** — the equipment-expansion milestone viewed every remaining
  un-shipped entry directly (rendered page images, `.research/img/
  dla_magicitems-092.png` through `-100.png`). All of it is either a
  unique artifact permanently owned by a named canon character (Staff/
  Dagger of Magius, Bupu's Emerald, the Bloodstone of Fistandantilus,
  Dalamar's Bracelet/Ring/Wand, Tasslehoff's Magic Mouse Ring, the
  Nightjewel, Warbringer, the Axe of Brotherhood/Sword of Friendship,
  Raistlin's Cursed Money, Rabbitslayer, the Helm of Griffon Mane — see
  `docs/CHARACTER_NOTES.md`'s "Magic items" for why these stay off-limits,
  same reasoning as Alhana/Derek/Gunthar staying off-stage) or needs a
  subsystem this engine doesn't have (creature charm/command, a
  translation flag, time travel, moon-phase magic, a plot-key/door
  mechanic). Don't re-open this chapter expecting to find something
  buildable — the next equipment gap, if any, is in the PHB's own
  Weapons/Armor tables instead (see `docs/CHARACTER_NOTES.md`'s
  "Equipment" section for what's already been drawn from there).
- **A second DELIVER quest reward, or item-granting quest, is still
  possible** — the equipment-expansion milestone's zone-file scan found
  exactly one more strong, unforced item-fetch hook (Thorbardin's
  Refugee Quarter, now `seed_for_thorbardin`); every other quest-less
  NPC checked had only a VISIT/TALK-shaped hook already well covered by
  existing quests. A future pass would need a genuinely new zone or
  NPC to find another one rather than reusing an existing hook. **Since
  shipped a third time** — see `what_the_tide_kept` above, which needed
  exactly the "new zone or NPC" this note predicted: Port O'Call postdates
  every prior sweep, and its hook came from giving voice to an
  already-described-but-silent shop POI, not from mining existing
  dialogue.
- **The `what_the_stones_remember`/`word_to_the_wilder_kin`/
  `new_faces_on_the_road` pass (see "Shipped quests" above) confirms the
  well is nearly dry, and `reason_worth_giving` (also above) closes the one
  real gap that sweep left.** That sweep's own checked-and-rejected list
  (Crossing, Palanthas's Knight/Astinus, Sancrist's Embarkation Officer,
  Flotsam, Port Balifor, Tarsis's Sailor/Runner, Pax Tharkas's Fortress
  Guard) never actually named Qualinesti's Elven Sentinel, Neraka's
  Deserting Guard, or Plains of Dust's Rider — re-checked directly for
  `reason_worth_giving`: the Sentinel and the Deserting Guard genuinely
  don't hold up (pure gatekeeping and pure desertion, neither shaped like an
  errand), but the Rider did. With all three now checked, every `TALK`-
  having POI across all 23 zone files has had a real look. Absent a new
  zone/NPC or a concrete user ask, don't re-run this sweep expecting to find
  more — same caution as the DLA magic-items chapter above.
- **Re-confirmed at the `what_the_tide_kept` milestone, this time
  checking every zone added since (Dargaard Keep, Foghaven Vale,
  Godshome, Hopeful Vale, Mount Nevermind, Port O'Call, Qualimori,
  Que-shu).** The well really is dry now for the "reuse existing
  dialogue" method specifically — every one of those either has zero
  talkable NPCs (deliberately, for the sacred/ruin/scenery ones) or, for
  Port O'Call, only a deliberately-mundane boat-logistics voice. The one
  way forward from here is `what_the_tide_kept`'s own move: give voice to
  an already-described-but-silent POI rather than invent a wholly new
  character. **Confirmed working a second time at `a_widows_due`**
  (Kalaman's Curiosities Cart) — that pass scanned every zone, not just
  ones added since the last sweep, and still found only one real
  standout; the well is getting shallower with each use, not deeper.
  Don't re-run the "check existing dialogue" sweep again absent a new
  zone; do consider the give-a-silent-POI-a-voice move again if a
  concrete future ask wants more quest content, but temper expectations —
  two uses in, genuinely person-shaped silent POIs (not scenery, not
  already-off-stage political seats) are getting hard to find.
