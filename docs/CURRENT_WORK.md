# Current work

Nothing in flight -- Milestone 117 Phase 2 is implemented, self-tested, and
rebuilt clean, but carries the same interactive-verification flag every
`GameLoop`-facing milestone has, since none of the actual play loop can be
driven headlessly.

Milestone 117 (2026-08-28) shipped Phase 2 of the party system: the
companion now actually fights. Picked from a two-way `AskUserQuestion`
scope check at the top of the session -- "full mutual combat" (the
companion can deal AND take damage) over a smaller "free companion" slice
that would have kept the companion un-hittable -- because a companion that
can never be hurt reads as a hollow half-measure, not a real Gold Box ally.
The companion (Bren Alder, still the one fixed level-1 Human Fighter from
Milestone 116) now occupies its own cell on the tactical combat grid,
starting adjacent to the player; it acts automatically right after the
player's own action each round (AI-controlled, no player-directed control
yet -- that's Phase 3), attacking an adjacent alive monster instance or
stepping toward the nearest one via `combat::stepToward`/the new
`combat::chebyshevDistance` helper. Monsters now pick between the player
and the companion as their melee target (attack whichever they're adjacent
to, coin-flip if adjacent to both, path toward whichever is closer
otherwise) -- the first real touch to `GameLoop::runCombat`'s
single-`Character` assumption, made possible entirely by reusing existing
generic functions (`combat::resolvePlayerAttack`/`resolveMonsterAttack`/
`rollSavingThrow` already took `const character::Character&`, so
`Combat.h`/`.cpp` needed zero changes). Companion HP is now real and
persists: `GameState::companion.currentHp` is mutated during combat exactly
like the player's, and the save format's `COMPANION` line grew a second,
optional field to carry it (a pre-Milestone-117 one-token `COMPANION 1`
line still loads, defaulting to full health). A knocked-out companion
(HP <= 0) stops acting/being targeted for the rest of that fight but does
NOT end the fight -- only the player's own knockout does that. `Rest`/
`BedRest` now heal the companion the same way they already heal the player.
Deliberately deferred, honestly flagged (not oversights): the Brooch of
Imog's globe wards the player only; Bozak's Magic Missile and Aurak's
breath weapon stay player-only special attacks (never pick the companion as
a target); Sivak's death-burst still only damages the player regardless of
who lands the kill; companion movement never provokes/takes opportunity
attacks; there's no "finish off a downed ally" mechanic. Full design
writeup: `docs/ARCHITECTURE.md`'s "Party companions" section,
`docs/CHARACTER_NOTES.md`'s "Party companion" section, and
`docs/COMBAT_NOTES.md`'s "Extending this later" section. Verified via a
throwaway self-test (`combat::chebyshevDistance` across several coordinate
pairs, plus a `SaveGame` round-trip proving a damaged companion's HP
survives save/load exactly and that an old one-token `COMPANION 1` line
still loads at full health -- both deleted after), a clean `/W4` rebuild,
the piped smoke test (moved all three real saves aside to reach character
creation cleanly, then restored them byte-for-byte), and a direct check
that the real `save1.txt` -- which already carried a Milestone-116-vintage
one-token `COMPANION 1` line -- still loads correctly through the real
executable (slot listing shows "Mike, level 1 Human Fighter (Day 0)" with
no error).
**Interactive verification needed** (`_getch()` blocks all of it) -- a real
playthrough should confirm: the companion appears on the combat grid next
to the player and attacks automatically each round; a monster sometimes
targets the companion instead of the player (both when adjacent to only
one and via the coin-flip when adjacent to both); the companion's HP shown
on the combat frame/HUD/character sheet actually drops when hit; a
knocked-out companion shows "(knocked out)" and stops participating without
ending the fight; the Brooch's globe protects the player but NOT the
companion; Rest and BedRest both heal a damaged companion; and saving mid-
fight (or right after) then reloading preserves the companion's exact
current HP. See `docs/MILESTONES.md` entry 117.

