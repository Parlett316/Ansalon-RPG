# Current work

Nothing in flight.

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
