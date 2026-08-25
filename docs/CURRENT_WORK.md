# Current work

Nothing in flight.

Milestone 73 (colored dialogue/picker/combat screens, and re-picking after
a talk) just shipped -- see `docs/MILESTONES.md`'s Milestone 73 entry and
`docs/ARCHITECTURE.md`'s "Colored dialogue/picker/combat screens, and
re-picking after a talk" section for the full mechanism and rationale. In
brief: the "Bob's game" ANSI palette (already used by the overworld/zone
status panel and character creation) now also colors the speaker name in
`drawDialogueFrame`, the selected row in `drawPickerFrame`, and the player/
monster stat lines in `drawCombatFrame`, via a new `BoxLine` type and a
parallel colored `writeBoxed` overload that leaves every other "organic"
screen (character sheet, shop, inventory, journal, help, ask-input)
untouched. Separately, `GameLoop::pickAndTalk` no longer drops back to the
explore screen after one conversation when multiple NPCs are present --
finishing a talk now returns to the "Talk to whom?" picker instead.
Verified via a throwaway self-test (deleted after use, output inspected
with `cat -v` for correct escape-code pairing and column alignment), a
clean `/W4` rebuild (zero new warnings), and the piped smoke test.
**Interactive verification still needs the user's own keyboard** -- seeing
the actual colors render in a real terminal, and confirming the picker
loop-back feels right when talking to two-plus present NPCs (e.g. all 8
Heroes sharing a schedule stop) in a row.
