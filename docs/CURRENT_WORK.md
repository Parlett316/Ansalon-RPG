# Current work

Nothing in flight.

Milestone 67 (Movement granularity, hours -> minutes) just shipped --
`world::TerrainInfo::hoursToCross` (flat whole-hour terrain cost) renamed to
`minutesToCross` and rescaled x15 into a new `GameState::minutesElapsed`
remainder (0-59, rolled into the existing `hoursElapsed` on overflow in
`GameLoop::tryMoveOverworld`), fixing user-reported feedback that ordinary
play was consuming the Heroes' schedule far faster than intended -- even
the cheapest terrain used to cost a full hour per keypress against
Solace's 48-hour opening window. `data/timeline.txt`'s day windows and
Rest's flat 8-hour cost are both deliberately untouched -- see
`docs/MILESTONES.md`'s Milestone 67 entry and `docs/MAP_NOTES.md`'s
"Movement granularity" section for the full design/rationale. New optional
`MINUTES <n>` save line (same backward-compatible shape as
RESTDAY/BROOCHDAY/STAFFCUREDAY). Verified via a throwaway self-test
(rollover arithmetic, save/load round-trip, pre-milestone save missing
MINUTES still loading), a clean `/W4` rebuild, and the piped smoke test.
Interactive verification (confirming the HUD's Hour only ticks up after
~4 road-tile moves, and that a mountain tile costs 1h30m) still needs the
user's own keyboard.
