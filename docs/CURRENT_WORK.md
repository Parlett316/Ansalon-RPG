# Current work

**Milestone 121 (the Wayreth quest) is implemented but not yet
interactively verified.** The user asked for "the Towers of High Sorcery"
to be added; research found the Palanthas Tower already shipped
(Milestone 44) and the Tower of Wayreth was the real gap, with a real
sourcing constraint (this project's own already-shipped Raistlin dialogue
says the Tower has no fixed location and "finds you rather than the
reverse"). Resolved per the user's own decision: unfindable except for a
Mage character, and even then reachable only through a new quest,
`wayreth_summons` (offered by a new "Robed Stranger" POI in Solace, gated
`REQUIRE wayreth_eligible` -- Mage, level 3+). Full design in
`docs/MILESTONES.md` entry 121, `docs/CHARACTER_NOTES.md`'s "The Wayreth
quest", and `docs/QUEST_NOTES.md`'s "Shipped quests".

Done so far: all code/data changes, a clean `/W4` rebuild (zero new
warnings), and the piped character-creation smoke test (confirms the new
`data/quests.txt` block and `data/zones/solace.txt` POI/binding parse
cleanly).

**Still needed**: interactive verification, same `_getch()` limitation as
every quest/combat milestone. Specifically: confirm the Robed Stranger
says nothing quest-related to a non-Mage or a Mage below level 3; confirm
the quest offers at level 3; confirm `VISIT palanthas` gates completion
correctly; confirm the Robe/outcome text matches the character's
alignment on turn-in and shows correctly on the character sheet
afterward. **No Mage save currently exists** -- `save1.txt` is Mason
(Half-Elf Thief) and `save2.txt` is Mike (Human Fighter), both level 1,
both with companions (Bren Alder + Dessa Corrin, and Dessa Corrin
respectively) -- so this needs either a fresh Mage character leveled to
3rd, or one of the two existing characters played further (neither is a
Mage, so a fresh character is the only path to testing this specific
milestone).

Prior milestones' own history lives in `docs/MILESTONES.md`, not here.
