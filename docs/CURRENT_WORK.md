# Current work

Nothing in flight.

Milestone 47 (the ridge farewell) shipped: research into the actual
end-of-book scene (`.research/dosd_full.txt` lines ~15165-15290)
corrected Milestone 46's own NEXT UP pitch -- only Caramon (with Tika,
untracked) is actually headed to Solace; Tanis is confirmed returning to
Kalaman instead; Tasslehoff is off to his own never-named, unmodeled
kender homeland. None of it happens anywhere but the Neraka ridge
itself, so no new `PRESENCE`/`LOCATION` was added -- instead three new
`TOPIC` entries went into the existing `PRESENCE neraka 105 107` window:
Tanis's "A Ring of Gold and Steel" (the real ring-exchange/
reconciliation beat with the unnamed elfwoman), Caramon's "Going Home",
and Tasslehoff's "A Hero's Welcome". Verified via a clean rebuild (zero
new warnings, pure data content, no throwaway self-test needed) and the
piped smoke test; docs updated (`docs/TIMELINE_NOTES.md`,
`docs/MILESTONES.md`). Not yet committed -- ask the user before
committing.

Next: pick from `docs/MILESTONES.md`'s "NEXT UP":

1. **Terrain-specific monster pools** -- encounter chance already varies
   by terrain (Milestone 27); which monster you fight is still
   uniform-random. See `docs/COMBAT_NOTES.md`'s "Extending this later."
2. **More monsters** -- Bozak/Sivak/Aurak Draconians, Thanoi, and other
   untouched Monstrous Manual entries. Higher-tier draconians are
   spellcasters/shapeshifters -- real mechanics not modeled yet. See
   `docs/COMBAT_NOTES.md`'s "Extending this later."

Or something else -- ask the user before starting.
