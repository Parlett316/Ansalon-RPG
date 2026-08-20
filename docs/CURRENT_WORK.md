# Current work

Nothing in flight.

The Order of the Rose milestone just shipped: the capstone of the Knights
of Solamnia chain, completing Crown (character creation) -> Sword
(Milestone 53) -> Rose. Re-confirmed the sourcing directly against rendered
page images of *Dragonlance Adventures* pp.18-19 (`pdftoppm`) rather than
trusting Milestone 53's earlier partial pass from memory -- turned up a
second real book inconsistency alongside the p.18/p.19 Sword erratum
Milestone 53 already found: the Rose "Minimum Requirements" prose
("two levels as Crown, then Sword and two additional levels," arithmetically
level 5) contradicts its own very next sentence ("sufficient hit points to
become 4th level"). Resolved via the Rose Knight Advancement Table itself,
which starts at level 4 ("Novice of Roses") -- the same hard-table-over-
loose-prose tie-break already used for the Sword erratum.

A new `character::KnightOrder::Rose` value (append-only-safe),
`character::meetsKnightOfRoseRequirements` (Str15/Int10/Wis13/Dex12/Con15),
and `game::conditionMatches`'s `rose_eligible` token (`knightOrder==Sword &&
level>=4 && meetsKnightOfRoseRequirements`), the same compound-condition
shape as `sword_eligible`. The real quest, `measure_of_roses`
(`data/quests.txt`), is given by a new POI (`R`, "A Rose Knight") at High
Clerist's Tower's Muster Yard -- `K`/`S`/`L` there already carry a quest
each. Objectives mix `VISIT plains_of_dust` (reusing `named_in_fact`'s
journey target, since the book's 500-mile/30-day requirement is identical
text between Sword and Rose) and `SLAY ogre 1` (the highest-XP, clearly-evil
single monster in the roster, deliberately distinct from Sword's Baaz duel).
Turning it in sets a new `REWARD_KNIGHT_ROSE` flag (100 steel/250 XP, above
Sword's 60/150). `SaveGame.cpp`'s `KNIGHTORDER` bound moved 3->4
(append-only-safe); a new `Leveling.cpp` flavor line foreshadows Rose
eligibility at level 4, mirroring the existing level-3 Crown->Sword line.

Verified via a throwaway self-test (25 checks: ability-score boundaries
including confirming Sword's own minimums don't accidentally satisfy Rose's
higher bar, `QuestLoader` against the real eight-quest `data/quests.txt`
including `REWARD_KNIGHT_ROSE`'s fail-fast case, and a save round-trip
covering both the widened `KNIGHTORDER` bound and its new exclusion
boundary), a clean `/W4` rebuild, a direct check that the user's real save
(the executable-relative `build\Debug\save.txt` -- discovered mid-session
that this, not the stale repo-root `save.txt`, is the live one; see
`docs/GOTCHAS.md`) still loads cleanly under the new bound, and the standard
piped smoke test.

**Not yet verified**: real interactive playthrough (reaching level 4 as a
Sword Knight, confirming the Rose Knight only offers the quest once
eligible, completing the VISIT+SLAY mix) -- `_getch()` can't be piped, the
same limitation flagged for every quest milestone so far. Needs the user's
own keyboard before calling the UI path fully done.

Docs updated: `docs/CHARACTER_NOTES.md` (new "Entry requirements for Knight
of the Rose" and "Sourcing note: a second inconsistency" under "Knights of
Solamnia", the now-shipped "Rose Knights, for real" bullet removed from
"Extending this later"), `docs/QUEST_NOTES.md` (grammar table's
`REWARD_KNIGHT_ROSE` row, new "Shipped quests" entry, "Extending this
later" trimmed), `docs/ZONE_NOTES.md` (the new POI, both the quest-POI
numbered list and the High Clerist's Tower zone writeup),
`docs/MILESTONES.md` (new Milestone 55 entry + NEXT UP renumbered),
`README.md` Status paragraph.

NEXT UP (`docs/MILESTONES.md`) now leads with more of the DLA magic items
chapter (Rods/Staves/Wands, Crystals and Gems, Miscellaneous Magic — real,
sourced, unused), then the still-unused `DELIVER` objective kind,
terrain-specific monster pools, and more monsters. Ask the user before
starting any of them.
