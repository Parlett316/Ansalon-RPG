# Current work

Nothing in flight.

Milestone 76 (widened "ask about anything" to the other seven Heroes --
Tanis, Caramon, Flint, Goldmoon, Riverwind, Sturm, Tasslehoff) just
shipped -- see `docs/MILESTONES.md`'s Milestone 76 entry and
`docs/TIMELINE_NOTES.md`'s "Ask about anything" section for the full
mechanism, gating table, and sourcing. Pure data (`data/timeline.txt`,
~85 new `SUBJECT`/`SUBJECT_WHEN` lines across the seven `CHARACTER`
blocks), no `.cpp`/`.h`/grammar changes. Verified via a clean rebuild
(zero new warnings) and the piped smoke test, confirming stderr shows no
new `TimelineLoader` keyword-collision warnings from any of the new
content. **Interactive verification still needs the user's own
keyboard** -- asking each of the seven Heroes about themselves, each
other, and testing that the Sturm/81, Raistlin/83, and Flint/103 gated
answers actually change on the right side of each boundary, in a real
playthrough.

Next backlog candidates (not started, not committed) -- see
`docs/MILESTONES.md`'s NEXT UP: widening the color palette further
(item 6), or zone-NPC `SUBJECT` content beyond Milestone 71's initial
pass (item 7).
