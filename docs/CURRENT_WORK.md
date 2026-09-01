# Current work

Nothing in flight. Milestones 133 (Astinus rebuilt), 134 (World Map
screen), and 135 (full-screen presentation) are all implemented,
documented, and interactively confirmed by the user (2026-09-01) -- see
`docs/MILESTONES.md` entries 133/134/135 for what was walked and
confirmed on each.

Milestone 136 (`what_the_tide_kept`, a new DELIVER quest at Port O'Call)
and Milestone 137 (`SUBJECT_WHEN` day-gated zone dialogue, fixing 12
already-shipped Astinus spoilers, plus 3 new topics) are both
implemented, documented, and verified by throwaway self-test/clean
rebuild/piped smoke test, but **neither is yet interactively walked** --
same standing `_getch()` limitation. Worth doing on the next play
session: for 136, talk to the Beachcomber's Stall at Port O'Call, find
the new Storm-Wrack POI, deliver the locket, and confirm the reward/
journal entry read correctly; for 137, talk to Astinus early (before day
2/3/12/17) and confirm his "before" answers read naturally, then again
after day 160 (e.g. via a save with `hoursElapsed` past that point) to
confirm the "after" half still reads exactly as before this milestone.
