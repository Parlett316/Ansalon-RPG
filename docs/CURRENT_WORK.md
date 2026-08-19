# Current work

Nothing in flight. All completed milestones are documented in the
relevant `docs/*_NOTES.md` files and `README.md`'s Status paragraph — see
`CLAUDE.md` for which doc covers which system.

Most recent milestone: Milestone 42 (healing potions). See
`docs/MILESTONES.md` for the full writeup and `docs/CHARACTER_NOTES.md`'s
"Potions" / `docs/COMBAT_NOTES.md`'s "Player actions" section for
sourcing.

**Partially confirmed live**: the user has seen the potion listed at a
shop in-game. Not yet explicitly exercised: actually drinking one (either
from the inventory screen, confirming the new HP line updates, or
mid-combat with `i`, confirming it consumes the round and the monster
still attacks), pressing `i` with no potions carried, and selling one
back. Worth a real pass next time you're at the keyboard — `_getch()`
can't be piped (see `docs/GOTCHAS.md`), so none of this can be scripted.

Next step: nothing committed to yet. Backlog candidates (see
`docs/MILESTONES.md`'s "NEXT UP"): terrain-specific monster pools, more
monsters, or continuing *Dragons of Spring Dawning* at Palanthas/
Godshome/Neraka. Ask the user which to start next, per the project's
standing workflow.
