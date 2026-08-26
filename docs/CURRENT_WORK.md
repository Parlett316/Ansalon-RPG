# Current work

Nothing in flight.

Milestone 95 (2026-08-26) added Southern Ergoth: closes a previously-
unexplained gap in Sturm/Flint/Tasslehoff/Laurana's schedule
(`ice_wall 38 42` jumped straight to `sancrist_isle 55 60`) and corrects
an error in `docs/TIMELINE_NOTES.md`'s own Ice Wall section, which had
claimed the party never lands there. New `LOCATION southern_ergoth`
(`SEA_LOCKED`, `POS 91 193`) and a new sparse zone (wrecked shore,
talkable Silvanesti Sentry, Wilder Elves' Camp `TIMELINE_ANCHOR`). New
`PRESENCE southern_ergoth 43 50` windows for Sturm, Flint, Tasslehoff,
Laurana, and Fizban (his existing `CHARACTER` block), plus a new full
`CHARACTER silvara` block. `data/zones/ice_wall.txt`'s existing "Ice
Barbarian Guide" `BOAT` was repointed from `sancrist_isle` to
`southern_ergoth` (same POI char, so `voyagesTaken` stays correct for
saves that already took it), with a new POI granting the continuation on
to Sancrist Isle. Verified via a throwaway self-test (25 assertions), a
clean `/W4` rebuild, and the piped smoke test with the user's real saves
confirmed byte-identical afterward. Interactive verification (walking the
new leg, talking to the new POIs) still needs the user's own keyboard.
See `docs/MILESTONES.md` entry 95, `docs/MAP_NOTES.md`/
`docs/TIMELINE_NOTES.md`/`docs/ZONE_NOTES.md`'s "Southern Ergoth"
sections, and `docs/ARCHITECTURE.md`'s "Sea travel" section.

**Related, not started**: `docs/MILESTONES.md`'s NEXT UP item 5, Flotsam
(+ Port Balifor) -- a second real gap of similar size, found by the same
research pass, closing Tanis/Raistlin/Caramon/Goldmoon/Riverwind's own
`silvanesti 25 30` -> `palanthas 83 83`/`kalaman 100 100` jump. Needs its
own research-and-plan pass before building.
