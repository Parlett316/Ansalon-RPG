# Current work

Nothing in flight. All completed milestones are documented in the
relevant `docs/*_NOTES.md` files and `README.md`'s Status paragraph — see
`CLAUDE.md` for which doc covers which system.

Most recent milestone: Milestone 42 (healing potions). See
`docs/MILESTONES.md` for the full writeup and `docs/CHARACTER_NOTES.md`'s
"Potions" / `docs/COMBAT_NOTES.md`'s "Player actions" section for
sourcing.

**Still needs a live human playthrough** — `_getch()` can't be piped (see
`docs/GOTCHAS.md`), so the interactive keypress loop itself wasn't
exercised by this session. Next time you're at the keyboard: buy a potion
at any shop, drink it from the inventory screen (`i`, `Enter`) and confirm
the new HP line updates, buy another, pick a fight and drink it mid-combat
with `i`, confirm the round is consumed and the monster still gets its
attack, confirm pressing `i` with no potions carried logs the right
message without burning the round, and confirm selling a potion back at a
shop works.

Next step: nothing committed to yet. Backlog candidates (see
`docs/MILESTONES.md`'s "NEXT UP"): terrain-specific monster pools, more
monsters, or continuing *Dragons of Spring Dawning* at Palanthas/
Godshome/Neraka. Ask the user which to start next, per the project's
standing workflow.
