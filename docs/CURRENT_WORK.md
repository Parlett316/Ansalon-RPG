# Current work

Nothing in flight.

Milestone 92 (2026-08-26) fixed two gaps the Milestone 91 interactive
playtest surfaced: talking to a `BOAT`-granting NPC executed the voyage
unconditionally (no way to decline), and Sancrist Isle -- reachable since
Milestone 91 -- had no talkable NPC and no route out, a genuine dead end.
Added `GameState::voyagesTaken` (persisted as a new `VOYAGED` save line)
so declining doesn't burn the offer, a Board/"Not yet" picker in
`GameLoop::talkTo` mirroring the quest Accept/Decline picker, and fixed
the travel log's hardcoded "carries you south" (wrong for two of the three
legs) to use the existing `compassDirection` helper. Closed the Sancrist
dead end with a third `BOAT` leg, `data/zones/sancrist_isle.txt`'s new
`E "An Embarkation Officer"` granting `BOAT E palanthas 96`, sourced from
*Dragons of Winter Night*'s account of Sturm's army sailing from Sancrist
to Palanthas. See `docs/MILESTONES.md` entry 92 for the full writeup, and
`docs/ARCHITECTURE.md`'s "Sea travel" / `docs/ZONE_NOTES.md`'s "Boats" and
"Sancrist Isle" for the updated mechanics.

Verified via a throwaway self-test (23 assertions -- the new POI's dialogue/
`BOAT` lines, the existing Runner/Guide voyages untouched, a `VOYAGED`
`SaveGame` round-trip, backward compatibility with a save that has no
`VOYAGED` line, and a malformed-count fail-fast case), a clean `/W4`
rebuild (zero new warnings), and the piped smoke test. Also directly
confirmed the user's real `save1.txt`/`save2.txt` still load and describe
themselves correctly in the save-slot menu under the new `VOYAGED`
keyword -- timestamps unchanged throughout.

**Not yet done**: no interactive replay of either change (talk to the
Tarsis Runner or the new Sancrist Embarkation Officer, decline once and
confirm the topics menu is still reachable, then accept and confirm the
jump/direction word/clock advance) -- `_getch()` can't be piped, so this
needs the user's own keyboard. See `docs/MILESTONES.md`'s NEXT UP #1 for
the exact checklist.
