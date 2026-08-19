# Current work

Three small UX/rules fixes are implemented and self-verified, but **not
yet confirmed in a real, human-at-the-keyboard playthrough** — `_getch()`
can't be piped (see `docs/GOTCHAS.md`):

- **Nearest-town death respawn**: getting knocked out (HP to 0) in combat
  should now fully heal the player and send them to the nearest of Solace/
  Haven/Kalaman/Tarsis/Palanthas (whichever is closest to where they fell),
  not always Solace at 1 HP. Confirm the combat-log and side-log messages
  name the right town, and that HP is actually full afterward. See
  `docs/COMBAT_NOTES.md`'s "Death: knocked out, not killed".
- **`?` help screen**: pressing `?` on the overworld or in a zone should
  open a bordered screen listing every command, dismissed by any keypress.
  Confirm it opens/closes cleanly and the listed bindings are accurate.

Also done this pass, and verifiable headlessly (already confirmed via the
piped character-creation smoke test): character names are now capped at
20 characters, reprompting on anything longer.

Milestone 44 (Palanthas) is also implemented and self-verified, but **not
yet confirmed in a real, human-at-the-keyboard playthrough**:

- Walk from the High Clerist's Tower to the new Palanthas tile and confirm
  the overworld announce reads correctly.
- Enter the zone (40×16) and confirm it renders cleanly in the Milestone 43
  layout, both at a comfortable terminal size and near the 72×20 floor.
- Stand on the Great Library tile on day 83 specifically and exercise
  **both** of Milestone 43's pickers with real multi-candidate content for
  the first time: Talk should list Astinus/Flint/Tasslehoff (3), Look
  should list Raistlin/Flint/Tasslehoff (3, since Look is unfiltered and
  Astinus isn't a timeline `Presence`) — confirm both list the right names
  and show the right text on selection.
- Revisit days 84-89 and confirm Raistlin has silently dropped out of both
  pickers while Flint/Tasslehoff remain, and that their `SAY_AGAIN`/
  `TALK_AGAIN` fire correctly on a second visit.
- Confirm the existing High Clerist's Tower and Kalaman content are
  unaffected (this milestone touched the shared `overworld.grid` and
  `timeline.txt` files).

What's already done: the new `LOCATION palanthas`/zone/timeline content, a
`("high_clerist_tower", "palanthas")` road (Ice Wall's glacier patch
captured before regenerating and diff-confirmed byte-for-byte identical
afterward), a throwaway self-test (17 assertions covering location/zone
loading and `Timeline::presentAt` across the day boundaries, then deleted),
a clean `/W4` rebuild (zero new warnings, no `.cpp`/`.h` changes), and the
piped smoke test. Also since: a road 4-connectivity fix (`docs/MAP_NOTES.md`'s
"Road 4-connectivity fix") that resolved a real blocker for this
playthrough — the player got stuck at tile `(174,106)`, unable to reach
the Tower, because a diagonal-only road segment isn't walkable with
4-directional-only movement. All 12 `ROAD_PAIRS` were regenerated and
reverified; the path to Palanthas should now be walkable start to finish.

See `docs/MILESTONES.md` entry 44 and `docs/MAP_NOTES.md`/
`docs/ZONE_NOTES.md`/`docs/TIMELINE_NOTES.md`'s "Palanthas (Milestone 44)"
sections for the full writeup.

Next step: have the user play for a bit and confirm all the items above
(the nearest-town respawn and help screen, plus the still-pending
Palanthas checklist), per CLAUDE.md's session workflow. Once confirmed,
clear this file back to "nothing in flight" and offer the next backlog
menu from `docs/MILESTONES.md`'s "NEXT UP" (Godshome and Neraka are now
the two remaining *Dragons of Spring Dawning* candidates, alongside
terrain-specific monster pools and more monsters).
