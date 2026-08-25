# Current work

Nothing in flight.

Milestone 79 (zone-NPC `SUBJECT` content -- "ask about anything" widened to
all 21 talkable zone-native NPCs) shipped 2026-08-25. Pure data, zero
`.cpp`/`.h` changes, verified via a clean rebuild and the piped smoke
test -- see `docs/MILESTONES.md` and `docs/ZONE_NOTES.md`'s "Ask about
anything" section for what shipped and why. Real in-terminal free-text
asking across the 21 new NPCs still needs the user's own keyboard
(`_getch()` can't be piped), same limitation every prior `SUBJECT`/picker
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
rendering revisit with real sprite art (item 3), widening `TALK_AFTER` to
three remaining zones (item 4), or widening the color palette further
(item 5).
