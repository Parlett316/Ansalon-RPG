# Current work

Nothing in flight.

Milestone 74 (showing the AD&D 2nd ed math behind attacks) just shipped --
see `docs/MILESTONES.md`'s Milestone 74 entry and `docs/COMBAT_NOTES.md`'s
"Showing the math" section for the full mechanism. In brief:
`combat::AttackOutcome` now exposes the roll breakdown
`resolvePlayerAttack`/`resolveMonsterAttack` already computed internally
(natural roll, to-hit bonus, THAC0/AC/target number, damage dice/roll/
bonus) -- hit/miss/damage logic itself is unchanged, just no longer thrown
away -- and new `game::describeToHit`/`describeDamage` in `GameLoop.cpp`
render it as a bracketed suffix on the existing combat-log hit/miss line.
`drawCombatFrame`'s `kMaxLogLines` was trimmed 12->8 to compensate for
entries now typically wrapping to two lines. Verified via a throwaway
self-test (10,000 rolls, deleted after passing), a clean `/W4` rebuild
(zero new warnings), and the piped smoke test. **Interactive verification
still needs the user's own keyboard** -- seeing the actual bracketed math
render correctly in a real fight, in a real terminal.
