# Current work

Nothing in flight.

Milestone 70 (widened aftermath dialogue -- `TALK_AFTER` -- to Haven's
Seeker Guard, Xak Tsaroth's Ruin-Scavenger, Qualinesti's Elven Sentinel,
and Kalaman's City Watchman) just shipped -- see `docs/MILESTONES.md`'s
Milestone 70 entry and `docs/ZONE_NOTES.md`'s "Aftermath dialogue" section
for the full list and rationale. Pure data content, no `.cpp`/`.h`
changes. Verified via a clean `/W4` rebuild (zero new warnings) and the
piped smoke test (confirms all four modified zone files still parse).
Interactive verification (walking to each zone after its `PRESENCE`
window(s) have closed, confirming the aftermath line fires once and falls
back to `TALK_AGAIN` after) still needs the user's own keyboard.
