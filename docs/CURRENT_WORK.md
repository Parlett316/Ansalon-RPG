# Current work

Nothing in flight.

Milestone 52 (real quest content) just shipped: four new quests from
ordinary NPCs (`inn_supply_run` at Otik's, `word_for_the_tower` at High
Clerist's Tower, `bazaar_road_raiders` at Kalaman, `kin_beyond_the_border`
at Silvanesti, `REQUIRE elf`-gated), proving VISIT and TALK objectives and
a non-`knight` `REQUIRE` live for the first time. Pure data content --
`data/quests.txt` plus one `QUEST <char> <quest-id>` line per target zone
file, zero `.cpp`/`.h` changes. Verified via a throwaway `QuestLoader`
self-test, a clean `/W4` rebuild, and the piped character-creation smoke
test (both real `save.txt` copies -- repo root and `build/Debug/` --
moved aside and restored around it). Docs updated: `docs/QUEST_NOTES.md`
("Shipped quests", "Extending this later"), `docs/ZONE_NOTES.md`,
`docs/MILESTONES.md` (new entry + trimmed/renumbered NEXT UP), `README.md`
Status paragraph. See `docs/QUEST_NOTES.md`'s "Shipped quests" section for
the full quest-by-quest writeup.

**Not yet verified**: real interactive playthrough (accepting each quest,
confirming `REQUIRE elf` actually gates `kin_beyond_the_border` to Elf
characters, and watching the proactive "ready to turn in" notification
fire for a VISIT and a TALK quest for the first time) -- `_getch()` can't
be piped and no PTY driver exists for this Windows console app, the same
limitation Milestone 51 flagged. Needs the user's own keyboard before
calling the UI path fully done.

NEXT UP (`docs/MILESTONES.md`) now leads with Knight of the Sword
advancement (deliberately deferred out of this pass -- see
`docs/QUEST_NOTES.md`'s "Extending this later" for the real DL Adventures
pp.18-19 requirements already researched) and the still-unused `DELIVER`
objective kind, followed by terrain-specific monster pools and more
monsters. Ask the user before starting any of them.
