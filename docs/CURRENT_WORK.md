# Current work

Nothing in flight.

Milestone 82 (widening zone-native NPC `SUBJECT` coverage -- 11 of the 21
Milestone-79 NPCs gained 1-2 new free-text-askable entries each,
paraphrased from their own already-shipped `TOPIC` text, plus two latent
`ZoneLoader` keyword-parsing bugs found and fixed along the way) shipped
2026-08-25, prompted directly by the user hitting the gap in play (asking
the Palanthas Knight of the Watch about the Tower siege got a canned
brush-off despite his own `TOPIC` covering it). Pure data content, zero
`.cpp`/`.h` changes, verified via a throwaway `ZoneLoader`-only self-test
(35 checks, all passing, deleted afterward), a clean `/W4` rebuild, and
the piped smoke test (real save moved aside and restored byte-identical
afterward) -- see `docs/MILESTONES.md` and `docs/ZONE_NOTES.md`'s "Ask
about anything" for what shipped, what was deliberately left alone, and
why. Interactive verification (actually asking these NPCs the new
keywords) still needs the user's own keyboard, same limitation every prior
`SUBJECT`/`TOPIC` content milestone has flagged.

**Note on the live save**: `build/Debug/save.txt` is still the
hand-edited maxed-stat (all six scores 18) Human Fighter built for the
Frostreaver verification trek (see Milestone 78). The user's actual
mid-game Fighter (EXP 967, day ~9 near Haven) was accidentally overwritten
by a throwaway Cleric test character in an earlier session and could not
be recovered. This maxed character remains the de facto live save unless
the user starts over.

Next backlog candidates (not started, not committed) -- see
`docs/MILESTONES.md`'s NEXT UP: an instant-defeat spell's interactive
verification (item 1), real Draconian mechanics (item 2), or an
SFML-backed rendering revisit with real sprite art (item 3).
