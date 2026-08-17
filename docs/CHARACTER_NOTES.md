# Character notes

## Accuracy: verified against scanned rulebooks

The numbers in `character/Race.cpp`, `character/Ability.cpp`, and
`character/CharClass.cpp` were originally written from memory, then
**checked directly against scanned copies of the source books** the
project owner provided (`Player's Handbook (revised).pdf`,
`Dungeon Master's Guide (2nd edition).pdf`, and the Dragonlance-specific
`TSR 2021 DragonLance Adventures.pdf` / `TSR 2143 PG1 Players Guide to the
Dragonlance Campaign.pdf`, kept locally, not committed — see
`.gitignore`). Specific pages checked, and what that check found:

- **Table 2 (Dexterity, PHB p.20) and Table 3 (Constitution, PHB p.21)**:
  the AC/HP adjustment tables in `character/Ability.cpp` had real errors at
  the low end (Constitution 3–8, Dexterity 3–6) and were missing the
  warrior-only Constitution 17–18 hit point bonus entirely. Fixed and now
  transcribed exactly.
- **Table 8 (Racial Ability Adjustments, PHB p.27)**: Dwarf/Elf/Gnome/
  Halfling adjustments already in `Race.cpp` were exactly correct. Human
  and Half-Elf getting no adjustment is also correct (they don't appear in
  the table).
- **Table 60 (Character Saving Throws, PHB p.134)**: all four classes'
  level-1 saving throw numbers in `CharClass.cpp` were exactly correct
  already.
- **Fighter/Mage/Cleric/Thief prime requisites and hit dice** (PHB pp.36,
  44, 48, 54): all exactly correct already.
- **Table 43 (Initial Character Funds, PHB p.89)**: starting gold was
  **wrong** for Mage, Cleric, and Thief (wrong dice, and Mage uses a
  `(1d4+1)×10` formula, not a flat `NdM×10`) — fixed.
- **Table 9 (Constitution Saving Throw Bonuses, PHB p.28) and the Dwarf/
  Gnome/Halfling race descriptions (PHB pp.28–31)**: the actual mechanic
  is a Constitution-scaled bonus (+1 per ~3.5 Con points) applied to
  specific save categories, not the flat +1 this project originally used —
  fixed, see below. Also found: **Elf's real ability is 90% magic
  resistance specifically against sleep/charm spells**, not a saving-throw
  bonus at all — the flat "+1 Spell save" this project originally gave
  Elves was fabricated and has been removed.
- **Half-Orc turned out not to be a standard PC race in the revised 2e
  PHB at all** — it doesn't appear in Table 7 or Table 8. Combined with
  Half-Orcs having essentially no presence in Dragonlance/Ansalon lore,
  it's been replaced with **Kender** (verified against Dragonlance
  Adventures p.53 — see below).

Any *remaining* unverified numbers (things not read directly from a book
page — mainly flavor text and minor rounding choices) are called out
inline where they occur. Race/class data still lives in small,
clearly-commented tables specifically so a future correction stays a
one-line fix.

## Ability score generation

**3d6, straight down the line** (STR, DEX, CON, INT, WIS, CHA, fixed
order — 2e's "Method I" dice, confirmed verbatim on PHB p.19), with a
house-rule: the player may **reroll the whole set of six as many times as
they like** before accepting. Not Method II (4d6, drop lowest, arrange to
taste) — the project owner specifically chose Method I's dice with free
rerolling instead. See `character::CharacterCreator::run`.

## Racial magic resistance (Dwarf, Gnome, Halfling, Kender)

Per PHB p.28 (Dwarf) and p.29 (Halfling), and Dragonlance Adventures p.53
(Kender, "all standard halfling abilities"): these races gain a bonus
against magic that scales with Constitution via Table 9
(`character::constitutionMagicResistanceBonus`), not a flat number. The
bonus applies to Rod/Staff/Wand and Spell saves for all four races, and
*additionally* to the Paralyzation/Poison/Death category (specifically
representing poison resistance) for Dwarf, Halfling, and Kender — but
**not** Gnome, whose PHB entry (p.30) only mentions the wand/staff/rod/
spell bonus. See `character::applyRacialSavingThrowBonus`.

Elf and Half-Elf have a *different* ability — flat percentage magic
resistance (90% and 30% respectively) specifically against sleep and
charm-related spells (PHB pp.29–31) — which doesn't correspond to any of
our five save categories and isn't mechanically represented yet (there's
no spell-casting system to resist). Not a bug: this is a genuine gap
between the source material's granularity and what the game currently
models, left for whenever a magic system exists to make it meaningful.

