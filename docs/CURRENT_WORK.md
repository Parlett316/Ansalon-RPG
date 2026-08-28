# Current work

Nothing in flight -- Milestone 119 (Thief backstab and Fighter sweep
attacks) is implemented and rebuilt clean (`/W4`, zero new warnings), with
a 26-assertion throwaway self-test passing and the piped character-
creation smoke test confirming nothing else broke loading.

**Interactive verification still open** (same `_getch()` limitation as
every other combat-facing milestone -- neither ability can be exercised
headlessly):

- **Sweep**: a Fighter-type character (the player, or Bren Alder) adjacent
  to 2+ weak monsters (Goblin, Kobold, Hobgoblin, or Skeleton -- the only
  roster entries with `hpDiceCount <= 1`) in a GROUP encounter attacks all
  of them in one round instead of opening the normal target picker. Log
  should read "You sweep through the Goblin Raiders!" (or the companion's
  own name) followed by one hit/miss line per adjacent weak instance, no
  to-hit/damage bonus.
- **Backstab**: with two party members (the player and a companion, or
  vice versa) both adjacent to the same monster instance from exactly
  opposite grid sides, and the second attacker a Thief-type
  (`ClassId::Thief`) wearing no armor heavier than Leather, the second
  attacker's hit should show `[... +4 = ...]` in the to-hit math and an
  `x2`/`x3`/`x4`/`x5` (by level) multiplier in the damage math, with a
  "Backstab! " prefix on the hit line. Confirm both directions: a
  companion (Bren Alder or Dessa Corrin) backstabbing around the player's
  engagement, and a Thief player backstabbing around a companion's.
  Also confirm the negative cases: same-side positioning (no bonus),
  armor heavier than Leather equipped (no bonus even when positioned
  correctly), and a non-Thief attacker positioned correctly (no bonus).

See `docs/MILESTONES.md` entry 119 for the full design writeup, and
`docs/COMBAT_NOTES.md`'s "Thief backstab and Fighter sweep attacks" /
`docs/CHARACTER_NOTES.md`'s "Party companions" and "Leveling / experience"
sections for how the system works. Prior milestones' own history lives in
`docs/MILESTONES.md`, not here -- see that file for the full numbered
writeup of everything shipped before this.
