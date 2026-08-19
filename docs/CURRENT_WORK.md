# Current work

Nothing in flight. All completed milestones are documented in the
relevant `docs/*_NOTES.md` files and `README.md`'s Status paragraph — see
`CLAUDE.md` for which doc covers which system.

Most recent milestone: Milestone 40 (Rest and spell memorization). A new
`render::Key::Rest` (`'r'`/`'R'`, `GameLoop::handleRest`) rests once per
in-game day: advances `hoursElapsed` by 8 and heals 1 hp capped at
`maxHp` (2e DMG p.74's base natural-healing rate — not its faster 3 hp/
day "complete bed-rest" tier, left for a future Inn-gated variant). For a
Mage or Cleric, the same keypress also (re-)memorizes their one known
spell via new `character::memorizeSpells` (PHB p.107/p.111 sourced,
folding the book's sleep-then-study two-step into one action since
there's only one spell to memorize either way — nothing to select).
`character::hasSpellSlotAvailable` lost its old silent auto-refill and is
now a pure query: no slots are available until memorization has actually
happened that day. New save field `Character::lastRestDay` /
`RESTDAY` keyword (optional on load, old saves default to "never
rested," which is correct). Verified via a throwaway self-test (14
assertions, deleted after passing), a clean rebuild (zero new `/W4`
warnings), and the piped smoke test. See `docs/CHARACTER_NOTES.md`'s
"Spellcasting" section for full sourcing and scope cuts.

One note for the user directly: the real `save.txt` character (a
level-1 Fighter) is currently at 1/10 HP — pressing `r` in-game will
heal 1 point right away, same as any character.

Next step: nothing committed to yet. Backlog candidates (see
`docs/MILESTONES.md`'s "NEXT UP"): terrain-specific monster pools, more
monsters, a bed-rest healing tier (now that base Rest exists), or
continuing *Dragons of Spring Dawning* at Palanthas/Godshome/Neraka. Ask
the user which to start next, per the project's standing workflow.
