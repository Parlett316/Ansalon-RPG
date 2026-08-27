# Current work

Nothing in flight.

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

`docs/QUEST_NOTES.md`'s "Extending this later" section now flags that a
full sweep of every zone found only these three unforced hooks left --
don't re-run that sweep expecting more without a new zone/NPC or a
concrete user ask.

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
