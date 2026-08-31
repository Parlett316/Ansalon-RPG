# Current work

Nothing in flight. Milestone 126 (Atlas Chronology Re-sync) is
implemented, documented, and verified -- see `docs/MILESTONES.md` entry
126 and `docs/TIMELINE_NOTES.md`'s "Atlas Chronology Re-sync (Milestone
126)" for the full history and the complete before/after day-number
tables.

Re-derived every `PRESENCE`/`SUBJECT_WHEN` day-boundary in
`data/timeline.txt` past `pax_tharkas 10 12` from `TSR 8448 The Atlas of
the Dragonlance World.pdf`'s real day-by-day chronology (pp.137-139),
replacing what had been explicit, disclosed guesses. The schedule now
spans ~195 days instead of ~107, at the user's explicit choice of the
Atlas's real absolute day counts over a compressed rescale. Pure data +
doc change, no `.cpp`/`.h` touched.

Verified via `--check-timeline` (zero new keyword-collision warnings
beyond the one pre-existing, already-documented Raistlin/Kitiara
override) and a clean `/W4` rebuild (zero new warnings, no code diff).
**Not yet interactively verified** in a real playthrough -- all three
save slots (Mason Day 5, Mike Day 4, Regan Day 7) were occupied this
session, so the piped smoke test couldn't reach character creation
either. Worth noting for whoever plays next: Regan (Day 7) was
previously inside the old `qualinesti 7 9` window and is now in a gap
(new `qualinesti` starts Day 10) -- not a bug, just the disclosed cost of
correcting the pacing (see the Milestone 126 writeup's "A new, honest gap
this correction creates").

Six real Atlas-named locations (Que-shu, Hopeful Vale, Skullcap,
Qualimori, Dragon Mountain, Mount Nevermind) were found but deliberately
not built this pass -- flagged as NEXT UP item 7, since each needs real
map-placement/zone work, not a day-renumbering change.
