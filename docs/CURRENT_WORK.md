# Current work

Nothing in flight.

Milestone 98 (2026-08-26) added Dargaard Keep, closing the real,
still-open gap Milestone 96 left behind: Laurana is captured between her
`kalaman 90 92` and `neraka 105 107` windows, but nothing staged it --
only referenced obliquely afterward (Flint's existing `kalaman 100 100`
dialogue, Laurana's own `neraka 105 107` lines). A fresh, direct re-read
of `.research/dosd_full.txt` -- not the existing doc summary, which
turned out to conflate this scene with Flotsam, corrected in the same
session -- confirmed a real, three-Hero, on-page scene: a forged letter
lures Laurana, Flint, and Tasslehoff out of Kalaman the night after the
festival with a false claim that Tanis is dying at Dargaard Keep; the
dragonarmy officer Bakaris (freed as part of the trade) turns on them in
a forest clearing short of the keep itself; Tasslehoff kills him, and an
ancient, spectral Knight of Solamnia -- confirmed to be Lord Soth, but
never named to Flint or Tas on-page -- paralyzes them both and carries
Laurana off toward Neraka, already modeled. New `LOCATION dargaard_keep`
(`POS 258 74`, placed by cropping the reference map the same region
Milestone 39 used for Kalaman, then confirmed against the live
`data/overworld.grid`'s own mountain cluster rather than trusted from the
image crop alone -- a real image-vs-grid discrepancy was found and
documented in `docs/MAP_NOTES.md`) and a new, sparse zone (a clearing, a
cave mouth, the keep itself visible only as a distant silhouette -- no
interior, since no tracked Hero is ever shown conscious inside the keep
in the source text). `PRESENCE dargaard_keep 93 93` added for Flint and
Tasslehoff only; Laurana deliberately gets no window there (now
confirmed, not deferred -- see the correction folded into
`docs/TIMELINE_NOTES.md`'s existing Laurana section). Verified via a
throwaway self-test (34 assertions), a clean `/W4` rebuild, and the piped
smoke test; no live save currently exists in `build/Debug` to risk, and
the one stale `save.txt` at the repo root (not on the executable's own
resolved data path) was confirmed byte-identical regardless. Interactive
verification (walking the new road, finding Flint and Tasslehoff at the
new location) still needs the user's own keyboard, same limitation every
prior milestone has flagged. See `docs/MILESTONES.md` entry 98 and the
"Dargaard Keep" sections in `docs/MAP_NOTES.md`, `docs/ZONE_NOTES.md`,
and `docs/TIMELINE_NOTES.md`.
