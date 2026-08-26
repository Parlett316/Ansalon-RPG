# Current work

Nothing in flight.

Milestone 93 (2026-08-26) fixed a road the user spotted crossing open
water north of Solace: `("solace", "high_clerist_tower")`, drawn at
Milestone 35 before this project had a real map source, was never
actually checked against one. Cropping and grid-overlaying
`References/dragonlancemap2.png` around the Strait of Schallsea found no
drawn road crosses it anywhere -- only coastal roads meeting a real,
labeled ferry town in the middle of the strait, "Crossing." Removed the
road from `ROAD_PAIRS` and regenerated `data/overworld.grid`; added
`LOCATION crossing` (`POS 200 185`, sourced from the map's own label) and
a small new zone (`data/zones/crossing.txt`, one generic Ferry Keeper
NPC, no timeline content -- none of the three novels mention the place).
Resolved with the user before building: Crossing stays a plain,
road-free, foot-reachable waypoint, not a scripted `BOAT` ferry -- the
strait's shallow water was already boat-free by design since Milestone
87, so nothing needed the sea-travel mechanism. Pure data content, zero
`.cpp`/`.h` changes. See `docs/MILESTONES.md` entry 93 for the full
writeup, and `docs/MAP_NOTES.md`'s "Crossing" section / `docs/
ZONE_NOTES.md`'s "Crossing" section for the sourcing and zone detail.

Verified via a throwaway BFS script (all remaining `ROAD_PAIRS`
connections intact end-to-end, every location including Crossing itself
still foot-reachable from Solace, zero road tiles left crossing the
strait -- script written, run, and deleted per the project's standing
convention), a clean `/W4` rebuild (zero new warnings), and the piped
smoke test. The user's real `save1.txt`/`save2.txt`/`save3.txt` were
moved aside before the smoke test and restored after -- timestamps and
content confirmed unchanged.

**Not yet done**: no interactive playtest of the new location (walking
from Solace to Crossing, confirming the Ferry Keeper's dialogue/topics,
confirming the Tower is still reachable beyond it) -- `_getch()` can't be
piped, so this needs the user's own keyboard, same limitation every
content milestone has flagged.
