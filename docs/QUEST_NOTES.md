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
                                  than derived (see "Objectives are queries
                                  over existing state" below)
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
zero objectives at `END`; a `SLAY` count below 1; an objective missing its
label; end-of-file inside an open block.

## Objectives are queries over existing state, not counters

This is the central design decision, and it shapes everything else here.
An objective doesn't start counting from zero when a quest is accepted —
it's a live read of state the game already tracks:

| Kind | Reads |
|------|-------|
| `VISIT <location-id>` | `GameState::visitedLocations` (tracked since Milestone 3) |
| `TALK <met-id>` | `GameState::metCharacters` (tracked since Milestone 18) |
| `SLAY <monster-id> <count>` | `GameState::monsterKills` (new this milestone) |

`VISIT` and `TALK` needed **zero new tracking** — those sets already
existed for unrelated reasons and simply happened to answer "has the
player done this" already. `SLAY` needed exactly one new field:
`monsterKills`, a lifetime tally incremented in `GameLoop::runCombat` on
every kill, regardless of whether any quest cares about that monster.

**Consequence, not a bug: a quest can be instantly completable the moment
it's accepted**, if the player already did the thing before ever hearing
about it. This was already unavoidably true for `VISIT`/`TALK` (those sets
predate quests entirely — a well-traveled character could easily have
already visited the target location), so making `SLAY` behave differently
would have been the *inconsistent* choice, not the safe one. The
alternative — snapshotting a per-quest kill baseline on accept — is a
second saved map and an ordering rule, for a game whose whole combat design
is already forgiving (knocked out, not killed; see
`docs/COMBAT_NOTES.md`). Author quest flavor with this in mind rather than
fighting it: `road_wolves`'s `ACCEPT` text ("the wolves, presumably, are
still out there") reads fine either way.

**Labels are authored, not derived.** Generating "Kill three timber
wolves" from `wolf`'s id would need `combat::MonsterCatalog::find` (and the
equivalent for `world::World`/`timeline::Timeline` for the other two
kinds) — cross-module lookups `quest::Objective` deliberately doesn't have
access to. Writing the label directly in `data/quests.txt` costs nothing
and matches `PRESENCE`'s own trailing-flavor-text shape.

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

Written immediately after `BOAT`, before `ZONE`/`ZONESTACK` —
`SaveGame.cpp` documents that `load()`'s `ZONESTACK` consumption relies on
`save()` always writing `ZONESTACK` last with nothing after it,  so
anything new has to land earlier than that, not appended at the end.

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

## Deliberately not in v1

- **`DELIVER` / item-possession objectives.** `character::InventoryItem`
  only models Armor/Shield/Weapon/Potion and items carry no id — this
  would mean a new `ItemKind`, new `InventoryItem` fields, an
  unsellable-in-shop rule, an equip-path guard (a quest item has no
  slot), and a change to the save's `INVENTORY` block. A whole item
  subsystem hiding inside "one more objective kind," and the riskiest
  thing to build near the user's real save — deferred to Milestone 52.
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
Knight of the Sword advancement, and the Dragonlance magical items
milestone shipped `solamnic_armor` (see "Shipped quests" above) — VISIT,
TALK, a mixed-kind quest, REQUIRE conditions beyond `knight`, and now an
item-granting reward are all proven live in real, played content. Two
items from the original backlog are still open:

- **`DELIVER`/item-possession objectives**, if a future quest concept
  genuinely needs one rather than being addable with VISIT/TALK/SLAY (as
  every quest shipped so far has been) — see "Deliberately not in v1"
  above for what this would actually require.
- **Order of the Rose advancement**, the natural follow-up to Sword now
  that the pattern exists — see `docs/CHARACTER_NOTES.md`'s "Rose Knights,
  for real" for the real, already-researched XP/ability-score
  requirements. Not entangled with `solamnic_armor` above, which
  deliberately gates on the already-shipped Sword rank instead of waiting
  on Rose.
- **More of DLA's "Magical Items of Krynn" chapter** (Rods/Staves/Wands,
  Crystals and Gems, Miscellaneous Magic) is real, sourced, and
  unused — see `docs/CHARACTER_NOTES.md`'s "Magic items" for what's
  already sourced and why the chapter's unique named artifacts
  specifically stay out of scope regardless.
