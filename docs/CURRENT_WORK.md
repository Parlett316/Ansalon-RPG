# Current work

Nothing in flight -- Milestone 113 is implemented, self-tested, and
rebuilt clean. The core of its interactive-verification flag (see below)
is now confirmed by the user's own playthrough: a Kender Thief hit a real
2-monster group encounter and saw the lettered target picker (A/B) render
and work correctly. What's still unconfirmed is narrower now -- see the
trimmed list below.

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
