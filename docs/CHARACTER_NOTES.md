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
- **Elf/Dwarf subraces (DL Adventures pp.60–69), Tinker Gnome (p.57), and
  Knights of Solamnia entry requirements (pp.14–20)**: same
  render-the-page-and-read-it discipline, done by a research agent rather
  than inline in this session. Found real gaps versus what this project
  had: Krynn's dwarves/Kagonesti elves can't be Mages at all (not
  previously modeled), Tinker Gnome's real adjustment (STR−1/DEX+2) differs
  from the generic PHB Gnome stats this project used before, and Wizards of
  High Sorcery turned out to have essentially nothing to model at 1st level
  (Robes aren't assigned until 3rd). See "Dragonlance depth" below for the
  full writeup.

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

No leveling system exists yet (combat now does — see
`docs/COMBAT_NOTES.md`), which is what makes several 2e mechanics safe to
skip for now rather than build unused:

- **Percentile Strength is implemented** (`Character::exceptionalStrengthPercentile`,
  rolled by `CharacterCreator` for a Fighter at STR 18, per PHB Table 1's
  warrior-only rule) — no longer deferred, now that combat reads
  STR-derived to-hit/damage bonuses. See `docs/COMBAT_NOTES.md`.
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
- **A real but deliberately small equipment system exists** (see
  "Equipment" below) — a General Store sells a handful of sourced armor
  tiers and one weapon upgrade per class, buyable with `steelPieces`, and
  a real carried inventory + equip/unequip screen (Milestone 21). Still no
  other item types, no selling gear back, no armor weight/encumbrance. The
  Priest-specific rule that clerics must return excess starting funds to
  their order (PHB p.89) still isn't enforced.
- **Three derived ability modifiers are computed**: Constitution → HP
  adjustment, Dexterity → AC adjustment, and (as of combat) Strength →
  to-hit/damage adjustment (`docs/COMBAT_NOTES.md`). INT/WIS (spell
  bonuses/max spell level) and CHA (reactions/henchmen) still have their
  own 2e tables too, deferred until something reads them (a magic system,
  for INT/WIS; nothing currently models NPC reactions or henchmen at all).

## Race and class scope

Six core 2e PHB races (Human, Dwarf, Elf, Gnome, Half-Elf, Halfling) plus
Kender (see above, replacing Half-Orc), and the four foundational classes
(Fighter, Mage, Cleric, Thief — confirmed on PHB p.35 as "the standard
classes... appropriate to any sort of AD&D game campaign"), plus a fifth,
Dragonlance-only class, Tinker, forced automatically onto every Gnome PC
(see "Tinker Gnome" below) rather than offered as a menu choice. Paladin,
Ranger, Druid, Bard, and specialist wizards are not implemented — adding
one means extending `character::ClassId`/`kAllClasses` and the table in
`CharClass.cpp` with its prime requisite, hit die, level-1 saves, and
starting-steel formula, plus (for Paladin/Ranger/Druid) deciding how to handle their
alignment restrictions, which nothing currently enforces.

## Dragonlance depth: subraces, Knights of Solamnia, Wizards of High Sorcery

Everything in this section was verified against scanned pages of
`TSR 2021 DragonLance Adventures.pdf`, the same visual-confirmation
discipline as the rest of this document — a research agent rendered each
relevant page as an image and read it directly rather than trusting the
scan's garbled OCR text layer. Page numbers below are PDF page numbers
(the book's own printed page number is consistently one less, e.g. PDF
p.60 = printed p.59).

### Elf and Dwarf subraces are mandatory, not optional

There's no generic "Elf" or "Dwarf" PC on Krynn — choosing either race in
`CharacterCreator` immediately prompts a subrace choice, which **replaces**
(doesn't add to) the base race's ability adjustment:

- **Silvanesti Elf** (p.60): CON−1/DEX+1 — identical to the base PHB Elf
  adjustment, just restated in the subrace's own table. Can be a Mage.
- **Qualinesti Elf** (p.61): same CON−1/DEX+1. Can be a Mage.
- **Kagonesti Elf** (p.62): STR+1/CON+1/DEX+2/INT−3 — four simultaneous
  adjustments, more than `RaceInfo::adjustments`' 3-slot array can hold,
  which is why `character::SubraceInfo` uses its own 6-int array indexed
  directly by `Ability` instead. **Cannot be a Mage at all.**
- **Hill Dwarf** (p.67): CHA−1/CON+1 — same as the base PHB Dwarf
  adjustment. **Cannot be a Mage at all** (a Dragonlance-specific
  restriction the base Dwarf entry doesn't have).
- **Mountain Dwarf** (p.68): same CHA−1/CON+1. Also cannot be a Mage.

`character::effectiveCanBeMage(race, subrace)` returns the subrace's
`canBeMage` when one is selected, else the base race's — this is how
Kagonesti/Hill/Mountain block Mage while Silvanesti/Qualinesti don't, using
the same non-blocking "flagged, not hard-blocked" UX as Kender's Mage
restriction (see above).

**Not modeled, deliberately**: each subrace's book page also lists ability
score *ranges* (min/max caps) and class *level limit* tables (e.g. a
Silvanesti Fighter capped at level 10) — left unenforced, extending the
exact same precedent already documented for Kender ("racial ability ranges
and class level limits... not enforced, consistent with every other race
here"). Level limits specifically are moot anyway until a leveling system
exists (see "No demihuman level limits" above).

**Gully Dwarf is deliberately not included** as a third Dwarf subrace
option, even though it's in the source book (p.69) and was on this
project's own "extend later" list. The book generates Gully Dwarf ability
scores with an entirely different method (e.g. Strength 4d4+2, Intelligence
2d4+1) instead of 3d6 — incompatible with this project's settled
3d6-down-the-line-with-reroll house rule. Approximating it with a flat
ability adjustment (the way every other subrace works) would misrepresent
a genuinely different generation method rather than honestly model it, so
it's left as clearly-flagged future work instead of a fudged approximation.

### Tinker Gnome replaces the generic PHB Gnome entirely, and Tinker is a real class

`character::raceInfo(RaceId::Gnome)`'s adjustment changed from the old
generic-PHB INT+1/WIS−1 to **Tinker Gnome's STR−1/DEX+2** (p.57) — Krynn
gnomes *are* Tinker Gnomes, there's no other kind of PC gnome in this
setting, same precedent as Kender fully replacing Half-Orc rather than
being an optional variant alongside it.

The book states *"Gnomes in Krynn can only be of the tinker class"* (p.57).
**This is hard-enforced, by explicit user decision** (superseding an
earlier, softer pass of this same feature that just printed a caveat and
left Gnomes free to pick any of the four core classes — the user asked
directly to force it instead). `character::ClassId` gained a fifth value,
`Tinker`; `CharacterCreator` auto-assigns it to every Gnome PC with no
class-choice prompt at all (there's only one legal answer, so there's
nothing to choose), and it never appears as an option for any other race —
`kAllClasses` (the menu list) deliberately still only lists the core four.

**Tinker's stats** (Dragonlance Adventures pp.22–23/printed pp.21–22,
"Gnome Advancement Table," visually confirmed): prime requisite
**Intelligence 10+**, hit die **d4**. The book also states Dexterity 12+
and a maximum Wisdom of 12 for Tinkers — **not enforced**, consistent with
this project's standing precedent of never enforcing ability score ranges
(see "Elf and Dwarf subraces" above).

**Not in the source book at all — a real gap, not an OCR failure**: a
dedicated research pass read the entire Tinker chapter and confirmed the
book has **no saving-throw table, no starting-steel formula, and no THAC0
value** for this class. Its own front matter states it deliberately avoids
duplicating PHB/DMG material and expects those to come from the core
books — but it never says which PHB class-group (Warrior/Wizard/Priest/
Rogue) a Tinker should borrow from. `CharClass.cpp`'s Tinker entry reuses
**Mage's** saving throws and starting-steel formula as a documented analogy (same d4
hit die, same Intelligence prime requisite, same fragile-caster
archetype) — clearly commented as an analogy, not a sourced number, so
it's a one-line fix if a better source ever turns up. THAC0 isn't an issue
either way: every class in this project is flat 20 at level 1 regardless
(true to 2e — see "Scope" above).

**Not modeled at all — deliberately out of scope, and a real subsystem**:
the book defines a genuinely detailed **device-construction minigame**
(pp.23–25/printed pp.22–24) — Complexity scores, part-cost tables, crew-size
modifiers, build-time charts, and a Success/Unpredictable/Failure roll
against a Gnomish Invention Results table (with its own Mishap sub-table).
This is Tinkers' whole reason for existing flavor-wise, but it's a sizable
standalone system (several cross-referenced tables, two of which — the
Complexity and Mishap tables themselves, printed pp.118–119 — weren't even
rendered/transcribed in the research pass that found this). Worth building
eventually as its own feature, not bolted onto character creation.

### Knights of Solamnia — Crown and Sword

Every Knight starts in the **Order of the Crown** (p.17: "every candidate
... must first enter the Knighthood as a squire of the Order of the
Crown"). Advancing into the **Order of the Sword** is real as of Milestone
53 — a `character::KnightOrder::Sword` value plus the `named_in_fact` quest
(`docs/QUEST_NOTES.md`), see below. A Sword Knight can also now earn
**Solamnic Armor** (`data/quests.txt`'s `solamnic_armor`) — see "Magic
items" above; it doesn't advance `knightOrder` further, it's equipment,
not a rank. The **Order of the Rose** still requires
XP thresholds and a witnessed quest (p.19–20) this project doesn't model —
deferred the same way Sword was until this milestone, not yet content this
project has scoped.

**Entry requirements for Knight of the Crown** (p.18, "Game Data",
visually confirmed on the rendered page): ability minimums **Strength 10,
Intelligence 7, Wisdom 10, Dexterity 8, Constitution 10** (no Charisma
minimum), and alignment must be **Good** (Lawful, Neutral, or Chaotic Good
— the book ties Knighthood to the good/evil axis specifically, not a
law/chaos one). Implemented in `character::meetsKnightOfCrownRequirements`
(`Knighthood.h/.cpp`), offered as a yes/no prompt in `CharacterCreator`
immediately after alignment is chosen, only to Fighters.

**Entry requirements for Knight of the Sword** (p.18, "Knights of the
Sword / Game Data", visually confirmed on the rendered page — see
"Sourcing note: a book erratum" below): the candidate must be a Knight of
the Crown who has "risen to 2d level ... and [has] sufficient experience
points to gain 3d level," and must meet ability minimums **Strength 12,
Intelligence 9, Wisdom 13, Dexterity 9, Constitution 10** (again no
Charisma minimum) — every minimum equal to or higher than Crown's, so not
every Crown Knight automatically qualifies. Implemented in
`character::meetsKnightOfSwordRequirements` (scores only — `knightOrder ==
Crown` already implies the Crown race gate was passed) and
`game::conditionMatches`'s `sword_eligible` token (`knightOrder==Crown &&
level>=3 && meetsKnightOfSwordRequirements(scores)`). `level>=3` stands in
for "2nd-level Crown with enough XP banked for 3rd": this project runs
Knights on the Fighter XP chassis rather than the book's separate Crown
Knight XP table (see "Flavor-only, not mechanical" below), and level 3 is
exactly where the existing Crown/Sword flavor line already fires. The
book's further "witnessed quest" requirement (a journey of ≥500 miles/30
days, three tests of wisdom, one of generosity, one of compassion, the
restoration of something lost, and single combat with an evil opponent)
is `data/quests.txt`'s `named_in_fact` quest — see `docs/QUEST_NOTES.md`
for how each element maps (or doesn't) onto real objective state.

**Sourcing note: a book erratum.** DL Adventures p.18 prints a "Game Data"
minimums box for the Sword directly under the sentence "the candidate must
also have the listed minimums ... to qualify for the Order of the Sword"
— but that box's own header reads "**Rose** Knight Minimum Scores," with
values (Str 12/Int 9/Wis 13/Dex 9/Con 10) that don't match the *other*,
correctly-headed "Rose Knight Minimum Scores" box printed on p.19 (Str
15/Int 10/Wis 13/Dex 12/Con 15). Confirmed via rendered page images
(`pdftoppm`, not just `pdftotext` OCR, which could otherwise read as a
column-alignment artifact rather than the book's own real mislabel) that
this is a genuine 1987 printing error, not an OCR misread. The p.18 box's
values are used above as the real Sword minimums; p.19's values remain the
real Rose minimums, for whenever Rose is implemented.

**Deliberate simplification**: the book actually builds Knights of
Solamnia on the **Cavalier** class (Unearthed Arcana), not Fighter — this
project doesn't implement Cavalier (out of scope, same as Paladin/Ranger/
Druid/Bard above) and isn't adding it just for this. Modeled instead as "a
qualifying Fighter who swears the oath," which loses the Cavalier-specific
perks the book mentions (e.g. guaranteed weapon specialization) — moot for
now anyway, since this project has no weapon-proficiency system for that to
plug into.

**Racial exclusion**: every Elf and Dwarf subrace researched shows "N/E"
(not eligible) for Knight of Crown/Sword/Rose in its class-limit table — no
exceptions found in any of the five tables checked. Modeled as a hard
exclusion by parent race (`race == RaceId::Elf || race == RaceId::Dwarf`)
in `meetsKnightOfCrownRequirements`. **Other races' Knighthood eligibility
was not found in the researched pages** (the class-limit tables checked
were specifically the Elf/Dwarf subrace ones) — they default to allowed
rather than guessed-excluded, since Solamnic Knights are portrayed as
predominantly-but-not-exclusively human in the wider setting. Revisit if a
source page covering Human/Half-Elf/Halfling/Kender/Gnome Knighthood
eligibility turns up.

**Not modeled**: no saving-throw bonus tied to Crown rank was found
anywhere in the chapter (any such bonus would come from the base Cavalier
chassis in Unearthed Arcana, which isn't implemented); falling from Good
alignment is described as demoting a Knight back to a plain Fighter (p.14)
but nothing currently tracks alignment changes after character creation to
enforce this.

### Wizards of High Sorcery — mostly out of scope at level 1, and that's correct

This is the single biggest scope-reducer the research turned up, and it's
worth stating plainly: **a starting (1st-level) Krynn wizard has no Robe
and belongs to no Order at all.** Per p.28 and p.35, alignment declaration
and Robe assignment (White/Red/Black, tied to Solinari/Lunitari/Nuitari)
don't happen until the **Test of High Sorcery at 3rd level** — a level this
project's level-1-only character creation never reaches. Inventing a
robe-choice prompt now would mean fabricating a mechanic the source
material explicitly says doesn't apply yet, which this project avoids on
principle (see the accuracy note at the top of this document).

What *does* apply at 1st level, confirmed on the "Student Wizard Minimum
Scores" table (p.35): an added **Dexterity 6+** requirement for Mage, on
top of the existing Intelligence 9+. Checked in `CharacterCreator` (a
non-blocking flag, same UX as every other class-eligibility note) and
paired with flavor text on the final summary and character sheet — "an
unaffiliated student of the arcane, a Robe and Order await at higher
levels" — rather than any mechanical robe system.

**Now that leveling exists** (see below): Robe assignment by alignment at
the Test of High Sorcery is implemented as a level-3 flavor moment.
Spellcasting itself now exists too (see "Spellcasting" below), but
Robe-based spell-sphere restrictions and moon-phase
(Solinari/Lunitari/Nuitari) saving-throw/spellcasting bonuses are still
deferred — moot with only one, non-sphere-restricted Mage spell in the
game (the book itself states a wizard "is unaffected by phases of the
moons" below 3rd level anyway, so this was never a level-1 concern
either).

## Leveling / experience

`character::applyPendingLevelUps` (`Leveling.h/.cpp`) is called from
`GameLoop::runCombat` after a monster's XP award, and covers everything
sourced from the PHB, visually confirmed against rendered page images:

- **XP tables per class** (Fighter: Table 14 p.36; Mage: Table 20 p.42;
  Cleric: Table 23 p.47; Thief: Table 25 p.53), **levels 1–20 — the full
  printed table for each class**. (Originally implemented through level
  10 only; extended to the full 20 in a follow-up pass once the user
  asked why it stopped short.) Level 20 costs 2.2–3.75 million XP
  depending on class, against monster kills worth 7–420 XP
  (`docs/COMBAT_NOTES.md`) — a very long way off, but no longer an
  artificial cap. THAC0 (Table 53) and saves (Table 60) were already
  sourced through 20/21+ from the start, so nothing else needed
  extending. Tinker reuses
  the Mage/Wizard-group table (see "Tinker Gnome" above for the existing
  saves/steel analogy this extends — the real *Dragonlance Adventures*
  Advancement Table, p.22, was only partially glimpsed in an earlier
  research pass and not re-confirmed, so it wasn't used).
- **HP per level**: each class rolls its hit die + CON adjustment through
  a cutoff level (9th for Fighter/Cleric, 10th for Mage/Thief), then a
  flat amount with **no** CON bonus after that (Fighter +3, Cleric +2,
  Mage +1, Thief +2) — each class's chapter states this near-verbatim.
  Level-ups heal (`currentHp` rises by the same amount as `maxHp`) — a
  standard convention, not a specifically-cited rule.
- **THAC0 by level** (Table 53, p.121) and **saving throws by level**
  (Table 60, p.134, bracketed by level range) are both transcribed as
  small lookup tables in `Leveling.cpp`, per class group (Warrior/Wizard/
  Priest/Rogue — Fighter/Mage/Cleric/Thief respectively; Tinker again
  maps to Wizard). Racial saving-throw bonuses
  (`applyRacialSavingThrowBonus`) are reapplied after every level's table
  lookup, same as at character creation.

**Not modeled**: Fighter's extra attacks per round past 6th level (PHB
Table 15) — the combat round loop resolves exactly one attack per side;
adding a second would mean restructuring it, deferred since level 7 is
far off given the XP costs above. Demihuman level limits — same standing
deferral as every other race/subrace ability-range cut in this document.

**Flavor, then (as of Milestone 53) real**: a Knight of the Crown reaching
level 3 still gets a foreshadowing line about the Order of the Sword
noticing them (`Leveling.cpp`) — that line no longer stands alone, though:
level 3 is also the exact moment `sword_eligible` can start being true (see
"Knights of Solamnia" above), so the Muster Yard's Sword Knight at High
Clerist's Tower has something to actually offer once the player next talks
to him. A Mage reaching level 3 gets the mechanical Test of High Sorcery
outcome the same way (robe assigned by alignment, per the "Wizards of High
Sorcery" section above) — sourced and applied directly, no quest gate
needed since the alignment-to-robe mapping doesn't depend on a narrative
test.

## Spellcasting

`character::Spellcasting` (`Spellcasting.h/.cpp`) gives Mage and Cleric one
real, PHB-sourced spell each -- not a spellbook/spell-selection system,
same "one sourced thing done honestly, not a whole subsystem faked" spirit
as `ClassInfo`'s single placeholder weapon (see `docs/COMBAT_NOTES.md`).
Everything below was visually confirmed against rendered PHB pages, same
discipline as every other rules pass in this project:

- **Mage knows Magic Missile** (PHB p.176, Evocation): 1d4+1 damage per
  missile, automatic hit (no attack roll, no saving throw -- the book's
  own wording: "unerringly strike their target"), missile count = 1 at
  1st level plus one more every two levels, capped at 5 (reached at 9th
  level).
- **Cleric knows Cure Light Wounds** (PHB p.253, Necromancy, Reversible):
  a flat 1d8 healed, no level scaling, capped at the character's `maxHp`.
- **Spell slots are real and level-based**: Wizard 1st-level-spell-per-day
  counts come from Table 21 (Wizard Spell Progression, p.43); Cleric's
  come from Table 24 (Priest Spell Progression, p.47) *plus* Wisdom bonus
  spells (Table 5, p.23, cumulative per the book's own worked example).
  Both tables' 1st-level column is transcribed in full for levels 1-20,
  matching `Leveling`'s existing level range. **Mages get no
  Intelligence-based bonus slots** -- confirmed against the book: INT
  governs chance-to-learn-a-spell and max spell level for Wizards, not
  slot count, unlike Wisdom's role for Clerics. This asymmetry is
  deliberate, not a missing feature on the Mage side.
- **Rest and spell memorization are real now (Milestone 40).** Pressing
  `r` (`render::Key::Rest`, `GameLoop::handleRest`) rests once per
  in-game day (`Character::lastRestDay`, same `hoursElapsed / 24` day
  convention as everything else): it advances `hoursElapsed` by 8 (an
  overnight rest) and heals 1 hp, capped at `maxHp` -- the DMG's base
  natural-healing rate (2nd ed. DMG p.74, "Healing": "Characters heal
  naturally at a rate of 1 hit point per day of rest. Rest is defined as
  low activity -- nothing more strenuous than riding a horse or
  traveling from one place to another"). Also not modeled: the DMG's
  food/water/sleep prerequisite for healing at all -- this project has no
  hunger/supply system, so Rest always assumes those are met.

  **Complete bed rest, at an Inn, is real now (Milestone 41).** A new
  `render::Key::BedRest` (`'z'`/`'Z'` -- not `'b'`, already `SouthWest` in
  the `yubn` diagonal-movement scheme; `GameLoop::handleBedRest`) works
  only while standing on a zone POI marked `BED <char>`
  (`world::PointOfInterest::isBed`, parsed by `ZoneLoader` exactly like
  `SHOP` -- no `TALK` prerequisite, see `docs/ZONE_NOTES.md`'s "Beds"
  section). It shares `Character::lastRestDay` with ordinary Rest (one
  overnight action per in-game day, whichever kind) and advances
  `hoursElapsed` by the same 8 hours, but heals fully to `maxHp` instead
  of 1 hp. This is a **deliberate simplification** of the DMG's literal
  "complete bed-rest" rule (2nd ed. DMG p.74: "If a character has
  complete bed-rest (doing nothing for an entire day), he can regain 3
  hit points for the day. For each complete week of bed-rest, the
  character can add any Constitution hit point bonus he may have to the
  base of 21 points (3 points per day) he regained during that week.").
  Taken literally, that's a multi-day-to-multi-week grind sitting in an
  Inn room -- a poor fit for this project's timeline-driven pace, where
  canon characters move on a real schedule (`docs/TIMELINE_NOTES.md`)
  the player can walk past and miss. A single full-heal action was chosen
  instead; the 3 hp/day and weekly-Constitution-bonus tiers are not
  modeled, and this deviation from the sourced rule is intentional, not
  an oversight. `data/zones/solace_inn.txt`'s existing `U "The Stairs Up"`
  POI carries the new `BED U` line -- no other zone has an authored
  Inn/lodging POI, so no other zone got one.

  For a Mage or Cleric, the same keypress also (re-)memorizes their one
  known spell for the day, via `character::memorizeSpells`. Real 2e
  requires a restful night's sleep *and then* time spent studying
  (Wizard, PHB p.107, "Wizard Spells": "Memorization is not a thing that
  happens immediately. The wizard must have a clear head gained from a
  restful night's sleep and then has to spend time studying his spell
  books. The amount of study time needed is 10 minutes per level of the
  spell being memorized") or praying (Priest, PHB p.111, "Priest
  Spells": "Priests must pray to obtain spells... The conditions for
  praying are identical to those needed for the wizard's studying").
  Since every caster in this project knows exactly one 1st-level spell,
  that's a night's sleep plus 10 minutes -- i.e. exactly what one Rest
  keypress already represents, with no actual *selection* to expose
  through a second command (there's nothing to choose between). Folding
  the book's two-step requirement into one action is a deliberate
  simplification, not an oversight -- it would stop being honest the
  moment this project ever gains a real spellbook with more than one
  spell to pick from.

  `character::hasSpellSlotAvailable` is now a pure query with no side
  effects: it returns false unless `Character::spellsCastDay` equals the
  day being asked about, i.e. **no slots are available at all until
  memorization has happened that day**, regardless of class or level.
  The old behavior -- slots silently refilling the first time anything
  checked on a new day, with no player action involved -- is gone.
- **Real enforcement, finally, of the racial arcane-magic block.** Kender
  (`raceInfo(Kender).canBeMage == false`) and the three subraces that
  can't be Mages (Kagonesti Elf, Hill Dwarf, Mountain Dwarf, via
  `effectiveCanBeMage`) now actually get `maxSpellSlotsPerDay == 0` --
  before spellcasting existed, this restriction was flavor-only (see
  "Kender in place of Half-Orc" and "Dragonlance depth" above, which both
  said as much). Character creation is unchanged: you can still *choose*
  Mage as one of these, same non-blocking UX as every other
  eligibility mismatch in this project -- the consequence just shows up
  in combat now instead of nowhere.

**Wired into combat** (`GameLoop::runCombat`, see `docs/COMBAT_NOTES.md`):
a new `render::Key::Cast` (`'m'`/`'M'`) lets a Mage or Cleric spend their
round casting instead of attacking, through the same initiative-ordered
exchange as a normal attack.

**Not modeled, deliberately, and still real gaps**: no spells above 1st
level, ever, in this pass; Elf/Half-Elf's sleep/charm magic resistance
still has nothing to resist (neither Magic Missile nor Cure Light Wounds
triggers it); Wizard Robe spell-sphere restrictions are moot with only one
spell in the game, which isn't sphere-restricted to begin with. Tinker
still never casts despite borrowing Mage's saves/steel table as an analogy
-- gadgets, not magic.

## Equipment

`character::Equipment` (`Equipment.h/.cpp`) lets a character buy armor and
one weapon upgrade at a shop POI (`p` while standing on one) — same "solve
the reported problem, not the whole simulation" scope as Spellcasting's one
known spell per caster. Everything below was visually confirmed against
rendered PHB pages (this scan's OCR badly garbles table columns, so text
search alone wasn't trusted for exact numbers — see `docs/GOTCHAS.md`):

- **Armor tiers**: Leather (AC 8, 5stl), Chain Mail (AC 5, 75stl), Splint
  Mail (AC 4, 80stl) — Table 46 (Armor Class Ratings, p.99) for the AC
  values, Table 47 (Armor, p.92) for cost (the PHB's own gold-piece
  numbers, applied here as Steel Pieces, Krynn's real currency — see
  "Gold -> Steel" below). Splint Mail is offered instead of Banded/Bronze
  Plate Mail for the same AC 4 tier because it's the cheapest real item at
  that protection level (80stl vs 200stl/400stl) — Plate Mail and heavier
  are priced far beyond any level-1 character's starting steel and aren't
  offered yet.
- **Shield**: -1 AC, 7stl (Table 47's "Medium" shield entry). Table 46
  confirms a shield always improves AC by exactly 1 over the same armor
  without one, so this is modeled as a flat subtraction, not a separate
  lookup row.
- **"Wizards cannot wear any armor, for several reasons"** (PHB, Money
  and Equipment chapter) — a real, sourced restriction, not house rule.
  `character::canWearArmor` is false only for Mage and Tinker (the
  Mage-analogy class — same fragile-caster archetype already established
  for its saves/steel/hit die, see "Tinker Gnome" above). Mirrors the
  existing Kender/subrace arcane-magic-block precedent as a genuine
  class-differentiating rule, not an oversight.
- **One weapon upgrade per class that can use one** (Table 44, Weapons,
  p.94): Fighter's Long Sword (1d8) upgrades to Two-Handed Sword (1d10,
  50stl); Cleric's Mace (1d6) upgrades to Footman's Flail (1d6+1, 15stl);
  Thief's Short Sword (1d6) upgrades to Long Sword (1d8, 15stl). Mage and
  Tinker have no upgrade offered — their dagger/wrench stays as-is,
  consistent with wizards' traditionally short allowed-weapons list and
  their fragile-caster identity being intentional, not a gap.
- **`Character::armorClass` is always kept in sync**, not derived on the
  fly: `Equipment::recomputeArmorClass` runs at character creation
  (equivalent to the old `10 - Dex adjustment` formula, since
  `equippedArmor` starts as `ArmorId::None`) and again after every
  purchase. `combat::resolvePlayerAttack` reads
  `Character::weaponDamageSides`/`weaponDamageBonus` directly instead of
  the class's placeholder starting weapon — the one change that actually
  makes a purchased weapon upgrade matter in a fight.
- **The shop is a zone-grammar addition, not a new mechanism**: a `SHOP
  <char>` line in a zone file (see `docs/ZONE_NOTES.md`) marks an
  existing POI as browsable, same "must reference an already-declared
  POI" validation `TALK`/`PORTAL` already use. `GameLoop::handleShop()`
  takes over input in its own nested loop (same architectural shape as
  `runCombat`), reinterpreting `Key::North`/`South`/`Enter`/`Inventory`/
  `Quit` locally (cursor up/down, buy or sell depending on the active
  view, toggle buy/sell, exit the shop — not the whole game) rather than
  adding new `Key` values for item selection. The shop's on-screen title
  is the POI's own name, not a hardcoded string — Solace's General Store,
  Haven's Market Stalls, and Tarsis's Old Sailor all reuse the exact same
  `handleShop`/`availableShopItems` code, so what's for sale is identical
  everywhere (a deliberate scope cut, see "Three shops now" below, not an
  oversight).
- **No downgrading protection.** Nothing stops buying a worse item by
  mistake. Same minimalism as one weapon/spell/dialogue-line precedent
  elsewhere in this project.

### Selling gear back

As of Milestone 28, the shop screen has a second view: pressing `i` while
inside a shop toggles between "Buying" (the existing catalog) and
"Selling" (`character::sellableItems`, one entry per carried
`inventory` item), with the same up/down-select-Enter-to-confirm
interaction either way.

**The resale price is an invented convention, not a sourced rule** — both
the PHB (Money and Equipment chapter) and the DMG (treasure chapter) were
searched (`pdftotext -layout` + text search for "sell"/"resale"/"resell")
specifically to check for a printed mundane-equipment buy-back rule before
picking a number. None exists in either book: the only "selling" passages
found are about merchants using deliberately mismatched coin-weights (PHB)
and magic-item trading (DMG), neither a resale-price table for ordinary
armor/weapons. So `character::sellItem` uses the near-universal RPG
convention instead — half of the item's real shop price, floored — flagged
here exactly the way `docs/COMBAT_NOTES.md`'s "damage floored at 1" note
flags its own unsourced-but-conventional number.

Only items that actually match something in the shop catalog are
sellable: any carried `ArmorId`, the shield, or a class's purchased
`WeaponUpgrade`. A class's **starting weapon** (Longsword, Mace,
Shortsword, Dagger, the Tinker's wrench) — which can land back in
inventory after buying and equipping an upgrade — was never itself sold
in any shop and has no established price, so it's marked unsellable
("cannot sell") rather than assigned an invented number for something
that was never actually for sale.

### Three shops now, one shared catalog

Haven's Market Stalls (`K`) and Tarsis's Old Sailor (`S`) each gained a
`SHOP` line alongside their existing `POI`/`TALK` content — no new POIs or
zone-grid changes, reusing already-written, already-justified tiles rather
than inventing new merchant characters with no grounding (Haven's stalls
already sell goods to passersby; Steel Pieces are established lore as "the
universal equivalent," so even Tarsis's xenophobic-toward-outsiders sailor
plausibly deals in it — see "Gold -> Steel" below). `K` was already the
zone's `TIMELINE_ANCHOR`; `SHOP` and `TIMELINE_ANCHOR` are independent
flags on the same POI, so this doesn't interfere with canon-character
encounters there. All three shops sell from the exact same
`availableShopItems` catalog — there's no per-location wares (Haven
doesn't sell anything Solace doesn't) — a real, documented scope cut, see
"Extending this later" below.

### Carried inventory and equip/unequip (Milestone 21)

Buying at the General Store no longer replaces whatever's equipped
directly — it adds the item to `Character::inventory`
(`std::vector<character::InventoryItem>`), a carried-but-not-worn list.
Equipping is a separate, deliberate action: press `i`
(`render::Key::Inventory`, always available, not gated on standing at a
shop POI) to open the inventory screen and pick a carried item to wear.
`character::equipInventoryItem` swaps it into the matching slot
(`equippedArmor`/`hasShield`/the `weapon*` fields) and pushes whatever was
previously equipped there back into `inventory` — gear is swapped, never
destroyed or discarded. `availableShopItems`'s "already owned" check now
looks at both the equipped slot and inventory contents, so buying a
duplicate of an identical armor tier/shield/weapon upgrade is blocked
(there's no reason to carry two).

Deliberately not modeled: there's no "unequip to nothing" action (going
fully bare-handed/unarmored isn't offered — equipping is always a swap
between two items, and a weapon slot is never actually empty since every
class starts with one); no sell-back (unchanged from before); no
encumbrance/weight limit (same minimalism as everywhere else). `i` also
works mid-adventure regardless of location, unlike `p` which needs a shop
POI underfoot.

### Gold -> Steel (Milestone 22)

Krynn's real currency, post-Cataclysm, is Steel Pieces (`stl`), not gold —
confirmed via `pdftotext` search of *Dragonlance Adventures* (TSR 2021):
"stl stands for steel pieces, the universal equivalent," used throughout
that book's own cost tables (e.g. Knights of Solamnia Circle-tithe
amounts). `character::Character::steelPieces` (and every dice/cost field
that used to say `gold`) is renamed accordingly, top to bottom: class
starting-currency dice, monster reward dice, shop prices, every in-game
"Gold: N gp" display. The underlying dice/prices themselves are unchanged
— this is a currency *name* accuracy fix, not a rebalance; the PHB's own
generic-D&D "gold piece" numbers (Table 43 starting funds, Table 47 armor
prices, Table 44 weapon prices) are applied as-is under the new name, same
as before the rename.

The save file's `GOLD` keyword became `STEEL`, but `SaveGame::load` still
accepts a legacy `GOLD` line too (a save written before this milestone) —
see `docs/GOTCHAS.md`.

### Potions (Milestone 42)

A new `ItemKind::Potion` (`character/Equipment.h`), alongside
Armor/Shield/Weapon: exactly one real item, a Potion of Healing, sold at
every shop's already-shared catalog (see "Three shops now, one shared
catalog" above). Sourced from the actual scanned DMG
(`pdftotext -layout`, page-image rendering unavailable in this
environment — `pdftoppm` isn't installed): the effect, "the potion
restores 2d4+2 hit points of damage," is clearly legible on p.142; the
price, 200gp, comes from the separate Magical Items treasure table
(p.134, "Healing" row, Table 88) and is applied as 200 Steel Pieces per
this project's established Gold -> Steel convention. Potion of Extra
Healing's own dice were present in the same scan but badly
OCR-garbled ("JdB +3," almost certainly 3d8+3 by shape and by real 2e
convention, but not independently confirmable without a page image) —
deliberately left out rather than guessed at.

**Lore framing, not a new mechanic.** Dragonlance canon establishes that
real clerical healing magic is gone from Krynn after the Cataclysm and
doesn't return until Goldmoon's Disks of Mishakal, early in *Dragons of
Autumn Twilight*'s own timeline — an ordinary General Store stocking
freshly-brewed magic healing would quietly contradict a setting detail
this project has otherwise respected carefully (no orcs, Kender barred
from Mage, etc.). Resolved by framing the shop's potion as a scavenged
**pre-Cataclysm relic**, not a merchant's own brew — old magic surviving
as found treasure needs no invented rule, just flavor text; the mechanics
are the real, unmodified DMG item.

**Buying and carrying**: unlike armor/weapons, a potion is stackable —
`ShopItem::alreadyOwned` is always `false` for it, so buying a second (or
third) is allowed on purpose. It's sellable back at the same invented
half-price convention already used for mundane gear (100 stl).

**Drinking**: two entry points, both via `character::drinkPotion`
(returns `PurchaseResult`, the same `{success, message}` shape `sellItem`
already reuses rather than a duplicate type) —
- Outside combat, press `i` (`GameLoop::handleInventory`) and `Enter` on a
  carried potion; the inventory screen now shows an `HP: current/max`
  line so the effect is visible immediately (previously this screen
  showed no HP at all).
- Mid-combat, press `i` (`GameLoop::runCombat`) to drink the first potion
  carried (`character::firstPotionIndex`) as the round's action instead
  of attacking — the exact same "local key reinterpretation instead of a
  new `Key` value" trick `handleShop` already uses for `'i'` (there it
  toggles buy/sell; in combat it drinks). No potion carried logs "You
  have no potions." and doesn't consume the round, same forgiving pattern
  `Cast` already follows for "no spell available." `drawCombatFrame`'s
  footer only shows `i=drink potion` when one is actually carried, same
  "only hint what's usable" precedent `m=cast` already follows for
  non-casters.

Every carried potion is identical, so there's nothing to actually pick
between multiple ones — same "nothing to select" simplification
Spellcasting's one-known-spell already established.

### Magic items

Two items, sourced from *Dragonlance Adventures* (TSR 2021)'s own
"Magical Items of Krynn" chapter (pp.91-99, visually confirmed via
rendered page images) and the 2nd ed. DMG's "Magical Item Tables" —
picked deliberately over the chapter's unique, canon-owned artifacts
(Wyrmslayer, Staff of Magius, the Hammer of Kharas, the Dragonlances
themselves), which stay off-limits to the player character for the same
reason Alhana/Derek/Gunthar stay off-stage and Sturm's/Raistlin's/Flint's
own story beats aren't replayable — see `docs/QUEST_NOTES.md`'s "Shipped
quests" for the design discussion.

**A "+1" enchanted weapon**, one per class, sold at every shop alongside
the mundane upgrade (`character::magicWeaponFor(ClassId)`, same shape as
`weaponUpgradeFor` but never `nullptr`). Sourced from the DMG's Table 109
(Attack Roll Adjustment, p.140, visually confirmed): a "+1" weapon carries
XP Value 400 (Sword) or 500 (Other Weapon) — reused directly as the Steel
Piece price, the same "reuse the DMG number as the price" convention
Milestone 42 used for the Potion, flagged here too since Table 109 gives
an XP crafting cost rather than a separate market-value column. The magic
bonus applies to **both** the attack roll and damage (real 2e convention),
which needed a genuinely new field — `Character::weaponMagicBonus`,
distinct from `weaponDamageBonus` (a mundane weapon's own base damage die
bonus, e.g. the Cleric's Footman's Flail, which carries no to-hit bonus of
its own) — since nothing in `combat::resolvePlayerAttack` previously read
anything but Strength for to-hit. Mage and Tinker, who have no mundane
weapon upgrade at all (`weaponUpgradeFor` returns `nullptr` for both —
see "Equipment" above), get their *first* weapon upgrade this way, via
magic rather than smithing — Ensorcelled Dagger and Ensorcelled Wrench
respectively.

**Solamnic Armor**, a quest reward (`data/quests.txt`'s `solamnic_armor`,
see `docs/QUEST_NOTES.md`), sourced directly from DLA p.93-94 (visually
confirmed): *"Solamnic armor is equal to AC 0 (plate +1 and shield +1).
It is only granted to those Knights who have demonstrated the finest
qualities of Knighthood... only available at a Circle of Knights."* The
book gates this on the title "Lord," a rank this project doesn't model
(only Crown/Sword exist — see "Knights of Solamnia" above); scoped
instead to already being a Knight of the Sword
(`game::conditionMatches`'s new `sword_knight` token, distinct from the
existing `knight` token which also matches Crown), framed as the Circle
recognizing service already proven. A new `ArmorId::SolamnicArmor`
(`armorClass = 0`, matching the book's number exactly, never sold —
absent from `kBuyableArmor`) plus an ordinary, already-existing Shield
item granted alongside it — a **deliberate simplification** of the book's
separate "shield +1": this engine's shield is a flat boolean bonus with no
enchantment tiers of its own, and modeling a distinct "+1 shield" would
double the new surface area for one extra point of AC. Marked unsellable
in `resaleValueStl` (same reasoning as a starting weapon that was never
bought: no established price, since it was never for sale).

`SaveGame.cpp` touches: `ArmorId`'s bound widened 4→5 (`SolamnicArmor` is
ordinal 4, append-only-safe, same precedent as `KnightOrder` 2→3 at
Milestone 53). The equipped/inventory weapon line moved from `WEAPON
sides bonus name` to `MAGICWEAPON sides bonus magicBonus name` — a new
field couldn't be inserted into the old format without corrupting it
(`name` is a greedy to-end-of-line read), so `save()` now writes
`MAGICWEAPON` unconditionally and `load()` accepts both it and the legacy
`WEAPON` keyword (`magicBonus` defaults to 0), the same "old keyword still
read, new keyword is what's written" migration as `GOLD`→`STEEL`.

## Where a character lives

`CharacterCreator::run()` executes once, in `main.cpp`, before `GameState`
or `GameLoop` exist — it's plain `std::cin`/`std::cout` prompts, not
`render::Console::readKey()` (see `docs/ARCHITECTURE.md` on why creation is
a different interaction mode from the real-time game loop). The result is
stored in `GameState::character` and never reassigned after that; pressing
`c` during play (`render::Key::Sheet`) just displays it via
`MapRenderer::drawCharacterSheet`, it doesn't let you re-run creation.

## Extending this later

- **Leveling/XP past level 20**: `Leveling.cpp`'s XP tables now go the
  full 1–20 the PHB itself prints for these four classes; the book
  doesn't give single-class thresholds beyond that either, so extending
  further would mean a different (optional/epic-level) rule set, not
  just more of the same table.
- **Fighter's extra attacks per round** (level 7+, PHB Table 15): needs
  the combat round loop restructured to resolve more than one attack per
  side — see "Leveling / experience" above and `docs/COMBAT_NOTES.md`.
- **Spellcasting past 1st level**: Mage and Cleric each know exactly one
  spell (see "Spellcasting" above) with real per-day slot counts; actual
  spell selection/spellbooks and spells above 1st level are still future
  work -- once a real spellbook exists, Rest's one-keypress-memorizes-
  everything simplification (see "Spellcasting" above) needs to become a
  real selection step. Elf/Half-Elf's sleep/charm magic resistance and
  Wizard Robe spell-sphere restrictions remain unenforced since nothing
  currently in the game triggers either (no sleep/charm spell exists,
  and the one Mage spell isn't sphere-restricted).
- **Equipment/inventory, past what exists now**: armor/weapon purchases,
  a carried inventory, sell-back, three shops (Solace, Haven, Tarsis), a
  Potion, and a "+1" magic weapon/Solamnic Armor all exist now (see
  "Equipment" and "Magic items" above). Still missing: per-location wares
  (every shop sells the identical catalog), armor weight/encumbrance, and
  any item types beyond armor/shield/weapon/potion (scrolls, tools). The
  DLA magic items chapter has real, sourced content for more of these
  (Rods/Staves/Wands, Crystals and Gems, Miscellaneous Magic) if wanted
  later — see "Magic items" above for why the chapter's unique named
  artifacts specifically stay out of scope.
- **Rose Knights, for real**: Sword shipped at Milestone 53 (see "Knights
  of Solamnia" above); Rose still needs its own XP threshold, ability
  minimums (Str 15/Int 10/Wis 13/Dex 12/Con 15 — visually confirmed on
  p.19 during Milestone 53's research, see "Sourcing note: a book erratum"
  above), and witnessed-quest content, the same shape of work Sword just
  went through.
- **Sword Knight's real healing/foresight/clerical-spell abilities and
  weekly fasting/meditation ritual** (p.19-20): not modeled, same
  "flavor-only, needs a fuller spell system" treatment already given to
  Wizard Robe spell-sphere restrictions below — this project's Sword
  Knights get the title only, not the book's limited-cleric powers.
- **Wizard Robe mechanics, for real**: Robe assignment by alignment at
  3rd level is implemented (see "Leveling / experience" above); robe-based
  spell-sphere restrictions and moon-phase (Solinari/Lunitari/Nuitari)
  bonuses still need a spell system to attach to.
- **Gully Dwarf**: needs its own ability-score generation method (e.g.
  Strength 4d4+2) instead of 3d6 — see "Elf and Dwarf subraces" above for
  why it wasn't approximated instead.
- **Ability score ranges and class level limits** for every race/subrace
  (not just Kender) — a real 2e/Dragonlance mechanic, consistently left
  unenforced across this whole project so far; would need a decision on
  what happens when a roll falls outside a race's range (reroll that one
  score? clamp it?) before implementing.
- **Cavalier class**: would let Knights of Solamnia be modeled on their
  actual book chassis instead of the current Fighter-plus-flag
  simplification — see "Knights of Solamnia" above.
- **Tinker's device-construction minigame**: a real, sizable subsystem
  (Complexity/cost/build-time tables, a Success/Unpredictable/Failure
  invention roll) — see "Tinker Gnome" above for what's already sourced and
  what still needs the two illegible table pages (printed pp.118–119)
  re-rendered.
- **Tinker's saving throws/starting steel**: currently a documented analogy
  to Mage (the source book doesn't print class-specific numbers for its own
  Dragonlance classes at all) — revisit if a better-grounded source or
  a deliberate house-rule choice ever supersedes the analogy.
