# Current work

Nothing in flight.

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

**Doc staleness noticed in passing, not yet fixed**: CLAUDE.md's smoke-test
example (`echo "" | ./build/Debug/ansalon_rpg.exe`) no longer reaches
character creation on its own -- Milestone 89's save-slot menu now sits in
front of it, and an empty line doesn't pick a slot. `echo "1" | ...` (pick
empty slot 1) is what actually reaches character creation now. CLAUDE.md
itself wasn't touched this session since it's the top-level instructions
file, not something to edit as a side effect of an unrelated milestone --
flagged here for whoever picks this up next.

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