Milestone 116 (2026-08-28) shipped Phase 1 of a party system: one
recruitable companion, no combat integration. Picked at the user's request
to start work toward `docs/MILESTONES.md`'s `NEXT UP` item 6 ("a party of
up to six characters"); offered a choice via `AskUserQuestion` between a
full one-pass build and a smaller first slice, the user picked the smaller
slice -- the same "ship the smaller half first" precedent Milestone 113
(monster groups) set before Milestone 114 (the grid). Research confirmed
why: `GameLoop::runCombat` is already one large function built entirely
around a single `character::Character`, and `combat::resolvePlayerAttack`/
`render::MapRenderer::drawCombatFrame`/the HUD/`CharacterCreator` all assume
exactly one PC exists -- teaching all of that about a second combatant in
the same pass as the save format and character creation was judged too much
for one sitting. New `character::buildCompanion()` (`Companion.h`/`.cpp`)
builds a fixed, deterministic level-1 Human Fighter (Bren Alder, Neutral
Good) -- fixed ability scores/steel, never `character::roll` -- so
`game::SaveGame` only persists a single `COMPANION 1` bool
(`GameState::hasCompanion`) rather than the companion's own fields; reload
just calls `buildCompanion()` again. Recruited via a new `RECRUIT <char>`
zone-grammar line (`data/zones/solace.txt`'s new `K "Bren Alder"` POI),
same "must already have a TALK line" validation as `BOAT`/`GRANTS_ITEM`/
`QUEST`, offered through the same Accept/Decline picker shape as `BOAT`'s
"Board"/"Not yet". Shown on the character sheet and HUD once recruited.
**Deliberately not attempted this phase**: the companion cannot fight
(`runCombat` never reads `hasCompanion`/`companion`), shop, level, be
dismissed, or move independently on the map -- reserved for Phase 2
(companion fights, AI-controlled) and Phase 3 (real multi-companion roster,
player-directed control, deployment order, backstab, sweep, `UIC`). Full
writeup: `docs/ARCHITECTURE.md`'s "Party companions", `docs/
CHARACTER_NOTES.md`'s "Party companion", `docs/COMBAT_NOTES.md`'s
"Extending this later". Verified via a throwaway self-test
(`buildCompanion()` determinism + sanity checks, deleted after), a clean
`/W4` rebuild, and the piped smoke test -- confirmed the new zone grammar
loads cleanly and all three of the user's real saves (written before
`COMPANION` existed) still load correctly with no companion.
**Interactive verification needed** (`_getch()` blocks all of it) -- a real
playthrough should confirm: talking to Bren Alder in Solace offers "Join
me"/"Not yet", declining leaves the offer re-appearing on a later visit,
accepting shows "Bren Alder joins your party." in the log and immediately
updates the character sheet/HUD, and saving then reloading (or restarting)
keeps the companion recruited with identical stats. See
`docs/MILESTONES.md` entry 116.

Milestone 115 (2026-08-28) shipped Phase 3 of the Gold Box-style combat
pass, fixing the user's own complaint: "the picking who to attack takes
you away from the screen." Every sub-choice inside a fight used to pop a
full-screen `drawPickerFrame`, clearing the terminal and hiding the grid/
HP roster/log at exactly the moment the player needed them -- confirmed a
real deviation from `References/DQoK.pdf`'s own manual, which targets on
the battle map itself (CENTER/EXIT commands), not via a separate list
screen. Scope was checked against a ranked gap analysis of everything
left separating this project from a real Gold Box game; the user picked
**in-frame targeting plus a real action menu** for this milestone, and **a
party of up to six characters** as the backlog item to record (now
`docs/MILESTONES.md`'s NEXT UP item 6). New `render::MapRenderer::
CombatPrompt` drives `drawCombatFrame`'s three states (idle command row,
grid-based target picking with a `[X]` bracket cursor and roster `> `
prefix, in-frame option list for spell/item choosers). New
`character::availableCombatItems` replaces the old fixed-priority `'i'`
handling, closing a real live gap along the way (a character carrying
both a Potion and a Webnet could never reach the Webnet before this).
Full design writeup and sourcing: `docs/COMBAT_NOTES.md`'s "In-frame
combat actions" section.
**Interactive verification needed** (`_getch()` blocks all of it) -- a
real playthrough should confirm: a group fight shows the `[A]`/`[B]`
bracket cursor moving on the grid while the log/HP roster stay visible
the whole time; a solo fight still auto-targets with no picker; a caster
with two-or-more memorized spells sees the in-frame chooser (not a
screen-clearing one); a character carrying both a Potion and a Webnet is
offered both under `i`; the idle command row shows only legal commands
(no `CAST` for a non-caster, no `USE:` hint with nothing carried); and
cancelling out of the spell/item choosers returns to the fight without
costing a round. See `docs/MILESTONES.md` entry 115.

Milestone 114 (2026-08-28) shipped Phase 2 of the Gold Box-style combat
pass: a real tactical grid with player/monster positions and movement,
picked as the next milestone from a backlog menu the user chose over
extending group sizes to the roster's dangerous tier or adding more
monsters. Before finalizing the design, `References/DQoK.pdf` (the actual
SSI Dark Queen of Krynn manual, already used for the Hoopak) was re-read
for its own real "COMBAT" section -- this corrected two assumptions in
the first draft (ranged weapons are disabled *while* adjacent, not usable
unconditionally; the grid is terrain-flavored) and surfaced a real,
sourced opportunity attack for moving away from an adjacent enemy, all
folded into the design before any code was written. It also surfaced a
real discrepancy in already-shipped Milestone 113 code -- a Fighter's
multi-attack should retarget after a kill instead of wasting the
remaining swings -- which the user asked to fix in the same session.
Melee attacks now require adjacency; the Tinker's Light Crossbow is this
project's first-ever ranged weapon (works at any range while unengaged,
refused outright once an enemy closes to melee); monsters close distance
via simple pathing when not adjacent instead of attacking from range.
Full design writeup, the DQoK.pdf sourcing, and the deliberately-deferred
findings (segmented initiative, encumbrance-based movement speed,
speed-based Flee, missile ammunition/range brackets, thief backstab):
`docs/COMBAT_NOTES.md`'s "Positional combat grid" section.
**Interactive verification needed, same heavier-than-usual flag as
Milestone 113** (`_getch()` blocks all of it) -- a real playthrough
should confirm: the grid renders with a terrain-appropriate backdrop and
correct player/monster glyphs, `w`/`a`/`s`/`d` moves the player and
consumes the round, a melee attack is refused with a message when
nothing is adjacent, closing distance and then attacking works, a Tinker
with the Light Crossbow can hit a non-adjacent monster but is refused
once something closes to melee range, moving away from an adjacent
monster triggers its free opportunity attack first, monsters visibly
approach when not adjacent and attack once they are, a Fighter's
multi-attack correctly retargets to a new opponent after a kill instead
of wasting the swing, and a solo fight's starting distance/pacing feels
reasonable rather than dragging. See `docs/MILESTONES.md` entry 114.

Milestone 113 (2026-08-28) shipped Phase 1 of a Gold Box-style combat
pass, at the user's request ("when a random battle begins we get a battle
screen like the old SSI gold box games"): monster encounter groups --
multiple monsters of the same type per fight (sourced `GROUP <min> <max>`
data, re-checking every roster monster's real "No. Appearing" field via
rendered page images), individually tracked HP, lettered identity, and a
target picker, with no position/grid/movement (that stays a separate,
later, deliberately-unstarted phase). Scoping was explicitly checked with
the user twice via `AskUserQuestion`: once to confirm Phase 1 (groups
only) over attempting the full tactical grid in one session, and again
after the No. Appearing research turned up real group data for
already-dangerous, currently-ungated monsters (Wight, Troll, Thanoi, plus
the already-gated Ogre/Kapak/Bozak/Sivak/Ettin) -- the user chose to defer
grouping those twelve rather than risk an untested difficulty spike, so
only the roster's 14 low/mid-HD "line troop" monsters carry a `GROUP` line
this pass. Full design writeup, the sourcing table, and every deferred
item: `docs/COMBAT_NOTES.md`'s "Monster encounter groups" section.
**Interactive verification, updated 2026-08-28**: confirmed by the user's
own real playthrough -- a Kender Thief encountered a genuine 2-monster
group, and the lettered target picker (A/B) rendered and worked
correctly when attacking. `_getch()` still blocks the rest from being
driven headlessly, and a Kender Thief can't exercise every path (no
multi-attack, no spellcasting), so still open: a solo-monster fight
reading/playing exactly as before (no letters/picker -- should already
hold since that code path is untouched, but not yet directly observed), a
Fighter's multi-attack landing all its swings on one chosen target, a
target dying mid-multi-attack wasting the remaining swings without
misbehaving, a non-damage spell (Sleep/Hold/Bestow Curse-style) or Webnet
correctly asking which enemy to target, a multi-kill round (e.g. two
Goblins killed in one Fighter double-attack) correctly advancing a `SLAY
goblin` quest objective by more than one, and Baaz Draconians appearing
in a group of 2-4 (never solo) with per-instance "turns to stone"
messages. See
`docs/MILESTONES.md` entry 113.

Milestone 112 (2026-08-28) added three more Monster Manual monsters --
Lizard Man, Giant Toad, Ettin -- bringing the roster to 26, picked once a
fresh check confirmed the `NEXT UP` backlog and the quest well (Milestone
106) were both dry, leaving the bestiary as the one genuinely open
backlog item per `docs/COMBAT_NOTES.md`'s own "Extending this later"
section. Before picking these, DLA's "Common Creatures of Krynn" chapter
was rechecked for anything unused since Milestone 107's Ice Bear --
Dreamshadow, Dreamwraith, Fetch, Minotaur (Bloodsea), Shadowpeople, and
Spectral Minion are all real entries there, but none fit this project's
wandering-overworld-encounter model (illusion-only, mirror-only,
civilized-race, Sanction-only, and death-site-bound respectively), so the
DLA well stays confirmed dry beyond Ice Bear. All three actual additions
are visually confirmed against rendered Monster Manual page images
(pp.227, 345, 135). Lizard Man's and Ettin's real multi-attack stat lines
are each simplified to their single most damaging hit (same treatment as
the Ghoul/Owlbear); Lizard Man and Giant Toad both carry `TERRAIN_BIAS *`
(bog), sourced directly from their own real swamp/near-water Climate/
Terrain fields and filling what was an almost-unused terrain code; Ettin
carries `TERRAIN_BIAS ^ A` (hills+mountains, its own real field) and
`MIN_TOWN_DISTANCE 40` (HD10, the roster's new highest, between Sivak's
35 and Aurak's 45). Pure data content -- no `.cpp`/`.h` changes (every
keyword needed already exists in `MonsterLoader.cpp`), no save-format
changes. Verified via a clean `/W4` rebuild and the piped smoke test
only, no throwaway self-test needed (same reasoning as Milestones 34,
100, and 107) -- no interactive verification flag either, since parsing
is the only thing to confirm and the smoke test already covers it. See
`docs/MILESTONES.md` entry 112 and `docs/COMBAT_NOTES.md`'s roster/
"Extending this later" and `MIN_TOWN_DISTANCE` sections.

Milestone 111 (2026-08-27) added the Hoopak (a real, race-gated Kender-only
weapon sourced from DQoK.pdf's Weapons Table, since neither the PHB nor
Dragonlance Adventures stats it) plus two new armor tiers, Hide Armor
(AC6, a real gap-filler) and Field Plate (AC2, re-priced to 1200stl rather
than the book's 2,000gp at the user's request). Also fixed an incidental
sourcing bug found along the way: the Tinker's Light Crossbow was wrongly
1d4+1 (that's the Heavy Quarrel's damage; Light Quarrel is 1d4, no bonus).
Verified via a 35-assertion throwaway self-test (deleted), a clean `/W4`
rebuild, the piped smoke test, and a direct check that both of the user's
real saves still load byte-for-byte unchanged.

**Follow-up, same session**: at the user's request, Kender now *start*
equipped with a Hoopak (`CharacterCreator::run`), instead of only being
able to buy one -- a three-line conditional swapping in the Hoopak's
stats instead of the class's normal starting weapon whenever
`character.race == RaceId::Kender`. Verified via a clean rebuild and the
piped smoke test; no self-test needed (too small, reuses already-proven
constants).

**Second follow-up, same session**: at the user's request ("Kenders
cannot be evil alignment"), re-checked DLA p.53 (the same page already
sourcing Kender's ability ranges/class limits) and confirmed it directly:
"No evil kender are known to exist." New `character::meetsAlignmentRestriction`
(`Race.h`/`.cpp`) hard-blocks Kender from all three Evil alignments in
`CharacterCreator`'s alignment prompt, same reject-and-reprompt shape as
the race/class prompts. First race-based alignment restriction this
project has ever enforced -- `docs/CHARACTER_NOTES.md`'s "Scope" section
updated accordingly. Verified via a 54-assertion throwaway self-test
(deleted), a clean rebuild, the piped smoke test, and confirming the
user's real saves still load unchanged (no save-format change).
**Interactive verification still needed** (running list) -- buying Hide
Armor/Field Plate at a shop that carries them, a real Kender character
confirming their sheet shows "Weapon: Hoopak" at creation, and confirming
the alignment screen blocks/annotates all three Evil options for a
Kender. See `docs/MILESTONES.md` entry 111 and `docs/CHARACTER_NOTES.md`'s
"Equipment" and "Kender in place of Half-Orc and Halfling" sections.

Milestone 110 (2026-08-27) removed Halfling as a playable race at the
user's request -- Krynn has no separate Halfling people in Dragonlance
canon, Kender already fill that niche. `RaceId::Halfling` and all its
data-table/condition-vocabulary references removed. Along the way this
surfaced a real instance of `docs/GOTCHAS.md`'s raw-enum-int save
fragility: deleting `Halfling` (ordinal 5) silently renumbered `Kender`
from 6 to 5 and broke a real save the user had created minutes earlier in
this same session while testing Milestone 109 (`build/Debug/save1.txt`,
a level-1 Kender Thief named "Mason"). Fixed by pinning
`RaceId::Kender = 6` explicitly (leaving ordinal 5 unused) and adding
`character::kRaceIdCount` for `SaveGame.cpp`'s RACE bounds check, the
same shape already used for `ClassId`/`kClassIdCount`. **Confirmed fixed
by reloading the real save through the built executable** -- the slot
menu shows "Mason, level 1 Kender Thief (Day 0)" and loads cleanly again;
this is stronger proof than a synthetic test, so none was added. Clean
`/W4` rebuild and the piped smoke test also pass. No new interactive-
verification flag beyond what Milestone 109 below already carries. See
`docs/MILESTONES.md` entry 110, `docs/GOTCHAS.md`'s `RACE`/`CLASS`/
`ALIGNMENT` note, and `docs/CHARACTER_NOTES.md`'s "Kender in place of
Half-Orc and Halfling" section.

Milestone 109 (2026-08-27) shipped ability score ranges and class level
limits -- closing a gap flagged "not modeled, deliberately" since the
Elf/Dwarf subrace and Kender milestones, picked from the backlog at the
user's request once the quest well, DLA magic items, and Fighter
multi-attacks (Milestone 108) were all confirmed shipped. Sourced from
rendered page images, not OCR (badly garbled for every one of these
tables): PHB Table 7 (p.27), DMG Table 7 ("Racial Class and Level Limits,"
p.15 -- genuinely absent from the PHB itself, which defers this to "ask
your DM"), and Dragonlance Adventures' per-subrace/Kender tables
(pp.53,59-61,66-67). New `character::meetsAbilityRange`/
`meetsSubraceAbilityRange`/`classLevelCap` (`Race.h`/`.cpp`), both
**hard-enforced** in `CharacterCreator` (reject-and-reprompt, not the
project's older soft-annotate UX) -- Human always qualifies for the race
check so the player is never dead-ended. Fixes two real, previously-live
mismatches: Halfling could be a Mage, Silvanesti Elf could be a Thief;
both are blocked now. `Leveling.cpp` stops converting XP into levels once
a demihuman hits their class's cap (XP itself still accrues). Mage/Cleric
map onto Dragonlance's licensed rows (Wizard of High Sorcery, Holy Orders
of the Stars), a user-confirmed interpretation. No save-format changes.
Verified via a throwaway self-test (30+ assertions), a clean `/W4`
rebuild, and the piped smoke test. **Interactive verification still
needed** -- triggering a real reject-and-reprompt on the race/class
screens with actual (random) rolls, and a demihuman actually hitting a
level cap in a long real playthrough -- same `_getch()` limitation as
every other character-creation/leveling milestone. See
`docs/MILESTONES.md` entry 109 and `docs/CHARACTER_NOTES.md`'s "Ability
score ranges and class level limits" section.

Milestone 108 (2026-08-27) shipped Fighter multi-attacks per round --
the first real engine change in several milestones (107 and earlier were
pure data), picked by the user from a backlog menu offered once the
session confirmed the quest well and DLA magic items were both still dry.
New `character::meleeAttacksThisRound(ClassId, level, roundNumber)`
(`Leveling.h`/`.cpp`), sourced from PHB Table 15 ("Warrior Melee Attacks
per Round," p.36, visually confirmed via a rendered page image): Fighter
(the only implemented Warrior-group class) gets 1/round at levels 1-6,
3/2 rounds at 7-12, 2/round at 13+; every other class stays at 1/round.
The 7-12 bracket's odd/even split (1 attack on odd rounds of the fight, 2
on even) is this project's own interpretation of the book's un-detailed
"3/2 rounds" rate -- an invented-but-flagged convention, same treatment
as the sell-back half-price rule (Milestone 28). `GameLoop::runCombat`'s
`playerAttacks` lambda now loops that many times per round, fed by a new
local `roundNumber` counter; only ordinary weapon attacks get the
multiplier, not casting/potions/Webnet/Brooch/Staff of Curing. No
save-format changes. Verified via a throwaway self-test (13 assertions
across the level boundaries and the odd/even split), a clean `/W4`
rebuild (zero new warnings), and the piped smoke test. **Interactive
verification still needed** -- a Fighter actually reaching level 7 and 13
in a real fight to see the extra swings and the odd/even pattern live --
same `_getch()` limitation as every other combat-facing milestone. See
`docs/MILESTONES.md` entry 108, `docs/CHARACTER_NOTES.md`'s "Leveling /
experience", and `docs/COMBAT_NOTES.md`'s "Accuracy" section.

Milestone 107 (2026-08-27) shipped three new monsters -- Black Bear, Worg,
Ice Bear -- bringing the roster to 23, picked from `docs/COMBAT_NOTES.md`'s
"Extending this later" bestiary backlog once a fresh check confirmed quests
(`docs/QUEST_NOTES.md`) and DLA magic items (`docs/CHARACTER_NOTES.md`) are
both exhausted. Black Bear and Worg are ordinary Monster Manual entries
(pp.17/362, visually confirmed via rendered page images); Ice Bear is this
project's first roster pull from DLA's broader "Creatures of Krynn" chapter
(p.76) beyond the Draconians/Thanoi, and shares the Thanoi's `ONLY_TERRAIN`
glacier restriction since the book's own prose ties the two together
(thanoi use ice bears to track prey and share the kill). Ice Bear's THAC0
(un-printed in DLA, same recurring gap as every other Krynn-specific
monster) was derived via this project's established HD-to-THAC0 pattern;
its XP is a real per-hp formula simplified to a flat value, same treatment
as the five Draconians. Pure data content -- no `.cpp`/`.h` changes, no
save-format changes. Verified via a clean `/W4` rebuild and the piped smoke
test only, no throwaway self-test needed (same reasoning as every prior
pure-monster-roster milestone, 34 and 100) -- no "interactive verification
needed" flag either, since parsing is the only thing to confirm and the
piped smoke test already covers it. See `docs/MILESTONES.md` entry 107 and
`docs/COMBAT_NOTES.md`'s roster/"Extending this later" sections.

Milestone 106 (2026-08-27) shipped one new quest, `reason_worth_giving`, at
the Plains of Dust -- the user asked for a new content milestone; DELIVER/
item rewards turned out already shipped (Milestones 51-102), and the
Milestone 105 "unforced hook" sweep looked exhausted until a closer check
found it had never actually looked at Qualinesti's Elven Sentinel, Neraka's
Deserting Guard, or Plains of Dust's Rider. The first two don't hold up, but
the Rider does: her own already-shipped "you still haven't given me a good
reason" line gets paid off with a disturbed-burial-mounds `SLAY ghoul 2`
errand (Ghoul being the one Monster-Manual-sourced monster no quest had used
yet). New flavor-only POI `M "The Old Mounds"` and a new `SUBJECT R
mounds,graves,dead,barrows` entry accompany the `QUEST R
reason_worth_giving` binding. Plains of Dust is this project's one
deliberately invented zone, so this needed no novel citation the way every
other zone's content does. Pure data content -- no `.cpp`/`.h` changes, no
save-format changes. Verified via a throwaway self-test, a clean `/W4`
rebuild, and the piped smoke test (no real save existed at `build\Debug\` to
protect this session). **Interactive verification still needed** --
accepting the quest, killing 2 Ghouls, turning in, and confirming the new
`SUBJECT`/POI read well in a real conversation. See `docs/MILESTONES.md`
entry 106 and `docs/QUEST_NOTES.md`'s "Shipped quests" section.

`docs/QUEST_NOTES.md`'s "Extending this later" section is now updated to
reflect that all three previously-unchecked NPCs (Qualinesti, Neraka, Plains
of Dust) have had a real look -- the quest-hook well really is dry now,
don't re-run this sweep expecting more without a new zone/NPC or a concrete
user ask.

Milestone 105 (2026-08-27) shipped three new quests, at the user's request
for "more quests and things to do for the PC": `what_the_stones_remember`
(Darken Wood's Unicorn, `SLAY owlbear 1`), `word_to_the_wilder_kin`
(Southern Ergoth's Silvanesti Sentry, `TALK southern_ergoth:K`, via a new
"A Kaganesti Lookout" POI split off the zone's `TIMELINE_ANCHOR`), and
`new_faces_on_the_road` (Haven's Seeker Guard, `SLAY gnoll 3`). All three
reframe hooks already written into existing `TALK`/`TOPIC`/`SUBJECT`
flavor text -- pure data content, no `.cpp`/`.h` changes, no save-format
changes. Verified via a throwaway self-test, a clean `/W4` rebuild, and the
piped smoke test (no real save existed at `build\Debug\` to protect this
session). **Interactive verification still needed** -- accepting/
completing all three quests, and confirming Southern Ergoth's Sentry
reads naturally now that she offers both a quest and (already) a boat
voyage in the same conversation. See `docs/MILESTONES.md` entry 105 and
`docs/QUEST_NOTES.md`'s "Shipped quests" section.

Milestone 104 (2026-08-27) made the "ask about anything" screen show its
available keywords ("You could ask about: Caramon, Goldmoon, ...") instead
of making the player type a subject blind. Purely additive:
`matchSubject`/`tokenizeAskInput`/`SUBJECT_UNKNOWN`/`Console::readLine` and
every `data/*.txt` file are unchanged. A fully clickable keyword menu was
considered and rejected (no picker scrolling exists yet, and some
characters have 20+ subjects). Verified via a throwaway self-test, a clean
rebuild, and the piped smoke test. **Interactive verification still
needed** -- talking to a large-subject-pool character (Raistlin) to confirm
the hint line reads well in a real conversation. See `docs/MILESTONES.md`
entry 104 and `docs/TIMELINE_NOTES.md`'s "Ask about anything" section.

Milestone 103 (2026-08-27) gave the level-3 Mage "Test of High Sorcery"
flavor moment three distinct outcome passages (White/Red/Black), replacing
the single generic templated line every Robe used to share -- sourced from
rendered DLA p.33-37 page images, each passage dramatizing a different
named Test guideline from the book (unsolvable-by-magic trial, combat
against an ally, the Red Robe's defining "balance" identity). Pure flavor
text in `character::applyPendingLevelUps` (`Leveling.cpp`), no new fields
or save-format changes. Verified via a clean `/W4` rebuild and the piped
smoke test (the user's real save loaded untouched); no throwaway self-test
needed. **Interactive verification still needed** -- a Mage reaching level
3 under each of the three alignment groups to see all three new passages.
See `docs/MILESTONES.md` entry 103 and `docs/CHARACTER_NOTES.md`'s
"Wizards of High Sorcery".

Milestone 102 (2026-08-26) also still has its own interactive
verification outstanding -- buying Studded Leather/Plate Mail at the
shops that should carry them, equipping the Mage's Quarterstaff and the
Tinker's Light Crossbow, and finding/accepting/completing
`seed_for_thorbardin` at Haven and Thorbardin. See `docs/MILESTONES.md`
entry 102.
