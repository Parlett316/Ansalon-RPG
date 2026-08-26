# Current work

Nothing in flight.

Milestone 99 (2026-08-26) gave Bozak/Sivak/Aurak Draconians real, sourced
combat abilities (Bozak's Magic Missile, Aurak's breath weapon, Sivak's
death-burst) instead of leaving them flavor-only, closing NEXT UP item 2.
Verified via a throwaway self-test, a clean `/W4` rebuild, and the piped
smoke test. Interactive verification (actually fighting one of the three
in a real playthrough, to confirm the new combat log lines and the
burst-then-knockout edge case) still needs the user's own keyboard --
worth doing before the next combat-touching change, more than usual since
this milestone changed live combat math rather than just data. See
`docs/MILESTONES.md` entry 99 and `docs/COMBAT_NOTES.md`'s "Draconian
roster" section.
