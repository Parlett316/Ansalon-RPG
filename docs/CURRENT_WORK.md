# Current work

Nothing in flight.

Milestone 77 (widened aftermath dialogue -- `TALK_AFTER` -- to five more
zones: Darken Wood's Forestmaster, the High Clerist's Tower's Garrison
Knight, Ice Wall's Young Knight, Silvanost's Warder, and Palanthas's
Knight of the Watch) just shipped -- see `docs/MILESTONES.md`'s
Milestone 77 entry and `docs/ZONE_NOTES.md`'s "Aftermath dialogue"
section for the full mechanism and per-POI sourcing. Pure data (one new
`TALK_AFTER <char> ...` line in each of five `data/zones/*.txt` files),
no `.cpp`/`.h`/grammar changes. Verified via a clean rebuild (zero new
warnings) and the piped smoke test (all five edited zone files parse
cleanly; the one stderr warning, the pre-existing Kitiara Solace-window
keyword overlap, predates this milestone). **Interactive verification
still needs the user's own keyboard** -- reaching each of the five zones
after its `latestDayEnd` and confirming the aftermath line fires once,
then falls back to ordinary `TALK_AGAIN` on the next visit.

Next backlog candidates (not started, not committed) -- see
`docs/MILESTONES.md`'s NEXT UP: three real zones still lack `TALK_AFTER`
(Pax Tharkas's Fortress Guard, Tarsis's Old Sailor/Knight's Runner,
Neraka's Deserting Guard -- item 5), widening the color palette further
(item 6), or zone-NPC `SUBJECT` content beyond Milestone 71's initial
pass (item 7).
