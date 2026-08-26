# Current work

Nothing in flight.

Milestone 91 (2026-08-26) closed the Sancrist Isle sea-voyage gap Milestone
88 deliberately left open: converting `GameState::hasBoat` into a scripted,
point-to-point `BOAT` voyage had converted only the Tarsis -> Ice Wall Castle
leg, leaving Sancrist Isle (which relied on that same removed global flag)
reachable only by a long coastal foot-walk. A fresh read of
`.research/dwn_full.txt` supplied real sourcing for the second leg: the
historical party escaped Ice Wall Castle's collapse "with the help of the
Ice Barbarians" (line 5704), and the ship's captain estimated "Sancrist in
two days" if the wind held (lines 5919-5920) -- an actual sourced duration,
unlike the first leg's invented-for-pacing 48 hours. Added a new POI,
`B "An Ice Barbarian Guide"`, to `data/zones/ice_wall.txt`, granting
`BOAT B sancrist_isle 48` -- deliberately not the zone's existing Young
Knight (`K`), who's already that zone's `frostreaver_salvage` quest-giver.
Pure data content, no `.cpp`/`.h` changes. See `docs/MILESTONES.md` entry 91
for the full writeup, and `docs/ARCHITECTURE.md`'s "Sea travel" /
`docs/ZONE_NOTES.md`'s "Boats" for the updated mechanics.

Verified via a throwaway self-test (14 assertions -- confirmed the new POI
parses at its intended tile with all its dialogue/subject lines, its
`BoatVoyage` resolves to `sancrist_isle` at 48 hours, the zone's `ENTRY`
tile itself still carries no POI, and the existing Young Knight is
untouched), a clean `/W4` rebuild (zero new warnings), and the piped
character-creation smoke test against an isolated scratch copy of the exe +
`data/` -- the real `build\Debug\save1.txt`/`save2.txt` were never touched
(confirmed unchanged timestamps).

**Not yet done**: no interactive replay of the actual jump (talk to the new
Ice Barbarian Guide at Ice Wall Castle, confirm arrival at Sancrist Isle,
the log line, and the 48-hour clock advance) -- `_getch()` can't be piped,
so this needs the user's own keyboard, same limitation flagged for every
prior `BOAT` milestone. Two more interactive-verification items remain open
from earlier milestones too (see `docs/MILESTONES.md`'s NEXT UP #5/#6): the
Milestone 88 Tarsis -> Ice Wall jump itself, and the Milestone 89 save-slot
menu's real-keypress path.
