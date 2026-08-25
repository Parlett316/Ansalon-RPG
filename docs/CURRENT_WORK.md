# Current work

Nothing in flight.

Milestone 83 (town-proximity monster gating -- a new `MIN_TOWN_DISTANCE`
field keeps Ogre and the higher-tier Draconians (Kapak/Bozak/Sivak/Aurak)
away from Solace and the other civilian towns, and Thanoi's `TERRAIN_BIAS`
toward glacier was tightened to a hard `ONLY_TERRAIN` lock) shipped
2026-08-25, prompted directly by the user hitting the gap in play: too many
high-powered Draconians/Ogres turning up right outside Solace, and Thanoi
appearing outside its sourced Icewall Glacier habitat. Real engine change
(`combat::Monster`/`MonsterCatalog`/`MonsterLoader`, one new
`GameLoop::tryMoveOverworld` distance computation) plus a `data/
monsters.txt` content pass -- verified via a throwaway self-test (16
assertions, deleted afterward), a clean `/W4` rebuild, and the piped smoke
test (real save moved aside and restored byte-identical afterward). See
`docs/MILESTONES.md` and `docs/COMBAT_NOTES.md`'s "Town-proximity monster
pools" for what shipped, the exact distance values, and what was
deliberately left alone. Interactive verification (actually walking near
vs. far from Solace and confirming which monsters turn up) still needs the
user's own keyboard, same limitation every prior combat/content milestone
has flagged.

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
