# Current work

Nothing in flight. Milestone 129 (bigger combat battlefield -- the
tactical grid bumped from 11x7 to 15x9) is implemented and documented --
see `docs/MILESTONES.md` entry 129 and `docs/COMBAT_NOTES.md`'s
"Positional combat grid" section for the sizing math.

**Not yet interactively verified** -- `_getch()` can't be piped, so the
actual in-game combat screen at the new size (roster layout, target-picker
brackets, monster spread across a wider/taller grid) hasn't been read back
by a human yet. Worth doing on the next real fight; if anything looks off
(too big, too small, awkward roster/log layout), the fix is just tuning
`render::MapRenderer::kCombatGridWidth`/`kCombatGridHeight` again -- both
are read dynamically everywhere else, no other code depends on their
specific values (width must stay odd).

Milestone 128 (terrain accuracy pass) shipped earlier in this same
session -- see that entry for the `data/overworld.grid` fixes. The World
Map screen mockup from that session (`docs/MILESTONES.md` NEXT UP item 8)
is still just a mockup, not started.
