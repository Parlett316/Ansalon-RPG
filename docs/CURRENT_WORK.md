# Current work

Nothing in flight.

Milestone 69 (character creation redesign: screen-per-step, colorized,
Method V dice) just shipped -- see `docs/MILESTONES.md`'s Milestone 69
entry, `docs/CHARACTER_NOTES.md`'s "Ability score generation" section,
and `docs/ARCHITECTURE.md`'s matching Milestone 69 addendum for the full
design/rationale. Verified via a throwaway self-test
(`roll4d6DropLowest()` bounds and average), a clean `/W4` rebuild, and a
complete scripted piped run through the real executable end-to-end
(confirmed correct screens/colors/race-adjustment math, and that the
resulting character reaches the real game loop). Actually seeing the
colors in a real terminal still needs the user's own keyboard.