## Kender in place of Half-Orc

Ability adjustments (STR −1, DEX +2) and the "cannot learn Mage/illusionist
spells" restriction are from Dragonlance Adventures (TSR 2021), p.53 —
kender have innate magic resistance that specifically blocks arcane
spellcasting. This restriction is *flagged*, not hard-blocked, in character
creation — consistent with how prime-requisite mismatches are handled
elsewhere (see `CharacterCreator::run`): a player who insists on a kender
Mage can still make one; the game just tells them why that's unusual.
Kender racial ability *ranges* (min/max caps) and class level limits from
the same source are not enforced, consistent with every other race here
not having min/max enforcement either.

## Scope: what this milestone does and doesn't model

No combat or leveling system exists yet, which is what makes several 2e
mechanics safe to skip for now rather than build unused:

- **No percentile Strength.** A Fighter with 18 Strength in real 2e rolls
  percentile (18/01–18/00) for a finer-grained bonus; here it's just 18.
  Worth adding once STR-derived to-hit/damage bonuses matter (i.e., once
  combat exists).
- **THAC0 is a flat 20 for every class at level 1.** This is correct 2e
  behavior at level 1 — THAC0 only diverges by class as levels are gained.
  Once leveling exists, THAC0 needs a per-class, per-level progression
  table.
- **No demihuman level limits.** Irrelevant until leveling exists.
- **No alignment restrictions.** The player picks freely from all 9
  alignments regardless of race/class.
- **No spellbook or spell selection.** A Mage or Cleric character just
  knows their class; actual starting spells are future work alongside a
  magic/casting system. (Kender specifically are flagged as unable to cast
  arcane magic at all — see above — but nothing stops them from being
  *chosen* as Mage today, since spellcasting doesn't exist yet either way.)
- **No equipment or armor system.** Starting gold is tracked as a bare
  number (`Character::goldPieces`); Armor Class is always unarmored
  (`10 - Dex adjustment`) until there's an inventory system to equip armor
  from. The Priest-specific rule that clerics must return excess starting
  gold to their order (PHB p.89) isn't enforced for the same reason.
- **Only two derived ability modifiers are computed**: Constitution → HP
  adjustment, Dexterity → AC adjustment. STR (to-hit/damage), INT/WIS
  (spell bonuses/max spell level), and CHA (reactions/henchmen) all have
  their own 2e tables too, deferred until something reads them.

## Race and class scope

Six core 2e PHB races (Human, Dwarf, Elf, Gnome, Half-Elf, Halfling) plus
Kender (see above, replacing Half-Orc), and the four foundational classes
(Fighter, Mage, Cleric, Thief — confirmed on PHB p.35 as "the standard
classes... appropriate to any sort of AD&D game campaign"). Paladin,
Ranger, Druid, Bard, and specialist wizards are not implemented — adding
one means extending `character::ClassId`/`kAllClasses` and the table in
`CharClass.cpp` with its prime requisite, hit die, level-1 saves, and gold
formula, plus (for Paladin/Ranger/Druid) deciding how to handle their
alignment restrictions, which nothing currently enforces.

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
  memorized spells" concept added to `Character`. This is also where
  Kender's "cannot cast arcane magic" restriction and Elf/Half-Elf's sleep/
  charm magic resistance would finally need real enforcement.
- **Equipment/inventory**: would replace the flat unarmored AC calculation
  with armor-modified AC, and give `goldPieces` somewhere to actually be
  spent.
- **Dragonlance-specific depth**: `TSR 2021 DragonLance Adventures.pdf` and
  the DL Player's Guide contain much more setting-specific material worth
  mining later — subrace variants (Silvanesti/Qualinesti/Kagonesti elves,
  Hill/Mountain/Gully dwarves, Tinker Gnomes), and the Knights of Solamnia/
  Wizards of High Sorcery kit systems that would let Fighter/Mage feel more
  distinctly Dragonlance. Not pulled in now — deliberately out of scope for
  this pass, which focused on getting the *core* four-class ruleset
  correct first.
