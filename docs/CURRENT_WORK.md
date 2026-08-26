# Current work

Nothing in flight.

Milestone 90 (2026-08-26) fixed a real softlock the user reported directly:
getting knocked out fighting the Thanoi the `frostreaver_salvage` quest
requires (hard-locked to the glacier around Ice Wall Castle since Milestone
83) sent the player to `nearestTown()`'s pick, Tarsis -- but the only way
back to Ice Wall Castle is a deliberately one-time `BOAT` voyage (Milestone
88), so Ice Wall became permanently unreachable afterward. Fixed by adding a
`SEA_LOCKED` flag to `data/locations.txt` (mirrors `TOWN`'s loader shape
exactly), tagging `ice_wall` with it, and widening what's now
`GameLoop::nearestRefuge()` (renamed from `nearestTown()`) to also accept
`seaLocked` locations. The one-time `BOAT` restriction itself was left
untouched, confirmed with the user first -- see `docs/MILESTONES.md` entry 90
for the full writeup, and `docs/COMBAT_NOTES.md`'s "Death: knocked out, not
killed" / `docs/MAP_NOTES.md`'s `data/locations.txt` grammar section for the
updated mechanics.

Verified via a throwaway self-test (confirmed `ice_wall.seaLocked` parses
`true` and is the only location with it set, then deleted), a clean `/W4`
rebuild (zero new warnings), and the piped character-creation smoke test
against an isolated scratch copy of the exe + `data/` -- the real
`build\Debug\save1.txt`/`save2.txt` (which include the user's actual live
character, "Mike," currently standing at Ice Wall Castle's own overworld tile
with the one-time boat already spent) were never touched, confirmed by
unchanged file timestamps before/after.

**Not yet done**: no interactive replay of the actual softlock scenario (get
knocked out on the glacier, confirm the game now wakes the character up at
Ice Wall Castle instead of Tarsis) -- `_getch()` can't be piped, so this
needs the user's own keyboard. Low risk: the fix is a straight-line-distance
comparison, and Ice Wall's `POS 150 305` is dramatically closer to itself
than Tarsis's `POS 197 274` is, so `nearestRefuge()` can't reasonably pick
Tarsis for a fight happening on Ice Wall's own glacier tiles. Also still
open from Milestone 89: the user hasn't yet played the save-slot menu
themselves with real keypresses (migration and the menu's non-interactive
prefix are confirmed; picking a slot and continuing into the world with real
input still needs their own hands-on check).
