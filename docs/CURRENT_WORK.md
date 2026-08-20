# Current work

Nothing in flight.

Milestone 57 (terrain-specific monster pools) just shipped: which monster a
random encounter draws now varies by terrain, via a real, sourced
Climate/Terrain hard exclusion (Gnoll only) plus clearly-flagged, invented
flavor weighting on top. Also fixed a real page-citation bug found along the
way (9 of 11 monster citations in `docs/COMBAT_NOTES.md` were off by +3).
See `docs/MILESTONES.md`'s Milestone 57 entry and `docs/COMBAT_NOTES.md`'s
"Terrain-specific monster pools" for the full design and sourcing.

**Not yet verified**: real interactive playthrough confirming terrain
visibly changes which monster shows up (e.g. Bugbears feeling more common in
hills/mountains, Gnolls never appearing on salt flat) -- `_getch()` can't be
piped, the same limitation flagged for every combat milestone so far.
Milestone 56's own interactive verification (buying/using a Webnet and
Brooch mid-fight) is also still outstanding from last session.

NEXT UP (`docs/MILESTONES.md`) now leads with the still-unused `DELIVER`
objective kind, then more monsters (Bozak/Sivak/Aurak Draconians, Thanoi,
etc.). Ask the user before starting either.
