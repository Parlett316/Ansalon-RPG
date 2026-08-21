# Current work

Nothing in flight.

Milestone 62 (Cleric and Mage spells beyond 1st level) just shipped, at
the user's explicit request: `character::Spellcasting` grew from Milestone
40's one-known-spell-per-caster into a real multi-level spellbook (49
implemented spells across Mage's 9 levels and Cleric's 7), sourced from
`References/DQoK.pdf` (an official TSR/SSI Dragonlance computer game
manual, read via rendered page images since its 4-column layout badly
scrambles OCR) and cross-referenced against the PHB's own spell index and
Tables 21/24 -- 85 of 88 real spells found matched the PHB exactly, three
corrected (see `docs/CHARACTER_NOTES.md`'s "Spellcasting" section for the
full census and every citation). `Character` gained `memorizedSpellIds`/
`preferredSpellIds`; Rest/Bed Rest now default to re-memorizing the
standing loadout, only re-prompting a picker on request; combat's `m`/`M`
casts directly or opens a picker; `SaveGame.cpp`'s `SPELLSTODAY` line is
replaced by `SPELLDAY`/`PREFERRED`/`MEMORIZED` (legacy `SPELLSTODAY` still
loads read-only). See `docs/MILESTONES.md`'s Milestone 62 entry for the
full writeup, including the four reused effect-resolution patterns
(damage/heal/block/this-fight buff-debuff) and what's deliberately not
modeled (the other 39 sourced spells, DQoK's own Red/White Robe
restriction, monster saving throws).

Verified via a throwaway self-test (spell-slot tables, the Kender/blocked-
subrace zero-slots rule, `castSpell`'s effect shapes, `SaveGame` round-
tripping and legacy-`SPELLSTODAY` backward compatibility), a clean `/W4`
rebuild (zero new warnings), the piped character-creation smoke test, and
a direct load of the user's real save (a Fighter, so unaffected by
spellcasting itself, but its legacy `SPELLSTODAY` line needed to keep
loading). **Not yet interactively verified** -- the Rest re-memorize
prompt, the multi-level spell-loadout picker, and the in-combat cast
picker were all built and self-tested but never driven by a real
keypress (`_getch()` can't be piped). `docs/MILESTONES.md`'s NEXT UP
offers this as its own follow-up candidate alongside the pre-existing
more-monsters / more-DLA-magic-items options. Ask the user before
starting any of them.
