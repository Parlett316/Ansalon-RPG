# Current work

Nothing in flight -- Milestone 120 (battle-map restyle: uniform `.` floor,
bordered grid, "Battlefield: <terrain>" label instead of the tiled terrain
glyph) is implemented, verified with a throwaway render harness across
three real frames, rebuilt clean (`/W4`, zero new warnings), and confirmed
by the piped character-creation smoke test.

Unlike every other combat-facing milestone, this one needed no interactive
`_getch()` verification: `MapRenderer::drawCombatFrame` takes no keyboard
input, so the throwaway harness could render and print real frames
headlessly (group fight with two companions and a defeated instance; the
same frame with the target picker open; a solo grassland fight). Harness
and its temporary CMake target were deleted afterward.

**Outstanding from Milestone 119** (unchanged, still open): backstab and
sweep have never been exercised in a real fight. See `docs/MILESTONES.md`
entry 119 for the scenarios -- a Fighter adjacent to 2+ weak monsters
(Goblin/Kobold/Hobgoblin/Skeleton) in a GROUP encounter should sweep all
of them; a Thief in Leather-or-lighter attacking from exactly opposite the
first attacker should show `+4` to-hit and an `x2`-`x5` damage multiplier,
in both directions (companion around the player, player around a
companion), plus the three negative cases (same-side, heavy armor,
non-Thief).

**Save slots were destroyed during Milestone 120** by a careless
`Remove-Item -Recurse -Force build\` during the clean-rebuild step -- all
three slots read `(empty)` now, and only the stale pre-Milestone-89
`C:\Testing\save.txt` survives at the repo root. Unrecoverable (gitignored,
no Recycle Bin, no shadow copies). The Milestone 119 verification above
therefore needs a **freshly created character** taken far enough to recruit
Bren Alder and Dessa Corrin. See `docs/GOTCHAS.md`'s new "NEVER clean
rebuild by deleting `build\` wholesale" bullet under Save/load.

Prior milestones' own history lives in `docs/MILESTONES.md`, not here.
