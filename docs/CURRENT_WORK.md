# Current work

Nothing in flight.

Milestones 85-87 (2026-08-25) all shipped from the same session. Milestone
85 redid the world map from a much higher-fidelity reference image
(`References/dragonlancemap2.png`), improving terrain classification and
re-deriving every location's `POS`. Milestone 86, prompted by locations
surfaced while re-reading labels for Milestone 85, added Thorbardin and
Sancrist Isle as real, walkable locations with full `PRESENCE`/`SAY`/
`TOPIC` content -- Thorbardin for all 8 Heroes (`13 19`, the *Dragons of
Winter Night* opening at the Hammer of Kharas ceremony), Sancrist Isle for
Sturm/Flint/Tasslehoff/Laurana (`55 60`, Sturm's Knights' Trial). Sancrist
Isle reverses a prior "deliberately not modeled" decision, done at the
user's explicit request after being shown the original reasoning.
Milestone 87 fixed roads whose straight-line paths cut across real open
water (the user spotted this on a rendered view of the map): two roads
(`xak_tsaroth`-`plains_of_dust`, `solace`-`silvanesti`) were removed
outright as bogus/redundant crossings of New Bay; a handful of single-
pixel classification-noise tiles on the other two flagged roads were
patched via a new `MANUAL_TERRAIN_OVERRIDES` dict in
`tools/generate_overworld.py`. `References/portcities.txt` was reviewed
per the user's request; no new port-city location was needed for this
specific fix (see `docs/MAP_NOTES.md`'s reasoning), with the broader
"which port cities deserve real content" question folded into
`docs/MILESTONES.md`'s NEXT UP item 4. See `docs/MAP_NOTES.md` and
`docs/TIMELINE_NOTES.md` for full sourcing and citations, and
`docs/MILESTONES.md` entries 85-87 for the complete writeups.

All three verified via clean `/W4` rebuilds, `--check-timeline` (no new
keyword-collision warnings), and the piped smoke test -- the live save
was found to have changed significantly partway through the 85/86 work
(the user was evidently playing concurrently), so a fresh backup was
taken before that verification pass rather than trusting the
session-start snapshot; the save was confirmed byte-identical
before/after every subsequent test, including Milestone 87's.

Next backlog candidates (not started, not committed) -- see
`docs/MILESTONES.md`'s NEXT UP: an instant-defeat spell's interactive
verification (item 1), real Draconian mechanics (item 2), an
SFML-backed rendering revisit with real sprite art (item 3), or more
locations from the Milestone 85 map re-read plus the Milestone 87
portcities.txt review -- Nordmaar, Schallsea, Northern/Southern Ergoth,
and any of `References/portcities.txt`'s other named ports are visible
on the map/reference file but not yet checked against the novels/
sourcebooks for book significance (item 4).

Also worth knowing for a fresh session: `git status` shows Milestones
85-87 (and their new files, `data/zones/thorbardin.txt` and
`data/zones/sancrist_isle.txt`) as uncommitted working-tree changes --
nothing has been committed to git yet, since no session this far has
been asked to commit.
