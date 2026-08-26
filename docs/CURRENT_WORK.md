# Current work

Nothing in flight.

Milestone 100 (2026-08-26) added three more monsters -- Owlbear, Wight,
Troll -- bringing the roster to 20, picked from `docs/COMBAT_NOTES.md`'s
"Extending this later" backlog after Milestone 99 left the NEXT UP list
otherwise exhausted. Pure data addition to `data/monsters.txt`, sourced
from visually-confirmed Monster Manual page images, no `.cpp`/`.h`
changes. Verified via a clean `/W4` rebuild and the piped smoke test
(reaches the save-slot menu cleanly, proving `MonsterCatalog` parsed the
file end-to-end); no interactive playtest needed since nothing new is
gated behind a special mechanic requiring live confirmation, same
reasoning as every prior pure-content monster milestone. See
`docs/MILESTONES.md` entry 100 and `docs/COMBAT_NOTES.md`'s "Accuracy:
what's sourced, what's invented" section.
