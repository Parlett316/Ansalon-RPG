# Current work

Nothing in flight.

Milestone 63 (key-binding cleanup) just shipped: on-screen movement
bindings are now `wasd`-only -- the `hjkl` vi convention and the `yubn`
roguelike diagonals are gone, at the user's request. Two same-session
follow-ups: Look moved from `;` to `l`, and arrow keys were restored as a
silent, on-screen-undocumented alias for `wasd` (Windows only). See
`docs/MILESTONES.md`'s Milestone 63 entry for the full writeup. Verified
via a clean `/W4` rebuild and the piped character-creation smoke test;
not yet interactively played with a real keyboard (`_getch()` can't be
piped, same limitation every UI-touching milestone has flagged), but the
change is pure input-mapping deletion with no save-format or gameplay-
logic impact, so that's a nice-to-have confirmation rather than a
blocker.
