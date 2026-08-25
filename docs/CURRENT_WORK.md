# Current work

Nothing in flight.

Milestone 80 (aftermath dialogue `TALK_AFTER` widened to the last three
real zones -- Pax Tharkas's Fortress Guard, Tarsis's Old Sailor, Neraka's
Deserting Guard) shipped 2026-08-25. Pure data, zero `.cpp`/`.h` changes,
verified via a clean rebuild and the piped smoke test (real save moved
aside and restored byte-identical afterward) -- see `docs/MILESTONES.md`
and `docs/ZONE_NOTES.md`'s "Aftermath dialogue" section for what shipped
and why. This closes the `TALK_AFTER` backlog entirely: every real
candidate is done, Godshome permanently out of scope (no talkable
zone-native NPC). Real in-terminal verification (reaching each zone after
its `latestDayEnd`, confirming each line fires once) still needs the
user's own keyboard, same `_getch()` limitation every prior `TALK_AFTER`
milestone has flagged.

**Note on the live save**: `build/Debug/save.txt` is still the
hand-edited maxed-stat (all six scores 18) Human Fighter built for the
Frostreaver verification trek (see Milestone 78). The user's actual
mid-game Fighter (EXP 967, day ~9 near Haven) was accidentally overwritten
by a throwaway Cleric test character in an earlier session and could not
be recovered. This maxed character remains the de facto live save unless
the user starts over.

Next backlog candidates (not started, not committed) -- see
`docs/MILESTONES.md`'s NEXT UP: an instant-defeat spell's interactive
verification (item 1), real Draconian mechanics (item 2), SFML-backed
rendering revisit with real sprite art (item 3), or widening the color
palette further (item 4).
