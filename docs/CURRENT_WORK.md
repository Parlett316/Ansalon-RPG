# Current work

Nothing in flight.

Milestone 46 (Neraka -- the war's climax and ending) shipped: a new
`LOCATION neraka`, a fully-walled four-POI zone, and a `PRESENCE neraka
105 107` window for Tanis, Caramon, and Tasslehoff, each closing a
thread from an earlier milestone (Tanis's "The Man Who Wouldn't Die"
from Godshome, Caramon's "The Brother He Can't Watch" from Kalaman, and
Tasslehoff's grief over Flint from Godshome). Kitiara, Laurana, Ariakas,
and Lord Soth stay off-stage per this project's established precedent;
Raistlin's own tracked schedule is untouched even though Caramon's
dialogue describes a real reunion with him. Verified via a throwaway
self-test (21 assertions), a clean rebuild, and the piped smoke test;
docs updated. Not yet committed -- ask the user before committing.

Next: pick from `docs/MILESTONES.md`'s "NEXT UP":

1. **Solace, going home** -- the book's own final scene has the
   survivors talk about "going back to Solace." No new zone or
   `LOCATION` needed, just a closing `PRESENCE solace` window for Tanis,
   Caramon, and Tasslehoff -- the natural, lowest-effort next stop.
2. **Terrain-specific monster pools** -- encounter chance already varies
   by terrain (Milestone 27); which monster you fight is still
   uniform-random. See `docs/COMBAT_NOTES.md`'s "Extending this later."
3. **More monsters** -- Bozak/Sivak/Aurak Draconians, Thanoi, and other
   untouched Monstrous Manual entries. Higher-tier draconians are
   spellcasters/shapeshifters -- real mechanics not modeled yet. See
   `docs/COMBAT_NOTES.md`'s "Extending this later."

Or something else -- ask the user before starting.
