# Current work

Nothing in flight.

Milestone 99 (2026-08-26) gave Bozak/Sivak/Aurak Draconians real, sourced
combat abilities (Bozak's Magic Missile, Aurak's breath weapon, Sivak's
death-burst) instead of leaving them flavor-only, closing NEXT UP item 2.
Verified via a throwaway self-test, a clean `/W4` rebuild, the piped
smoke test, and now also a real interactive playthrough: the user fought
a live Bozak, Sivak, and Aurak (via a throwaway isolated test copy of the
exe/data, never touching the real save) and confirmed all three combat
log lines plus the Sivak burst-then-knockout edge case (XP/steel still
awarded even when the post-kill burst knocks the player out). See
`docs/MILESTONES.md` entry 99 and `docs/COMBAT_NOTES.md`'s "Draconian
roster" section.
