# Current work

Nothing in flight. Milestone 121 (the Wayreth quest), its Milestone 122
follow-up (Port O'Call, the Crossing/Flotsam ferry additions, the
repeatable-`BOAT` bugfix), and Milestone 123 (redoing `wayreth_summons`
twice in one session, per the user's "can't just be a standard fetch
quest" and then "what if the player falls it shouldn't just be... the
roll of a dice") are all implemented and documented -- see
`docs/MILESTONES.md` entries 121-123 for the full history.

Milestone 123's final shape: `SLAY wight 1` alongside `VISIT palanthas`
(pure data), plus a real `drawPickerFrame` choice at turn-in
(`GameLoop::offerOrTurnInQuest`'s `rewardWayrethRobe` block) that decides
the Robe **and now overwrites the character's actual alignment**
(`game::withEthic`, `GameLoop.cpp`/`.h`) instead of reading a stat frozen
at character creation. Verified via a clean `/W4` rebuild and the piped
smoke test (confirms nothing else broke; `QuestLoader`/`main.cpp`
cross-validation still accept everything). **Not yet interactively
verified** -- there's no piped-testable surface for the picker/setup
scene/alignment-shift log line at all (turn-in is well past character
creation, where `_getch()` stops being pipeable), and `save3.txt`
(Serath) already completed the old version of this quest and can't
retest it. Needs a fresh level-3+ Mage and the user's own keyboard;
trying at least two of the three picker options across playthroughs
would exercise both the "alignment changes" and "alignment already
matches, stays quiet" branches of the new log line.

All three save slots (Mason, Mike, Serath) were occupied this session,
so the piped smoke test itself couldn't run past the save-slot menu --
verification relied on the program reaching that menu at all, which only
happens after every data loader (including `QuestLoader`) parses
successfully.
