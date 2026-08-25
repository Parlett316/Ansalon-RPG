# Current work

Nothing in flight.

Both of Milestone 78's (Frostreaver) and Milestone 62's (spellcasting UI)
open interactive-verification gaps closed out 2026-08-25 -- see their
entries in `docs/MILESTONES.md` for exactly what was confirmed and how.
One narrow gap remains from the Milestone 62 pass: an instant-defeat spell
(e.g. a Mage's Sleep) still hasn't been keyboard-verified -- see NEXT UP
item 1.

**Note on the live save**: `build/Debug/save.txt` is currently a
hand-edited maxed-stat (all six scores 18) Human Fighter, built
specifically to breeze safely through the Frostreaver verification trek.
The user's actual mid-game Fighter (EXP 967, day ~9 near Haven) was
accidentally overwritten by a throwaway Cleric test character earlier in
this same session and could not be recovered -- see memory for the
incident note. This maxed character is now the de facto live save going
forward unless the user starts over.

Next backlog candidates (not started, not committed) -- see
`docs/MILESTONES.md`'s NEXT UP: an instant-defeat spell's interactive
verification (item 1), real Draconian mechanics (item 2), SFML-backed
rendering revisit with real sprite art (item 3), widening `TALK_AFTER` to
three remaining zones (item 4), widening the color palette further (item
5), or zone-NPC `SUBJECT` content beyond Milestone 71's initial pass
(item 6).
