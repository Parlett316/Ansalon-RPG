# Character notes

## Accuracy: read this first

The numbers in `character/Race.cpp` and `character/CharClass.cpp` (racial
ability adjustments, saving-throw bonuses, class saving-throw tables,
starting gold formulas) are reproduced **from memory** of the 2nd Edition
AD&D Player's Handbook, not copied from the book. They are believed
reasonable but **not guaranteed accurate** — the project owner has said
they know this ruleset and should treat every number here as a first draft
to spot-check, not gospel. That's deliberate: each table is one small,
clearly-commented block specifically so a wrong number is a one-line fix,
not a hunt through the codebase. If you correct one, it's worth a quick
note in this file for future reference.

## Ability score generation

**3d6, straight down the line** (STR, DEX, CON, INT, WIS, CHA, fixed
order — 2e's "Method I" dice), with a house-rule: the player may **reroll
the whole set of six as many times as they like** before accepting. This is
not Method II (4d6, drop lowest, arrange to taste) — the project owner
specifically chose Method I's dice with free rerolling instead. See
`character::CharacterCreator::run`.

## Scope: what this milestone does and doesn't model

No combat or leveling system exists yet, which is what makes several 2e
mechanics safe to skip for now rather than build unused:

- **No percentile Strength.** A Fighter/Paladin/Ranger with 18 Strength in
  real 2e rolls percentile (18/01–18/00) for a finer-grained bonus; here
  it's just 18. Worth adding once STR-derived to-hit/damage bonuses matter
  (i.e., once combat exists).
- **THAC0 is a flat 20 for every class at level 1.** This is actually
  correct 2e behavior at level 1 — THAC0 only diverges by class as levels
  are gained (Fighters improve fastest, Mages slowest). Once leveling
  exists, THAC0 needs a per-class, per-level progression table.
- **No demihuman level limits.** 2e caps how high non-human races can
  advance in certain classes (e.g. a Dwarf Fighter is capped well below a
  Human's unlimited progression). Irrelevant until leveling exists.
- **No alignment restrictions.** The player picks freely from all 9
  alignments regardless of race/class. (The core four classes here have
  minimal restrictions in 2e anyway — this matters more once Paladin,
  Ranger, or Druid are added.)
- **No spellbook or spell selection.** A Mage or Cleric character just
  knows their class; actual starting spells are future work alongside a
  magic/casting system.
- **No equipment or armor system.** Starting gold is tracked as a bare
  number (`Character::goldPieces`); Armor Class is always unarmored
  (`10 - Dex adjustment`, see `character::acAdjustmentForDexterity`) until
  there's an inventory system to equip armor from.
- **Only two derived ability modifiers are computed**: Constitution → HP
  adjustment, Dexterity → AC adjustment (`character/Ability.h`). The other
  four abilities have their own 2e tables (STR → to-hit/damage, INT → max
  spell level/languages, WIS → spell bonuses, CHA → henchmen/reactions) —
  deferred until something in the game actually reads them.

## Race and class scope

Seven core PHB races (Human, Dwarf, Elf, Gnome, Half-Elf, Halfling,
Half-Orc) and the four foundational classes (Fighter, Mage, Cleric, Thief).
Paladin, Ranger, Druid, Bard, and specialist wizards are not implemented —
adding one means extending `character::ClassId`/`kAllClasses` and the table
in `CharClass.cpp` with its prime requisite, hit die, level-1 saves, and
gold formula, plus (for Paladin/Ranger/Druid) deciding how to handle their
alignment restrictions, which nothing currently enforces.

Racial saving-throw bonuses are simplified to a flat +1 in one or two
categories per race. Real 2e dwarves and halflings have bonuses that scale
with Constitution rather than a flat number — simplified here on purpose to
avoid a second ability-dependent lookup table for a first pass.

## Where a character lives

`CharacterCreator::run()` executes once, in `main.cpp`, before `GameState`
or `GameLoop` exist — it's plain `std::cin`/`std::cout` prompts, not
`render::Console::readKey()` (see `docs/ARCHITECTURE.md` on why creation is
a different interaction mode from the real-time game loop). The result is
stored in `GameState::character` and never reassigned after that; pressing
`c` during play (`render::Key::Sheet`) just displays it via
`MapRenderer::drawCharacterSheet`, it doesn't let you re-run creation.

## Extending this later

- **Leveling/XP**: would need a per-class, per-level table for THAC0 and
  saving throws (currently only level 1 is modeled), plus demihuman level
  limits if those are to be enforced.
- **Combat**: would start reading `Character::thac0`/`armorClass` and the
  currently-unused STR-derived to-hit/damage tables.
- **Spellcasting**: Mage/Cleric would need spell list data and a "known/
  memorized spells" concept added to `Character`.
- **Equipment/inventory**: would replace the flat unarmored AC calculation
  with armor-modified AC, and give `goldPieces` somewhere to actually be
  spent.
