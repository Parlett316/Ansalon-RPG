# Current work

Nothing in flight. Milestones 133 (Astinus rebuilt), 134 (World Map
screen), and 135 (full-screen presentation) are all implemented,
documented, and interactively confirmed by the user (2026-09-01) -- see
`docs/MILESTONES.md` entries 133/134/135 for what was walked and
confirmed on each.

Milestone 136 (`what_the_tide_kept`, a new DELIVER quest at Port O'Call),
Milestone 137 (`SUBJECT_WHEN` day-gated zone dialogue, fixing 12
already-shipped Astinus spoilers, plus 3 new topics), Milestone 138
(3 more Astinus topics -- Kagonesti, gnomes, gully dwarves), and Milestone
139 (3 more Astinus topics -- minotaurs, ogres/Irda, Reorx/Graystone of
Gargath) are all implemented, documented, and verified, but **none is yet
interactively walked** -- same standing `_getch()` limitation. Worth
doing on the next play session: for 136, talk to the Beachcomber's Stall
at Port O'Call, find the new Storm-Wrack POI, deliver the locket, and
confirm the reward/journal entry read correctly; for 137, talk to Astinus
early (before day 2/3/12/17) and confirm his "before" answers read
naturally, then again after day 160 (e.g. via a save with `hoursElapsed`
past that point) to confirm the "after" half still reads exactly as
before this milestone; for 138, ask him about Kagonesti, gnomes, and
gully dwarves and confirm the new entries read correctly; for 139, ask
him about minotaurs, ogres, and Reorx and confirm the new entries read
correctly.

The Astinus ask-anything pool is an ongoing, open-ended content thread --
expect more sessions like 137/138 adding further topics as they come up.

Milestone 140 (combat grid starting positions widened -- player now
starts on the bottom row, monsters on the top row, an 8-row gap instead
of 5) is implemented and verified via clean rebuild + piped smoke test,
but **not yet interactively walked**. Worth doing on the next play
session: start a fight and confirm the wider gap plays well (not so wide
that closing distance feels tedious).

Milestone 141 (Weapon Specialization for Fighters -- PHB Tables 34/35: a
Fighter may choose at creation to specialize in their weapon for +1
to-hit/+2 damage and a faster attacks-per-round progression; full
per-weapon proficiency slots deliberately not modeled, see
`docs/CHARACTER_NOTES.md`'s "Weapon Specialization" section for why) is
implemented, documented, and verified via a throwaway self-test, a clean
rebuild (zero new warnings), and two piped character-creation smoke tests
(prompt appears correctly for a Fighter, correctly skipped for a Mage),
but **not yet interactively walked in a real fight**. Worth doing on the
next play session: create a specialized Fighter and confirm to-hit,
damage, and the faster extra-attacks rate all read correctly against a
real monster.

Milestone 142 (three new Monster Manual monsters -- Harpy p.184, Griffon
p.178, Stirge p.332, all visually confirmed against rendered page images;
roster grows from 26 to 29; see `docs/MILESTONES.md` entry 142 for the
terrain-code-honesty and danger-gating reasoning) is implemented,
documented, and verified via a data-only rebuild and a piped
character-creation smoke test confirming `MonsterCatalog` parses the
three new blocks cleanly, but **not yet interactively walked**. Worth
doing on the next play session: trigger a wilderness encounter on hills/
mountains (Griffon), grassland/hills (Harpy), and forest (Stirge) and
confirm all three read correctly in a real fight.

Milestone 143 (`a_widows_due`, a fourth DELIVER quest at Kalaman -- the
give-a-silent-POI-a-voice move proven a second time on the Curiosities
Cart; see `docs/MILESTONES.md` entry 143 and `docs/QUEST_NOTES.md`'s
"Shipped quests" for the full design) is implemented, documented, and
verified via a clean rebuild (zero new warnings) and a piped
character-creation smoke test confirming `ZoneCatalog`/`QuestCatalog`
parse the new content cleanly, but **not yet interactively walked**.
Worth doing on the next play session: talk to the Curiosities Cart in
Kalaman, find the new Furtive Trader POI, deliver the wedding band, and
confirm the reward/journal entry read correctly.

Milestone 144 (Haste and a real Slow -- Haste is a new Mage 3rd-level spell
that doubles the player's attacks-per-round this fight, unblocked now that
Milestone 108's `meleeAttacksThisRound` exists; Slow's existing THAC0
penalty was corrected from an invented -2 to the real -4 and gained a new
+4 AC penalty on the monster, both sourced against the scanned PHB p.192/
196 -- see `docs/MILESTONES.md` entry 144 and `docs/CHARACTER_NOTES.md`'s
"Haste and a real Slow" section for the full sourcing and what's still
deliberately unmodeled) is implemented, documented, and verified via a
throwaway self-test, a full clean rebuild (zero new warnings), and a piped
character-creation smoke test confirming `Spellcasting`'s new spell list
parses cleanly, but **not yet interactively walked**. Worth doing on the
next play session: as a Mage, cast Haste in a fight and confirm the logged
attack count doubles; cast Slow on a monster and confirm both the THAC0 and
AC math show up correctly in the "Showing the math" bracketed breakdown.

Nine milestones in a row (136-144) are now implemented but unplayed --
worth a dedicated playtest pass on the next session before piling on
more unverified content.
