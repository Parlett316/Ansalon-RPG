# Current work

Nothing in flight. Milestone 125 (sourced `GROUP` data for Thanoi, Ice
Bear, and Giant Spider) is implemented and documented -- see
`docs/MILESTONES.md` entry 125 for the full history.

Re-verified all 12 of Milestone 113's still-solo monsters' real No.
Appearing data against rendered rulebook page images. Two (Owlbear,
Ettin) turned out to print a solitary number for the real wandering
encounter and are now permanently solo, not just deferred. Of the
remaining 10, the user chose to keep the 7 `MIN_TOWN_DISTANCE`-gated
danger-tier monsters (Ogre, Kapak, Bozak, Sivak, Aurak, Wight, Troll)
solo for now, shipping only the 3 that carry no distance gate --
`data/monsters.txt` gained a new `GROUP 1 4` line for Thanoi, Ice Bear,
and Giant Spider each. Pure data change, no C++ touched.

Verified via a clean `/W4` rebuild (zero new warnings) and reaching the
save-slot menu, which loads `MonsterCatalog` (and therefore all three
edited blocks) before it renders. **Not yet interactively verified**: all
three save slots (Mason, Mike, Regan) were occupied again this session,
so the piped test couldn't reach character creation either, and there's
no piped path to a live combat encounter regardless (`_getch()` blocks
that). Seeing an actual Thanoi/Ice Bear/Giant Spider band in a real fight
still needs the user's own keyboard -- Ice Bear and Giant Spider are the
easiest to reach (Icewall Glacier tiles for Ice Bear; most forest/bog
terrain for Giant Spider), Thanoi needs the same glacier terrain as Ice
Bear.
