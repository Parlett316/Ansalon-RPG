# Current work

Nothing in flight. All completed milestones are documented in the
relevant `docs/*_NOTES.md` files and `README.md`'s Status paragraph — see
`CLAUDE.md` for which doc covers which system.

Most recent milestone: Milestone 38 (the siege of the High Clerist's Tower
and Sturm's death) — the bridging event between *Dragons of Winter Night*
(this project's existing content) and *Dragons of Spring Dawning* (the
next arc), needed because Spring Dawning opens with Sturm already dead.
Sourced directly from `Dragons_of_Winter_Night_-_Margaret_Weis.pdf`. Pure
data change: a single-day `PRESENCE high_clerist_tower 81 81` window added
for Sturm, Flint, and Tasslehoff in `data/timeline.txt`. Sturm's window has
no `SAY` (the first deliberate use of "no SAY = not talkable" to represent
a character dying in the narrated scene) and is his schedule's permanent
last window. The Dragon Highlord who kills him (Kitiara, in the source)
stays unnamed, same precedent as Alhana/Derek/Gunthar. Verified via a
throwaway self-test, a clean rebuild (zero new warnings, no source
changes), and the piped smoke test. See `docs/TIMELINE_NOTES.md`.

Next step: per user direction (2026-08-18), a Spring Dawning research pass
was already completed and Kalaman was chosen as the next walkable zone —
see `docs/MILESTONES.md`'s "NEXT UP" for the full research summary
(Kalaman's scope, and why Palanthas/Godshome/Neraka are the follow-on
candidates while Flotsam/Vingaard Keep/Dargaard Keep/sunken Istar/Sanction
stay flavor-only). Nothing has been built for Kalaman yet — that's the
next session's starting point.
