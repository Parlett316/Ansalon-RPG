# Current work

Nothing in flight.

Milestone 53 (Knight of the Sword advancement) just shipped: a new
`character::KnightOrder::Sword` value, `meetsKnightOfSwordRequirements`
(Str12/Int9/Wis13/Dex9/Con10), and `game::conditionMatches`'s new
`sword_eligible` token (Crown + level>=3 + those minimums) -- this
project's first compound, non-single-word `REQUIRE` condition. Real DL
Adventures pp.17-19 requirements, reconfirmed via rendered page images
this session, which turned up a genuine 1987 book erratum: p.18's Sword
minimums box is printed with a "Rose Knight Minimum Scores" header,
contradicted by the correctly-labeled Rose box on p.19 with different
values -- see `docs/CHARACTER_NOTES.md`'s "Knights of Solamnia" section
("Sourcing note: a book erratum").

The real quest, `named_in_fact` ("In Fact as Well as Blood"), is given by
a new POI (`S`, "A Sword Knight") at High Clerist's Tower's Muster Yard --
reframing that POI's own pre-existing "names a new Knight in fact as well
as blood" flavor line, since the Garrison Knight POI already carries
`word_for_the_tower` and v1 allows only one quest per POI. This project's
first quest to mix two objective kinds: `VISIT plains_of_dust` (the book's
500-mile/30-day journey) and `SLAY baaz 1` (its single combat, satisfied
narratively for free by this project's existing "knocked out, not killed"
combat framing). The book's other four required elements (three tests of
wisdom, one of generosity, one of compassion, restoring something lost)
have no trackable state, so they're narrated in the `COMPLETE` text
instead, the same treatment Milestone 47's ridge farewell gave untracked
lore. A new bare `REWARD_KNIGHT_SWORD` quest-file flag promotes
`knightOrder` on turn-in. `SaveGame.cpp`'s `KNIGHTORDER` bound moved from
2 to 3 values (append-only-safe -- old saves' 0/1 stay valid).

Verified via a throwaway self-test (ability-score boundary cases,
`QuestLoader` parsing the new reward flag and its fail-fast
trailing-argument case against the real six-quest `data/quests.txt`), a
clean `/W4` rebuild (zero new warnings), a direct check that the user's
real `save.txt` (a level-1 Human Fighter) still loads cleanly under the
new `KNIGHTORDER` bound, and the standard piped character-creation smoke
test (both real `save.txt` copies -- repo root and `build/Debug/` --
moved aside and restored around it). Docs updated: `docs/CHARACTER_NOTES.md`
("Knights of Solamnia" rewritten for Crown+Sword, the erratum note, the
Rose/healing-abilities backlog trimmed), `docs/QUEST_NOTES.md` ("Shipped
quests", grammar table, "Extending this later"), `docs/ZONE_NOTES.md` (new
POI, both the quest-POI list and the zone's own writeup), `docs/MILESTONES.md`
(new entry + NEXT UP now leads with Order of the Rose), `README.md` Status
paragraph.

**Not yet verified**: real interactive playthrough (actually reaching
level 3 as a Crown Knight, confirming the Sword Knight only offers the
quest once `sword_eligible` is true, completing the VISIT+SLAY mix, and
confirming the character sheet shows "Knight of the Sword") -- `_getch()`
can't be piped, the same limitation flagged for Milestones 51 and 52.
Needs the user's own keyboard before calling the UI path fully done.

NEXT UP (`docs/MILESTONES.md`) now leads with Order of the Rose
advancement (the natural follow-up now that the Sword pattern exists --
real p.19 requirements already researched, see `docs/CHARACTER_NOTES.md`'s
"Rose Knights, for real") and the still-unused `DELIVER` objective kind,
followed by terrain-specific monster pools and more monsters. Ask the user
before starting any of them.
