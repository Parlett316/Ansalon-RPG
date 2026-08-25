# Current work

Nothing in flight.

Milestones 83-84 (2026-08-25) both shipped from the same session, both
prompted directly by the user hitting gaps in play: Milestone 83 added
town-proximity monster gating (`MIN_TOWN_DISTANCE`, `ONLY_TERRAIN`) so
Ogre/high-tier Draconians stay away from Solace and Thanoi is glacier-only;
Milestone 84 zeroed shallow water's leftover 5% encounter chance, which was
ambushing the player with land monsters while sailing. Both verified via
clean `/W4` rebuilds and the piped smoke test (real save protected each
time). See `docs/MILESTONES.md` for full writeups and
`docs/COMBAT_NOTES.md`'s "Town-proximity monster pools"/"Encounters"
sections for what shipped and what's still deliberately left alone.
Interactive confirmation (walking/sailing and watching which monsters turn
up) still needs the user's own keyboard.

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
