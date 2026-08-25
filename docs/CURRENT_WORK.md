# Current work

Nothing in flight.

Milestone 75 (Raistlin's deferred second "ask about anything" group --
draconians, Fistandantilus, Bupu, Alhana Starbreeze, Cyan Bloodbane, Lorac)
just shipped -- see `docs/MILESTONES.md`'s Milestone 75 entry and
`docs/TIMELINE_NOTES.md`'s "Ask about anything" section for the full
mechanism and sourcing. In brief: six new character-level `SUBJECT_WHEN`
pairs added to `data/timeline.txt`'s `CHARACTER raistlin` block, each gated
at the day the window establishing it opens (`darken_wood`/`xak_tsaroth`/
`tarsis`/`silvanesti`). Pure data, no `.cpp`/`.h` changes, no new grammar.
Verified via a clean rebuild (zero new warnings) and the piped smoke test,
including confirming no new `TimelineLoader` keyword-collision warnings.
**Interactive verification still needs the user's own keyboard** -- asking
Raistlin about each new subject, both before and after its gate day, in a
real playthrough.

Next backlog candidate (not started, not committed): widening "ask about
anything" to the other seven Heroes' own character-level pools -- see
`docs/MILESTONES.md`'s NEXT UP item 6.
