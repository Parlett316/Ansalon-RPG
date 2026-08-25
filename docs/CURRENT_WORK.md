# Current work

Nothing in flight.

Milestone 81 (widening the "Bob's game" color palette to the remaining
plain organic screens -- character sheet, spellbook, shop, inventory,
journal, help; `drawAskInputFrame` deliberately left plain) shipped
2026-08-25. Pure `MapRenderer.cpp` presentation change, zero new
`.h`/other-`.cpp` files, verified via a throwaway self-test (`cat -v`-
inspected escape codes and alignment), a clean `/W4` rebuild, and the
piped smoke test (real save moved aside and restored byte-identical
afterward) -- see `docs/MILESTONES.md` and `docs/ARCHITECTURE.md`'s
"Widening the palette to the remaining organic screens" for what shipped
and why. Real in-terminal color rendering still needs the user's own
eyes, same limitation every prior color/UI milestone has flagged.

**Note on the live save**: `build/Debug/save.txt` is still the
hand-edited maxed-stat (all six scores 18) Human Fighter built for the
Frostreaver verification trek (see Milestone 78). The user's actual
mid-game Fighter (EXP 967, day ~9 near Haven) was accidentally overwritten
by a throwaway Cleric test character in an earlier session and could not
be recovered. This maxed character remains the de facto live save unless
the user starts over.

Next backlog candidates (not started, not committed) -- see
`docs/MILESTONES.md`'s NEXT UP: an instant-defeat spell's interactive
verification (item 1), real Draconian mechanics (item 2), or an
SFML-backed rendering revisit with real sprite art (item 3).
