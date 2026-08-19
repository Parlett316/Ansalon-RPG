# Current work

One small fix is implemented and self-verified, but **not yet confirmed
in a real, human-at-the-keyboard playthrough** — combat can't be tested
headlessly (see `docs/GOTCHAS.md`):

- **"Press any key to continue." on combat end screens**: fleeing,
  winning, and being knocked out each used to redraw the final combat
  frame and silently block on a keypress with no visual cue one was
  expected. All three (`GameLoop::runCombat`, `src/game/GameLoop.cpp`)
  now push a "Press any key to continue." log line first. Confirm it
  reads clearly in all three cases (flee, victory, knockout) and doesn't
  look out of place in the log panel.

What's already done: a clean `/W4` rebuild (zero new warnings, only
`GameLoop.cpp` touched) and the piped character-creation smoke test.

Next step: trigger a flee, a win, and a knockout in a real playthrough
and confirm the new line. Once confirmed, clear this file back to
"nothing in flight" and offer the next backlog menu from
`docs/MILESTONES.md`'s "NEXT UP" (terrain-specific monster pools, more
monsters, or continuing *Dragons of Spring Dawning* with Godshome/Neraka).
