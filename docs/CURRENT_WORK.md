# Current work

Nothing in flight. All completed milestones are documented in the
relevant `docs/*_NOTES.md` files and `README.md`'s Status paragraph — see
`CLAUDE.md` for which doc covers which system.

Most recent milestone: Milestone 39 (Kalaman) — the first *Dragons of
Spring Dawning* content, and the biggest single content milestone to date.
New `LOCATION kalaman` (`POS 262 73`, `REGION Estwilde`, placed by
cropping the reference map and cross-checking against the High Clerist's
Tower's known position), a new Tarsis-scale zone, and `data/timeline.txt`
windows for six of the eight Heroes across two sourced story beats: the
Spring Dawning festival (`kalaman 90 92`, Flint and Tasslehoff) and the
reunion with Tanis/Caramon/Goldmoon/Riverwind under a Dragon Highlord's
ultimatum (`kalaman 100 100`, all six). Raistlin gets no window here — he
splits off to Palanthas via the dragon orb in this arc. The new road
required regenerating `data/overworld.grid`, which wiped and then had
reapplied (byte-for-byte verified) Ice Wall's hand-painted glacier patch,
same as Milestone 37. Verified via a throwaway self-test, a visual grid
check, a clean rebuild (zero new warnings), and the piped smoke test. See
`docs/MAP_NOTES.md`, `docs/ZONE_NOTES.md`, `docs/TIMELINE_NOTES.md`.

Next step: per `docs/MILESTONES.md`'s "NEXT UP", the natural continuation
of *Dragons of Spring Dawning* is Palanthas, Godshome, or Neraka (in
roughly that order of fit) — nothing has been built for any of them yet.
Ask the user which to start next, per the project's standing workflow.
