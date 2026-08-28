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

**4d6, drop the lowest die, six times, assign to taste** — 2e's "Method
V" dice, confirmed verbatim on PHB p.19 ("Roll four six-sided dice (4d6).
Discard the lowest die and total the remaining three. Repeat this five
more times, then assign the six numbers to the character's abilities
however you want"), with the pre-existing house rule kept: the player may
**reroll the whole set of six as many times as they like** before
assigning. Implemented as `character::roll4d6DropLowest()`
(`character/Dice.h`/`.cpp`) plus an interactive assignment loop in
`character::CharacterCreator::run`.

**Supersedes an earlier decision, deliberately.** This project previously
used Method I (3d6 straight down the line, fixed STR→CHA order, same free
whole-set reroll) specifically chosen *over* Method II (4d6 drop lowest,
arrange to taste). A Milestone 69 request to redesign character creation
as a colorized, screen-per-step wizard modeled on a reference screenshot
(`References/abilityscore.png`) turned out to depict exactly Method V's
assign-to-taste interaction ("Assigning: Strength", pick from the pool of
rolled values) — confirmed against the real PHB page (rendered as an
image; this page's two-column layout mis-orders under `pdftotext
-layout`) rather than assumed from the screenshot alone. Asked directly,
the project owner chose to switch. See `docs/MILESTONES.md`'s Milestone
69 entry.

## Racial magic resistance (Dwarf, Gnome, Kender)

Per PHB p.28 (Dwarf), and Dragonlance Adventures p.53 (Kender, "all
standard halfling abilities" — the book builds Kender on the PHB's
Halfling chassis, even though this project has no separate playable
Halfling race, see "Kender in place of Half-Orc and Halfling" below):
these races gain a bonus against magic that scales with Constitution via
Table 9 (`character::constitutionMagicResistanceBonus`), not a flat
number. The bonus applies to Rod/Staff/Wand and Spell saves for all three
races, and *additionally* to the Paralyzation/Poison/Death category
(specifically representing poison resistance) for Dwarf and Kender — but
**not** Gnome, whose PHB entry (p.30) only mentions the wand/staff/rod/
spell bonus. See `character::applyRacialSavingThrowBonus`.

Elf and Half-Elf have a *different* ability — flat percentage magic
resistance (90% and 30% respectively) specifically against sleep and
charm-related spells (PHB pp.29–31) — which doesn't correspond to any of
our five save categories and isn't mechanically represented yet (there's
no spell-casting system to resist). Not a bug: this is a genuine gap
between the source material's granularity and what the game currently
models, left for whenever a magic system exists to make it meaningful.

## Kender in place of Half-Orc and Halfling

Ability adjustments (STR −1, DEX +2) and the "cannot learn Mage/illusionist
spells" restriction are from Dragonlance Adventures (TSR 2021), p.53 —
kender have innate magic resistance that specifically blocks arcane
spellcasting. Halfling was removed as a separate playable race for the
same reason Half-Orc was never one: Krynn has no distinct Halfling people
in Dragonlance canon — Kender fill that niche (and, per the book's own
"all standard halfling abilities" line above, are mechanically built on
the same PHB Halfling chassis anyway). `RaceId::Halfling` no longer
exists. This restriction is **hard-blocked** in character creation as
of "Ability score ranges and class level limits" below (a kender picking
Mage is rejected and reprompted) — a change from this project's earlier
"flagged, not hard-blocked" UX, which is still how the *unrelated*
prime-requisite-minimum check works elsewhere (see `CharacterCreator::run`).
Kender racial ability *ranges* (min/max caps) and class level limits from
the same source are enforced too — see "Ability score ranges and class
level limits" below.

**Kender cannot be Evil** (a later, user-requested content pass): the same
DLA p.53 "Kender Game Statistics" box states plainly, right after the
monk-ineligibility note, "No evil kender are known to exist." This is the
one and only race-based alignment restriction this project enforces (see
the "Scope" section below) — `character::meetsAlignmentRestriction`
(`Race.h`/`.cpp`) returns false only for Kender + one of the three Evil
alignments (`Alignment::LawfulEvil`/`NeutralEvil`/`ChaoticEvil`), true for
every other race/alignment combination. Hard-enforced the same
reject-and-reprompt way as the Mage block above and the ability-range/
class-cap checks below, not the older soft-flag UX — `CharacterCreator`'s
alignment prompt annotates each Evil option "(kender cannot be evil)" for
a Kender and re-prompts on an attempted pick, the same shape as the race/
class prompts' own ineligibility handling.

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
- ~~**No demihuman level limits.** Irrelevant until leveling exists.~~ Shipped
  once leveling did — see "Ability score ranges and class level limits"
  below.
- ~~**No alignment restrictions.** The player picks freely from all 9
  alignments regardless of race/class.~~ One real, sourced exception now
  exists: Kender cannot be Evil (DLA p.53 — see "Kender in place of
  Half-Orc and Halfling" below). Every other race/class combination is
  still unrestricted.
- **No spellbook or spell selection at character creation.** A Mage or
  Cleric character just knows their class at level 1; the real spell
  loadout picker happens later, at the character's first Rest (see
  "Spellcasting" below — a real multi-level system now, not the single
  fixed spell this line originally described). (Kender specifically are
  flagged as unable to cast arcane magic at all — see above — but nothing
  stops them from being *chosen* as Mage today.)
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

Five core 2e PHB races (Human, Dwarf, Elf, Gnome, Half-Elf) plus Kender
(see above, replacing both Half-Orc and Halfling), and the four
foundational classes (Fighter, Mage, Cleric, Thief — confirmed on PHB p.35
as "the standard classes... appropriate to any sort of AD&D game
campaign"), plus a fifth,
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

`character::effectiveCanBeMage(race, subrace)` is now a one-line wrapper
over `classLevelCap(race, subrace, ClassId::Mage) > 0` (see "Ability score
ranges and class level limits" below) — this is still how Kagonesti/Hill/
Mountain block Mage while Silvanesti/Qualinesti don't, just derived from
the same table that also drives Fighter/Cleric/Thief now, instead of its
own separately-maintained `canBeMage` bool.

**Gully Dwarf is deliberately not included** as a third Dwarf subrace
option, even though it's in the source book (p.69) and was on this
project's own "extend later" list. The book generates Gully Dwarf ability
scores with an entirely different method (e.g. Strength 4d4+2, Intelligence
2d4+1) instead of this project's own 4d6-drop-lowest-assign-to-taste
generation. Approximating it with a flat
ability adjustment (the way every other subrace works) would misrepresent
a genuinely different generation method rather than honestly model it, so
it's left as clearly-flagged future work instead of a fudged approximation.

## Ability score ranges and class level limits

Shipped at Milestone 109 (see `docs/MILESTONES.md`), closing the gap the
"Not modeled, deliberately" note used to flag here and in the Kender
section above. Two real 2e mechanics,
both keyed off a race or subrace, both checked in `CharacterCreator::run`
against the character's *base*, pre-racial-adjustment rolled scores (PHB
p.27: "Consult Table 7 *before* making any racial adjustments... Once you
satisfy the requirements at the start, you never have to worry about them
again"):

- **Ability score ranges** (`character::meetsAbilityRange`/
  `meetsSubraceAbilityRange`, `Race.h`/`.cpp`): PHB Table 7 (p.27) for the
  base Gnome/Half-Elf entries, Dragonlance Adventures p.53 for Kender, and
  the same per-subrace pages already cited above (p.60/61/62
  Elves, p.67/68 Dwarves) for Silvanesti/Qualinesti/Kagonesti/Hill/
  Mountain — visually confirmed via rendered page images, not the scan's
  badly-garbled OCR text layer. Human has no Table 7 entry at all (p.27:
  "Any character can be a human, if the player so desires") and is always
  selectable regardless of scores. **Hard-enforced**: the race and Elf/
  Dwarf-subrace prompts reject a pick whose scores don't qualify and
  reprompt, rather than merely annotating it — Human staying always-legal
  means the player can never be dead-ended with no valid choice.
- **Class eligibility and level limits** (`character::classLevelCap`,
  `Race.h`/`.cpp`): DMG Table 7, "Racial Class and Level Limits" (2e DMG
  p.15) — genuinely **not** in the PHB, which explicitly defers this to
  "ask your DM for the level limits imposed on nonhuman characters" (p.27),
  the reason a second rulebook needed to be consulted at all — plus
  Dragonlance Adventures' own per-subrace/Kender class-limit tables (same
  pages as their ability ranges above). Returns 0 for a class the
  sourcebook marks "N/E" (not eligible at all) or this project's own real
  level ceiling, 20, standing in for "Unlimited"/"U". **Hard-enforced** at
  the class prompt, generalizing the old Mage-only `canBeMage` block to all
  four core classes. Fixed a real, previously-live mismatch this
  cross-check turned up: Silvanesti Elf could be a Thief (Table 7 says
  "—") — now blocked. (A second mismatch it also found, Halfling being
  allowed as a Mage, was mooted by Halfling's later removal as a playable
  race entirely — see "Kender in place of Half-Orc and Halfling" above.)
  `Leveling.cpp`'s `applyPendingLevelUps` stops converting XP into levels
  once a demihuman hits their class's cap, matching the DMG's own "cannot
  advance beyond the listed level" verbatim — XP itself keeps accruing
  unmodified, no new save field needed.
- **Project interpretation, not a purely mechanical fact**: Dragonlance
  Adventures splits both Mage and Cleric into two parallel tracks with
  different caps — Magic-User (Renegade) vs. Wizard of High Sorcery, and
  Cleric (Heathen) vs. Holy Orders of the Stars. This project's Mage/Cleric
  classes don't distinguish these, so `classLevelCap` maps onto the
  licensed/sanctioned row (Wizard of High Sorcery, Holy Orders of the
  Stars) for both, matching how they're already played elsewhere in this
  project (the Test of High Sorcery robe assignment at level 3; real
  Cleric spells from the start, not a pre-Goldmoon "Heathen" restriction).
- **Deliberately not modeled**: two DM-optional/edge-case "exceed the cap
  via an exceptional prime requisite" bonus-level mechanics — DMG Table 8
  ("Exceeding Level Limits," explicitly labeled an *Optional Rule*) and
  Kender's own DLA p.53 footnote (Strength 17 → Fighter cap 6, Strength
  18 → cap 7). Skipped for the same "flag it, don't build every edge case"
  restraint already applied to Sword/Rose Knight's unmodeled real
  abilities.

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
and a maximum Wisdom of 12 for Tinkers — **not enforced**: this is a
class-specific requirement, not a race-level PHB Table 7/DLA entry (see
"Ability score ranges and class level limits" below, which covers race,
not class, ability requirements), and Gnome has no class-choice prompt to
gate in the first place (every Gnome PC is unconditionally a Tinker).

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
not a rank. The **Order of the Rose**, Solamnia's highest rank, is real as
of the Order of the Rose milestone — a `character::KnightOrder::Rose` value
plus the `measure_of_roses` quest (`docs/QUEST_NOTES.md`), see "Entry
requirements for Knight of the Rose" below.

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
values are used above as the real Sword minimums; p.19's values are the
real Rose minimums, used below.

**Entry requirements for Knight of the Rose** (p.19, "Knights of the Rose /
Game Data", visually confirmed on the rendered page — see "Sourcing note: a
second inconsistency" below): ability minimums **Strength 15, Intelligence
10, Wisdom 13, Dexterity 12, Constitution 15** (again no Charisma minimum)
— every minimum equal to or higher than Sword's. Implemented in
`character::meetsKnightOfRoseRequirements` (scores only, same reasoning as
Sword's own check) and `game::conditionMatches`'s `rose_eligible` token
(`knightOrder==Sword && level>=4 && meetsKnightOfRoseRequirements(scores)`).
The book's further "witnessed quest" requirement (a journey of ≥500
miles/30 days, one test of wisdom, three of generosity, three of
compassion, the restoration of something lost, and the defeat of an evil
opponent of equal or higher level than the candidate, without killing it)
is `data/quests.txt`'s `measure_of_roses` quest — see `docs/QUEST_NOTES.md`
for how each element maps (or doesn't) onto real objective state.

**Sourcing note: a second inconsistency.** The Rose "Minimum Requirements"
prose (p.19) says a candidate must have "gained two levels as Knights of
the Crown, then been accepted as Knights of the Sword and earned two
additional levels" before being considered for the Rose — arithmetically
level 5, since Sword is entered at level 3 (see above). The very next
sentence contradicts this: "Once a character has earned sufficient hit
points to become 4th level, he can petition the Order of the Rose." Both
readings were checked against a rendered page image, ruling out an OCR
misread. The tie-breaker is the **Rose Knight Advancement Table** printed
on the same page, which starts at **level 4** ("Novice of Roses") — the
same kind of hard-table-over-loose-prose resolution used for the p.18/p.19
Sword erratum above. `rose_eligible` uses `level>=4` accordingly, the exact
parallel of how Sword's own "2d level... XP to gain 3d level" prose
resolves to (and is confirmed by) its own table starting at level 3.

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
source page covering Human/Half-Elf/Kender/Gnome Knighthood eligibility
turns up.

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
Spellcasting itself now covers many spells (see "Spellcasting" below), but
Robe-based spell-sphere restrictions and moon-phase
(Solinari/Lunitari/Nuitari) saving-throw/spellcasting bonuses remain a
deliberate scope cut, not an oversight — DQoK's own Red/White Robe
per-spell restriction was considered and rejected as not sourced from the
PHB or Dragonlance Adventures (the book itself states a wizard "is
unaffected by phases of the moons" below 3rd level anyway, so the moon-
phase half was never a level-1 concern either).

**Milestone 103**: the Test itself gained three distinct outcome passages
(`character::applyPendingLevelUps`, `Leveling.cpp`, switched on the
already-computed `RobeColor`) in place of one generic templated line.
Sourced from rendered page images of DLA pp.33–37 (this section's text is
column-garbled in `pdftotext`, unreadable without rendering) — "The Test of
High Sorcery"/"Wizards of the White/Red/Black Robes." The book gives no
single canonical Test to transcribe (each initiate's is individually
designed around their own weaknesses, and failure means death); instead it
lists design guidelines a DM builds a Test from — at least three trials
unsolvable by magic alone, a combat against a known ally, a solo combat
against a stronger-than-usual opponent. Each of the three new passages
freshly dramatizes one of those named elements rather than inventing
unrelated flavor: White reframes "unsolvable by magic" as refusing to
spend a trusted illusion for personal power; Red dramatizes the robe's own
defining "balance" identity (p.36, "the widest range of spells available")
as every trial resolving into a mercy-vs-cruelty choice and refusing both;
Black reframes the "combat against an ally" guideline as choosing yourself
over a friend, with the Conclave marking *that* choice, not the spell, as
the pass condition. `robeColorName`/`robeMoonName` (`WizardOrder.cpp`) are
still reused for the "you emerge a ___, sworn to ___" clause rather than
hardcoding robe/moon names into each passage.

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

**Modeled (as of Milestone 108)**: Fighter's extra attacks per round past
6th level (PHB Table 15, p.36) — `character::meleeAttacksThisRound`
(`Leveling.h`/`.cpp`), consumed by `GameLoop::runCombat`'s `playerAttacks`
lambda. 1–6: 1/round; 7–12: 3/2 rounds (this project's interpretation of
that rate: 1 attack on odd rounds of the fight, 2 on even — the book
states the rate but not which rounds carry the extra swing); 13+: 2/round.
Every non-Warrior-group class always gets 1. See `docs/COMBAT_NOTES.md`.

**Modeled** (see "Ability score ranges and class level limits" above): a
Fighter's `classLevelCap` applies here too, so a demihuman Knight of the
Crown/Sword stops advancing once their race's Fighter cap is reached, same
as any other Fighter.

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

`character::Spellcasting` (`Spellcasting.h/.cpp`) grew from Milestone 40's
one-known-spell-per-caster into a **real multi-level spellbook** covering
every spell level a character's own level unlocks (up to Mage 9th / Cleric
7th), still nowhere near a "cast anything in the PHB" simulator -- see the
census below for exactly what's in and what's deliberately left out.

### Sourcing: DQoK.pdf cross-referenced against the PHB

At the user's direction, the spell list was built from `References/
DQoK.pdf` -- the manual for *Dark Queen of Krynn*, an official TSR/SSI
Dragonlance gold-box computer game (not a rulebook) -- then verified
against the actual PHB, the same "don't trust a secondary source, check
the real book" discipline as everything else in this project. DQoK's own
4-column "Spell Descriptions" pages (real book pp.23-30, rendered as
images via `pdftoppm` since the OCR text layer scrambles the columns badly
-- see `docs/GOTCHAS.md`) gave a complete per-level census: **29 Cleric
spells across 7 levels, 59 Magic-User spells across 9 levels** (its Druid
spells were skipped -- not a class this project implements). Every one of
those 88 was checked against the PHB's own alphabetical spell index
(`"SpellName (Wiz N)"` / `"(Pr N)"` with real page numbers, pp.303+) --
**85 matched exactly.** Three didn't, and were corrected rather than
silently ported from the game manual:

- **"Resist Cold"** (DQoK: Cleric 1st level, standalone) -- the real PHB
  spell is a single 2nd-level **Resist Fire/Resist Cold** covering both
  (DQoK separately and correctly places "Resist Fire" at 2nd level, so
  this is DQoK's own inconsistency, not a real 2e split).
- **"Iron Skin"** (DQoK: Mage 5th level, "-4 AC") -- not a real PHB spell
  name. The real analog is **Stoneskin** (PHB p.208, Wiz4), a 4th-level
  spell with a completely different mechanic (absorbs a number of hits
  outright, not an AC bonus) -- DQoK invented a simplified computer-game
  reskin. Not sourced, not implemented.
- **"Fire Touch"** (DQoK: Mage 5th level) -- doesn't appear anywhere in
  the PHB's Wizard spell index under any name. Appears to be a DQoK-
  original invention. Not sourced, not implemented.

Tables 21 (Wizard Spell Progression, PHB p.43) and 24 (Priest Spell
Progression, PHB p.47) were re-rendered and transcribed **in full** (every
spell-level column, levels 1-20) for `spellSlotsPerDay` -- Milestone 40 had
only transcribed each table's 1st-level column, which this pass's own
1st-level numbers still match exactly (a good cross-check that both
research passes read the same page correctly). Table 24's footnote --
6th-level slots need Wisdom 17+, 7th-level need Wisdom 18+ -- is enforced
directly in `spellSlotsPerDay` (a Cleric below that Wisdom gets 0 in that
column regardless of what the raw table says). Wisdom bonus spells (Table
5, p.23) still only cover 1st-level slots, the exact scope Milestone 40
already had -- Table 5's bonus-spell breakdown at higher spell levels
wasn't re-verified this pass, so nothing claims to model it.

### "Known spells" = the whole accessible-level roster

Real 2e Clerics need no spellbook -- "all spells of the appropriate level
are always available" (matches DQoK's own rules text almost verbatim).
Real 2e Wizards do need one, gated by an Intelligence-based "chance to
learn" roll this project doesn't implement (see "Scope" above, unchanged
since character creation). Rather than build that second, smaller
subsystem for one class, **both classes get the Priest's rule**: a caster
automatically knows every implemented spell (see the census below) at
every level their own character level unlocks. This is a documented
simplification of the Wizard's real spell-research/spellbook rules, called
out here exactly once rather than re-flagged at every spell.

### The spell census: 49 implemented, 39 sourced-but-excluded

Of the 88 real spells found above, **only a spell whose PHB effect maps
onto state this engine already tracks is actually castable** -- a
spell needing a subsystem that doesn't exist (poison/disease/blindness/
curse status, monster saving throws, damage-type resistance, multi-attack
rounds, item identification, locks/traps, ally summoning, NPC charisma
reactions) is sourced and documented here but never offered in-game, the
same "restraint over completeness" precedent Milestone 56 used for the
rest of DLA's magic item chapter. This keeps the player from ever
memorizing a spell that silently does nothing.

Every implemented spell resolves through `character::SpellEffect`, one of
six categories, each reusing a mechanism this project already had rather
than inventing six new ones:

| `SpellEffect` | Reuses | Engine detail |
|---|---|---|
| `DamageMonster` | Magic Missile's existing branch | subtracts from `monsterHp` |
| `HealCaster` | Cure Light Wounds's existing branch | adds to `currentHp`, capped at `maxHp` |
| `BlockMonsterAttacks` | Webnet (`blockedMonsterAttacks`, a count)/Brooch of Imog (`monsterIncapacitatedRestOfFight`) -- now one shared mechanism in `GameLoop::runCombat` | the sentinel `kBlockRestOfFight` means "until the fight ends," otherwise a counted number of the monster's attacks |
| `BuffPlayerThac0` / `BuffPlayerDamage` / `BuffPlayerAc` | new local variables in `runCombat`, passed as optional params to `combat::resolvePlayerAttack`/`resolveMonsterAttack` | **never** written into the character's real saved `armorClass`/`thac0` -- local to the one `runCombat` call, same "doesn't survive to the save file" precedent Brooch already established |
| `DebuffMonsterThac0` / `DebuffMonsterDamage` | same new local variables, applied against the monster's roll instead | |
| `BuffPlayerAndDebuffMonsterThac0` | both of the above at once | Prayer is the one spell that buffs the player and debuffs the monster in a single cast |
| `InstantDefeat` | nothing new -- sets `monsterHp = 0`, falls into the existing victory branch | Death Spell, Disintegrate, Flesh to Stone, Power Word Kill, and Cloudkill (its real "kills weaker monsters outright" framing, applied unconditionally since this engine has no monster Hit Dice threshold to gate it on) |

**No monster saving throws.** This engine has never modeled one (only the
player rolls saves, e.g. vs. the Giant Spider's poison bite). Every
damage/effect spell above that would normally allow a monster a save
instead applies at full, unconditional effect -- a flagged simplification,
same spirit as "damage floored at 1" in `docs/COMBAT_NOTES.md`.

**Cleric, implemented (14 of 29):**

| Spell | Level | Effect | Source |
|---|---|---|---|
| Bless | 1 | +1 player THAC0 | PHB p.252; DQoK p.24 |
| Cure Light Wounds | 1 | heal 1d8 | PHB p.253 (unchanged from Milestone 40) |
| Protection from Evil | 1 | +2 player AC | PHB p.271; DQoK p.24 |
| Hold Person | 2 | block, rest of fight | PHB p.261 |
| Spiritual Hammer | 2 | damage 1d6 | DQoK p.24 ("normal hammer damage" -- reuses the Cleric's own Mace die, PHB Table 44, not a fresh PHB citation) |
| Prayer | 3 | +1 player THAC0 / -1 monster THAC0 | PHB p.271; DQoK p.24 |
| Cure Serious Wounds | 4 | heal 2d8+1 | PHB p.253 |
| Protection from Evil, 10' Radius | 4 | +2 player AC | PHB p.271 |
| Sticks to Snakes | 4 | block, 3 attacks | PHB p.280 |
| Cure Critical Wounds | 5 | heal 3d8+3 | PHB p.253 |
| Dispel Evil | 5 | +7 player AC | DQoK p.25 |
| Flame Strike | 5 | damage 6d8 | DQoK p.25 |
| Blade Barrier | 6 | damage 8d8 | DQoK p.25 |
| Heal | 6 | heal to (`maxHp` - 1d4) | PHB p.253 |

**Cleric, sourced but excluded (15 of 29), and why:** Detect Magic, Find
Traps (no item-identification/trap system); Resist Fire/Resist Cold,
Silence 15' Radius (no damage-type or enemy-spellcasting system); Slow
Poison, Snake Charm, Cure Blindness, Cure Disease, Neutralize Poison,
Remove Curse (no poison/disease/blindness/curse status exists to cure);
Dispel Magic (nothing currently debuffs the player for it to remove);
Raise Dead, Resurrection, Restoration (this engine's "knocked out, not
killed" model means player characters never actually die).

**Mage, implemented (35 of 59):**

| Spell | Level | Effect | Source |
|---|---|---|---|
| Burning Hands | 1 | damage = caster level (flat, no die) | PHB p.170; DQoK p.26 |
| Charm Person | 1 | block, rest of fight | PHB p.171 |
| Enlarge | 1 | +1 player THAC0 | PHB p.173; DQoK p.26 |
| Magic Missile | 1 | damage, 1d4+1/missile, 1 missile/2 levels, capped at 5 | PHB p.176 (unchanged from Milestone 40) |
| Protection from Evil | 1 | +2 player AC | PHB p.271 |
| Shocking Grasp | 1 | damage 1d8 + caster level | PHB p.178 |
| Sleep | 1 | block, rest of fight | PHB p.178 |
| Mirror Image | 2 | block, 1d4 attacks | PHB p.186 |
| Ray of Enfeeblement | 2 | -2 monster damage | PHB p.187 |
| Stinking Cloud | 2 | block, 3 attacks | PHB p.188 |
| Strength | 2 | +2 player damage | PHB p.188 |
| Fireball | 3 | damage 1d6/level, capped 10d6 | PHB p.192, "a maximum of 10d6" |
| Hold Person | 3 | block, rest of fight | PHB p.193 |
| Lightning Bolt | 3 | damage 1d6/level, capped 10d6 | PHB p.194, "maximum ... of 10d6" |
| Protection from Evil, 10' Radius | 3 | +2 player AC | PHB p.195 |
| Slow | 3 | -2 monster THAC0 | PHB p.196 |
| Bestow Curse | 4 | -4 monster THAC0 | PHB; DQoK p.28 ("reduces THACO and saving throws by 4") |
| Charm Monster | 4 | block, rest of fight | PHB p.198 |
| Confusion | 4 | block, 4 attacks | PHB p.198 |
| Fear | 4 | block, rest of fight | PHB p.201 |
| Fumble | 4 | block, 3 attacks | PHB p.202 |
| Ice Storm | 4 | damage 3d10 (flat) | PHB p.202; DQoK p.29 ("3-30") |
| Cloudkill | 5 | instant defeat | PHB p.212 (simplified -- see "No monster saving throws" above) |
| Cone of Cold | 5 | damage, level x (1d4+1) | PHB p.212, "1d4+1 ... per level ... of the wizard," no cap |
| Hold Monster | 5 | block, rest of fight | PHB p.215 |
| Death Spell | 6 | instant defeat | PHB p.221 |
| Disintegrate | 6 | instant defeat | PHB |
| Flesh to Stone | 6 | instant defeat | PHB |
| Delayed Blast Fireball | 7 | damage 1d6/level, capped 10d6 | PHB, same dice as Fireball |
| Power Word, Stun | 7 | block, rest of fight | PHB p.237 |
| Mass Charm | 8 | block, rest of fight | PHB p.241 |
| Otto's Irresistible Dance | 8 | block, rest of fight | PHB p.241 |
| Power Word, Blind | 8 | -4 monster THAC0 | PHB p.238 |
| Meteor Swarm | 9 | damage, uniform 10-40 | PHB p.248; DQoK p.30's own "10-40" number is used directly rather than the real spell's four-separate-2d6-sphere total, flagged since it isn't an ordinary N*d*M roll |
| Power Word, Kill | 9 | instant defeat | PHB p.249 |

**Mage, sourced but excluded (24 of 59), and why:** Detect Magic, Read
Magic, Knock, Friends (no item-identification/lock/NPC-reaction system);
Shield, Protection from Normal Missiles, Minor Globe of Invulnerability,
Globe of Invulnerability, Mind Blank (only matter against an enemy
spellcaster or ranged attacker -- monsters in this engine never cast
spells or shoot); Detect Invisibility, Invisibility, Invisibility 10'
Radius, Mass Invisibility (no stealth system); Haste (needs the
already-deferred multi-attack-per-round engine feature, see "Leveling /
experience" above); Dimension Door, Blink (the existing Flee action
already always succeeds for free, so a "guaranteed escape" spell adds
nothing to model); Fire Shield (needs a reflect-damage mechanic not built
this pass); Remove Curse, Feeblemind (no curse/spellcasting-disable status
exists); Dispel Magic (nothing currently debuffs the player for it to
remove); Stone to Flesh (counters a status -- petrification -- nothing
inflicts); Mass Charm's own Charm Monster analog is separate and IS
implemented above; Iron Skin, Fire Touch (not real PHB spells, see
sourcing above); Monster Summoning (no ally-summoning system).

### Rest and spell memorization

Pressing `r` (`GameLoop::handleRest`) or `z` (`GameLoop::handleBedRest`,
standing on a `BED` POI) still rests once per in-game day
(`Character::lastRestDay`) and heals as before (see below) -- what changed
is what happens for a caster. `GameLoop::performSpellMemorization`:

- **First rest ever** (`Character::preferredSpellIds` empty): walks
  `chooseSpellLoadout`, a `drawPickerFrame` loop asking one spell per
  prepared slot, grouped lowest-level-first (only levels/spells
  `spellSlotsPerDay`/`spellListFor` actually allow are offered). The
  chosen ids become the new standing `preferredSpellIds`.
- **Every rest after that**: asks once, "Keep the same spells memorized?
  (Y/n)." Saying yes re-copies `preferredSpellIds` into
  `Character::memorizedSpellIds` -- this default, not a re-prompt every
  night, directly matches DQoK's own quoted design: *"Spells should be
  rememorized as soon as possible after they are used... Selecting REST
  without choosing new spells has the spellcasters rememorize the spells
  they have cast since last resting."* A level-up since the last rest
  that opened new slots gets them auto-filled with the roster's lowest-
  level spell rather than silently left empty. Saying no re-runs
  `chooseSpellLoadout` for a fresh loadout.

Real 2e's two-step memorization requirement (a restful night's sleep,
*then* 10 minutes of study per spell level, PHB p.107/p.111) is still
folded into the one Rest keypress, same simplification Milestone 40
already made and flagged -- now covering a real selection step instead of
"nothing to choose between."

`character::hasMemorizedSpellsAvailable` is a pure query: false unless
`Character::spellsCastDay` equals the day asked about AND
`memorizedSpellIds` is non-empty -- no slots are available at all until
memorization has happened that day, same no-silent-refill guarantee
Milestone 40 established.

Ordinary Rest still heals 1 hp (DMG p.74's base natural-healing rate) and
Bed Rest still heals fully to `maxHp` -- neither changed this pass, see
Milestone 40/41's original writeups below for their own sourcing.

### Real enforcement of the racial arcane-magic block

Unchanged in spirit from Milestone 40: Kender (`raceInfo(Kender).
canBeMage == false`) and the three subraces that can't be Mages (Kagonesti
Elf, Hill Dwarf, Mountain Dwarf, via `effectiveCanBeMage`) get
`spellSlotsPerDay == 0` at every spell level, regardless of character
level. Character creation still lets you *choose* Mage as one of these
(the usual non-blocking eligibility UX) -- the consequence just shows up
in combat.

### Viewing your spellbook

The character sheet's own "Spells memorized: ..." line is deliberately
terse (just what's prepared today). For the full picture, a caster gets an
extra hint on the sheet, `(s=view spells known, any other key to
continue)` -- pressing `s` (`render::Key::South`, locally reinterpreted
the same "instead of a new `Key` value" way `Cast`/`Inventory` already are
elsewhere) opens `MapRenderer::drawSpellbookFrame`: every implemented
spell for the character's class, grouped by level up to
`maxAccessibleSpellLevel`, each level line showing its real per-day slot
count, and each spell still memorized-and-uncast today marked `(memorized)`
/ `(memorized x2)`. Any key returns to the character sheet (not straight
back to gameplay), so a caster can flip between gear and spells in one
`c` session. Since a character is only ever one class at a time (no
multi/dual-classing exists), this naturally already shows "Mage spells"
or "Cleric spells" separately -- there's no case where both would need
splitting apart on screen.

### In combat

`GameLoop::runCombat`'s `m`/`M` (`render::Key::Cast`) casts the character's
one remaining memorized spell directly if there's only one distinct id
left (preserves the exact original one-spell UX for a low-level
character); with more than one, it opens a `drawPickerFrame` picker
("Cast which spell?", showing `x2`/`x3` counts for a spell memorized into
more than one slot) before spending the round. See `docs/COMBAT_NOTES.md`.

### Save format

`SPELLSTODAY <count> <day>` (one line, a bare cast-count) is replaced by
`SPELLDAY <day>` plus `PREFERRED <count> <id>...` / `MEMORIZED <count>
<id>...` (the standing loadout and what's left to cast today,
respectively). A save written before this milestone still has only
`SPELLSTODAY` -- still accepted on load (only the day carries over;
`memorizedSpellIds` simply stays empty, i.e. "nothing to cast until you
next rest," never a crash), same "old keyword still read, new keyword is
what's written" migration as `GOLD`->`STEEL`.

**Not modeled, deliberately, and still real gaps**: Elf/Half-Elf's
sleep/charm magic resistance still has nothing to resist (none of the
block-category spells above roll a save this engine could apply it to);
Wizard Robe spell-sphere restrictions -- DQoK has its own Red/White Robe
per-spell restriction, but it's that computer game's own balancing
invention, not sourced from the PHB or Dragonlance Adventures, and this
project's Robe assignment (level 3, see "Wizards of High Sorcery" above)
is still flavor-only with nothing to attach a restriction to -- **skipped
this pass by design**, not an oversight, flagged here since it's a real
Dragonlance-flavor call rather than a purely mechanical one. Tinker still
never casts despite borrowing Mage's saves/steel table as an analogy --
gadgets, not magic.

## Equipment

`character::Equipment` (`Equipment.h/.cpp`) lets a character buy armor and
one weapon upgrade at a shop POI (`p` while standing on one) — same "solve
the reported problem, not the whole simulation" restraint as Spellcasting's
census of implemented-vs-excluded spells. Everything below was visually confirmed against
rendered PHB pages (this scan's OCR badly garbles table columns, so text
search alone wasn't trusted for exact numbers — see `docs/GOTCHAS.md`):

- **Armor tiers**: Leather (AC 8, 5stl), Studded Leather (AC 7, 20stl),
  Hide Armor (AC 6, 15stl), Chain Mail (AC 5, 75stl), Splint Mail (AC 4,
  80stl), Plate Mail (AC 3, 600stl), Field Plate (AC 2, 1200stl) — Table 46
  (Armor Class Ratings, p.99) for the AC values, Table 47 (Armor, p.92) for
  cost (the PHB's own gold-piece numbers, applied here as Steel Pieces,
  Krynn's real currency — see "Gold -> Steel" below). Splint Mail is
  offered instead of Banded/Bronze Plate Mail for the same AC 4 tier
  because it's the cheapest real item at that protection level (80stl vs
  200stl/400stl). Studded Leather and Plate Mail (the equipment-expansion
  milestone) round out the curve with a budget mid-tier and a premium tier
  priced for a character with banked quest/kill rewards rather than
  starting steel.
  - **Hide Armor** (a later content pass, "more weapons and armor")
    closes a real gap this project used to skip entirely — Studded Leather
    (AC 7) used to jump straight to Chain Mail (AC 5), missing the AC 6
    tier Table 46 lists (Studded Leather+shield, Brigandine, Scale Mail,
    or Hide Armor). Hide is the cheapest real item at AC 6 (15gp vs
    Brigandine/Scale Mail's 120gp each) — same "pick the cheapest real
    item at this AC" rule Splint Mail already established above. It is
    genuinely cheaper than Studded Leather despite better AC, confirmed on
    a rendered page image (not a transcription error) — the PHB's real
    numbers just work out that way.
  - **Field Plate** (AC 2) was added in the same pass, reversing this
    document's own earlier "too expensive" scope cut — Full Plate (AC 1/0)
    is still not offered, since it would just duplicate Field Plate/
    Solamnic Armor's niche one slot up, and Solamnic Armor (below) already
    fills the AC 0 spot for a Sword Knight. **Its 1200stl price is a
    deliberate deviation from the real Table 47 number (2,000gp)** — done
    at the user's explicit request to fit the existing curve (Plate Mail's
    own 600stl was already framed as "priced for banked quest/kill
    rewards"; another raw 3.3x jump for one more AC point would put it out
    of reach even for a well-quested character). 1200stl (2x Plate Mail)
    keeps it a genuine late-game splurge instead. Flagged here exactly the
    way Hoopak's price below is flagged, for the same reason: an invented
    number where the real one exists but doesn't serve this game's economy.
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
- **One weapon upgrade per class** (Table 44, Weapons, p.94-95): Fighter's
  Long Sword (1d8) upgrades to Two-Handed Sword (1d10, 50stl); Cleric's
  Mace (1d6) upgrades to Footman's Flail (1d6+1, 15stl) — real 2e
  restricts standard clerics to blunt weapons only (this PHB's own Money
  and Equipment chapter: "Standard clerics ... are allowed to use only
  blunt, bludgeoning weapons," visually confirmed), which the Flail
  satisfies; Thief's Short Sword (1d6) upgrades to Long Sword (1d8,
  15stl). **Mage and Tinker's own upgrades were added by the
  equipment-expansion milestone**, closing a gap this document used to
  flag as deliberate: Mage's Dagger (1d4) upgrades to a Quarterstaff
  (1d6) — the PHB prices it "--" (a cut length of wood, no real cost), so
  its 2stl shop price is a small invented number, flagged in
  `character/Equipment.cpp` the same way `kWebnetCostStl` is; Tinker's
  Wrench upgrades to a Light Crossbow (1d4, 35stl, reusing the Light
  Quarrel's damage since this engine doesn't track ammunition separately
  from the weapon) — a mechanical weapon fitting the class's gadgeteer
  identity rather than a sword-and-board reskin. **Correction** (found
  while researching the Hoopak below): this was originally listed as
  1d4+1, but the real PHB Table 44 Light Quarrel line is 1d4 with no
  bonus — 1d4+1 is actually the Heavy Quarrel's damage, re-confirmed via a
  rendered page image.

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
  is the POI's own name, not a hardcoded string — every shop reuses the
  exact same `handleShop`/`availableShopItems` code, parameterized per
  shop by a `character::ShopCatalog` (see "Six shops, six catalogs"
  below) rather than each shop having its own bespoke logic.
- **No downgrading protection.** Nothing stops buying a worse item by
  mistake. Same minimalism as one weapon/spell/dialogue-line precedent
  elsewhere in this project.

### Hoopak (Kender-only weapon)

A later content pass ("more weapons and armor... hoopak for Kender!").
Unlike every weapon above (one upgrade per `ClassId`), the Hoopak is
**race-gated**, not class-gated: any Kender, regardless of class, gets
one — and can still buy their own class's weapon upgrade too, since the
two aren't mutually exclusive (see below).

Neither the PHB nor *Dragonlance Adventures* gives the hoopak any game
stats — both only mention it in passing (DLA p.53's "his hoopak or other
weapon," in the Kender Pocket Grab Table's exemption list; PG1 *Players
Guide to the Dragonlance Campaign* p.70/71 describes it at length in
flavor terms — a 5' ironwood sling-staff, spiked at one end, forked and
gut-laced at the other, "thrown as a spear... struck as a staff... shot or
slung with stones" — but no numbers). Real stats came from `References/
DQoK.pdf` (Dark Queen of Krynn, the official TSR/SSI Dragonlance computer
game manual — already this project's precedent source for the
Spellcasting census above), whose Weapons Table (printed p.51, PDF page
28, visually confirmed via a rendered page image, the OCR text layer being
badly garbled for this table like every other one in this project) gives
the hoopak two distinct profiles, both footnoted "Only usable by kender
characters":

| Mode | Damage vs. man-sized | Damage vs. larger |
|---|---|---|
| Hoopak (Melee) | 3-8 (1d6+2) | 3-6 (1d4+2) |
| Hoopak (Missile) | 2-5 (1d4+1) | 2-7 (1d6+1) |

At the time this was written, this engine had no ranged/melee distinction
for any weapon (the Tinker's Light Crossbow was then still used
identically to a melee weapon), so only one profile could be modeled —
the higher-damage **Melee** line was chosen, the same "pick the number
that matters, flag what's lost" simplification as Meteor Swarm's uniform
damage in the Spellcasting census above. **This project does have a real
ranged/melee distinction as of Milestone 114** (see
`docs/COMBAT_NOTES.md`'s "Positional combat grid" section) — the Light
Crossbow is now genuinely ranged in combat, but the Hoopak's own choice
above (Melee over Missile) was never revisited when that shipped; it's a
separate, standing decision, not an oversight. **Cost (50stl) is an invented, flagged number** — DQoK's
manual has no in-game currency to reuse (unlike the Potion/magic weapons,
which reuse real DMG gp values), so it's calibrated instead to the
Fighter's Two-Handed Sword, this project's closest real peer by average
damage (1d6+2 and 1d10 both average 5.5).

**A Kender character starts equipped with a Hoopak** (added at the user's
direct follow-up request, right after the shop-purchasable version
shipped): `CharacterCreator::run` sets `Character::weaponName`/
`weaponDamageSides`/`weaponDamageBonus` to the Hoopak's stats instead of
`ClassInfo::weaponName` whenever `character.race == RaceId::Kender`,
regardless of chosen class (Fighter/Cleric/Thief — Mage is already
race-blocked for Kender, see "Kender in place of Half-Orc and Halfling"
above). This doesn't remove or replace anything about the class's own
`WeaponUpgrade` — a Kender Fighter can still buy a Two-Handed Sword later,
same as before; only the level-1 starting weapon field changes. The
shop's own "already owned" check (`ownsWeapon`, matched by name against
`Character::weaponName`) correctly greys out buying a second Hoopak for a
freshly created Kender, with no special-casing needed — a Kender who
later sells or swaps theirs away can still buy a replacement.

Still sold at General/Armory/Bazaar (wherever a class weapon upgrade is
already sold — see "Six shops, six catalogs" below) for the rare case a
Kender needs one, always listed but greyed out for a non-Kender, same
"show it, don't hide it" precedent as Webnet/Brooch being Mage-gated.

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
sellable: any carried `ArmorId`, the shield, a class's purchased
`WeaponUpgrade`, or a purchased Hoopak (matched by name, not by class,
since it's race- not class-gated — see "Hoopak" above). A class's
**starting weapon** (Longsword, Mace,
Shortsword, Dagger, the Tinker's wrench) — which can land back in
inventory after buying and equipping an upgrade — was never itself sold
in any shop and has no established price, so it's marked unsellable
("cannot sell") rather than assigned an invented number for something
that was never actually for sale.

### Six shops, six catalogs

Milestone-100-era shops all sold from the exact same `availableShopItems`
catalog — "there's no per-location wares" was a documented scope cut.
Closed at the user's explicit request: `character::ShopItem` now carries
a `ShopItemKind` (`ArmorTier`/`Shield`/`WeaponUpgrade`/`KenderWeapon`/
`MagicWeapon`/`Potion`/`Webnet`/`Brooch`), and `availableShopItems`/`purchaseItem` take
a `character::ShopCatalog` that filters which of those kinds a given shop
offers (`Equipment.cpp`'s file-local `catalogDef` table). This also
replaced `purchaseItem`'s old fragile position-based offset math
(`armorCount + 1`-style arithmetic assuming one fixed, unfiltered
sequence) with a direct switch on `item.kind` — a correctness cleanup
the filtering needed anyway.

**Which catalog is invented gameplay tuning, not sourced content** — same
honesty as `docs/COMBAT_NOTES.md`'s `encounterChancePercent`/
`kBiasWeight`. Six catalogs, one per shop, chosen from each POI's own
already-written flavor text rather than arbitrary assignment:

| Catalog | Shop | Contents |
|---|---|---|
| `general` | Solace's General Store (`G`) | unchanged baseline: all 7 armor tiers, shield, class weapon upgrade, Hoopak (Kender only), magic weapon, potion, webnet/brooch |
| `armory` | Solace's Flint's Smithy (`S`, new) | all 7 armor tiers, shield, class weapon upgrade, Hoopak (Kender only), magic weapon — no potion/webnet/brooch (a smith, not an alchemist) |
| `market` | Haven's Market Stalls (`K`) | Leather + Studded Leather + Hide Armor, shield, potion only — a pedestrian goods market, budget tier only |
| `salvage` | Tarsis's Old Sailor (`S`) | potion, magic weapon only — extends the existing "scavenged pre-Cataclysm relic" framing (see "Potions" below) to a salvaged enchanted weapon too; no mundane armor/weapon/shield, a ruined port isn't an armorer |
| `bazaar` | Kalaman's Market Square (`M`, new) | Leather + Studded Leather + Hide Armor + Chain Mail armor, shield, class weapon upgrade, Hoopak (Kender only), potion — a real bazaar, but no Plate Mail/Field Plate or enchanted goods |
| `harbor` | Palanthas's Harbor (`H`, new) | all 7 armor tiers, shield, magic weapon, potion — the one surviving great port trades in finished goods, not mundane smithing (no weapon upgrade, so no Hoopak either) |

(Studded Leather and Plate Mail, added by the equipment-expansion
milestone, and Hide Armor/Field Plate, added by a later content pass, all
follow each catalog's already-established character rather than appearing
everywhere: General/Armory/Harbor carry the full 7-tier range; Market/
Bazaar gain the budget Studded Leather and (now) Hide Armor, but not the
premium Plate Mail/Field Plate; Salvage stays armorless, unchanged. The
Hoopak rides along wherever a class weapon upgrade is already sold —
General/Armory/Bazaar — since it's gated by the same `weaponUpgrade`
catalog flag rather than a new one of its own.)

**Webnet/Brooch of Imog are now General-Store-exclusive** — a real
behavior change from Milestone 56, which sold them at every shop. Called
out explicitly rather than silently: a Mage who wants either now has to
be in Solace.

**Flint's Smithy is gated behind a quest** (`SHOP_LOCKED`, see
`docs/ZONE_NOTES.md`): it won't open at all until `ore_for_the_forge` is
turned in, a real payoff for already-shipped content (the smithy's own
`TALK` text has always been about a stalled ore supply) rather than an
invented new mechanic. Known minor gap: the smithy's `TALK`/`TALK_AGAIN`
lines don't yet change once the quest completes and the shop unlocks —
same "not addressed by this pass" honesty as other documented
limitations in this project, not an oversight to hide.

Haven's Market Stalls (`K`) and Kalaman's Market Square (`M`) were/are
already each zone's `TIMELINE_ANCHOR`; `SHOP` and `TIMELINE_ANCHOR` are
independent flags on the same POI, so neither interferes with
canon-character encounters there. Palanthas's Harbor (`H`) and Kalaman's
Market Square needed no new `POI`/grid changes — both already existed as
flavor-appropriate, unclaimed POIs, reusing already-written tiles rather
than inventing new merchant characters with no grounding.

Three other `TOWN`-flagged locations (Crossing, Port Balifor, Flotsam)
were deliberately left without a shop: Crossing is a two-POI ferry
waypoint, not a town square; Port Balifor's harbor is explicitly
draconian-guarded, not a free market; Flotsam is a smugglers'/pirates'
haven whose captains "ask no questions," not a storefront. Same
"restraint over completeness" discipline as Plains of Dust/Tarsis having
no timeline content.

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
every shop as of this milestone (every catalog includes it -- see "Six
shops, six catalogs" above). Sourced from the actual scanned DMG
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
- Mid-combat, press `i` (`GameLoop::runCombat`) to use an item as the
  round's action instead of attacking — the exact same "local key
  reinterpretation instead of a new `Key` value" trick `handleShop`
  already uses for `'i'` (there it toggles buy/sell; in combat it uses).
  With a Potion the only usable item this auto-selects it directly, same
  as the original Milestone 42 UX; see the Webnet section below for how
  this behaves once more than one item is usable
  (`character::availableCombatItems`, Milestone 115). No potion (or
  anything else) carried logs "You have nothing to use." and doesn't
  consume the round, same forgiving pattern `Cast` already follows for
  "no spell available." `drawCombatFrame`'s command row only names a
  `USE:` item when one is actually usable, same "only hint what's
  usable" precedent `m=cast`/`CAST` already follows for
  non-casters.

Every carried potion is identical, so there's nothing to actually pick
between multiple ones — a "nothing to select" simplification, unlike the
real spell-loadout picker Spellcasting now has.

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

**Webnet and Brooch of Imog** (Milestone 56), two Mage-only combat items
sourced from *Dragonlance Adventures* p.93 (Miscellaneous Magic) and p.92
(Crystals and Gems), both visually confirmed via rendered page images
(book pages, not PDF pages — the DLA scan carries a confirmed +1 PDF-page
offset against its own printed page numbers). Picked over the rest of the
"Magical Items of Krynn" chapter specifically because they're the only two
entries whose real book effect maps onto a numeric combat outcome this
engine already has (an attack landing or not) without inventing a new
subsystem (charges, creature command/charm, translation flags, a plot-key
mechanic) just to place one item — see NEXT UP below for what was found
and left out.

Both are Mage-only per their own DLA text ("This item is only useful to a
magic-user") and sold at every shop alongside the existing catalog
(`character::availableShopItems`, listed-but-greyed-out for other classes,
same precedent as armor for a Mage). Neither has a book-printed Steel
Piece price (unlike the Potion/magic weapons above, reused from the DMG's
own tables) — `kWebnetCostStl` (150) and `kBroochOfImogCostStl` (500) are
invented, flagged in `character/Equipment.h`, calibrated relative to the
Potion (200stl, one-shot) and a "+1" weapon (400-500stl, permanent).

*Webnet*: "when worn by a mage who knows the command word, the webnet can
be cast... it instantly grows to a 10-foot-diameter net of entrapment."
Consumed on use (`character::useWebnet`, stackable like the Potion —
buying a second is allowed on purpose); negates the monster's *next*
attack. Used via the same `'i'`-in-combat local-key-reinterpretation
`GameLoop::runCombat` already uses for drinking a potion. **As originally
shipped this was a fixed priority chain (potion first, then Webnet, then
Brooch of Imog) that silently stopped at the first match — a character
carrying both a Potion and a Webnet could never actually reach the Webnet
through `'i'`. Milestone 115 replaced it with a real in-frame chooser
over everything usable (`character::availableCombatItems`), auto-picking
only when exactly one item qualifies — see `docs/COMBAT_NOTES.md`'s
"In-frame combat actions" section.**

*Brooch of Imog*: "can be used once per day to create a *minor globe of
invulnerability*. The globe lasts for 10 rounds." Unlike the Webnet, this
is **not** consumed (worn/carried, blocked from a duplicate purchase like
armor/weapons — a second one would grant nothing, since the daily charge
is tracked per-character) and gated to once per real in-game day exactly
like `Character::lastRestDay` already gates Rest — a new
`Character::lastBroochUseDay` field, same shape
(`character::broochAvailableToday`/`activateBrooch`). Activating it
negates **all** the monster's remaining attacks for the rest of the
current fight — a deliberate, flagged simplification of "10 rounds": this
engine has no round-duration tracker outside a single `runCombat` call,
and fights are short enough that "the rest of this fight" and "10 rounds"
are functionally the same thing.

Both effects ("does the monster's next attack land") are resolved as
plain **local variables inside `GameLoop::runCombat`**
(`blockNextMonsterAttack`/`globeActive`), exactly like `monsterHp` and the
combat log already are — nothing about whether an attack lands this fight
needs to survive to the save file, only whether the Brooch's daily charge
has been spent does. `SaveGame.cpp` touches: a new `BROOCHDAY` line
(optional on load, defaults to -1 — same backward-compatibility shape
`RESTDAY` already has, re-verified directly against the user's real save)
and two new bare-keyword inventory lines, `WEBNET`/`BROOCH`, alongside the
existing `ARMOR`/`SHIELD`/`POTION`/`MAGICWEAPON` ones.

**Staff of Striking/Curing**, DLA's next magic item after Webnet/Brooch of
Imog, sourced from p.91 (Rods, Staves, and Wands), visually confirmed via a
rendered page image. A quest reward (`data/quests.txt`'s
`staff_of_striking_curing`, see `docs/QUEST_NOTES.md`), Cleric-only via
`REQUIRE cleric` — the book frames it as "common among the clerics of the
Age of Might," and this project has no printed restriction to the contrary,
but the flavor precedent (Webnet/Brooch's own "only useful to a magic-user"
text gating them to Mage) made a class restriction the honest call.

The **striking** side needs no new mechanism at all: it's an ordinary
`ItemKind::Weapon` InventoryItem (`character::kStaffOfStrikingCuringName`),
magicBonus +3 (to-hit and damage, the same convention `MagicWeapon`
already established) and 1d6 base damage — "it strikes as a +3 weapon...
4-9 points of damage," i.e. 1d6+3. Equipping, unequipping, and resale
(unsellable — falls through `resaleValueStl`'s existing `ItemKind::Weapon`
case to `sellable = false` since the name matches neither the class's
mundane upgrade nor its `MagicWeapon`, no special-casing needed) all reuse
the exact machinery a "+1" magic weapon already has.

The **curing** side needed real design judgment, since the book's own
mechanics don't fit this engine cleanly. DLA says curing "drains two
charges for each cure," referencing the 2nd ed. DMG's own separately
defined *Staff of Curing* — but that item isn't in this project's 2e DMG
at all (a 1st-edition-only item DLA assumes without restating; confirmed
by a direct text search of the DMG extraction, not an OCR gap), so no heal
amount is available to source. **kStaffCureDiceSides is 1d8, reusing this
project's own already-PHB-sourced Cure Light Wounds dice**
(`character/Spellcasting.cpp`) rather than inventing an unrelated number —
naming parity with the real spell it stands in for, chosen over reusing
the Potion of Healing's 2d4+2 (a different, DMG-"Healing"-table-sourced
value with less naming justification).

The book's 50-charge pool (5/day recharge in sunlight) and its second
striking mode (spend 2 charges for double damage, then blocked from
curing for an hour per double-damage blow) are **both deliberately not
modeled** — the latter needs real-time cooldown tracking this engine has
never had, the same category of gap already flagged for the Golden
Circlet/Flute of Wind Dancing (see "Extending this later" below); and once
that's cut, the charge pool becomes vestigial. The book's other cap —
"no more than once per day on a given individual" — is already stricter
than any charge count, since this engine tracks exactly one player
character and there's no party to spread cures across: 5/day recharge
always outpaces the at-most-once/day-at-2-charges consumption, so the pool
could never actually run dry. Modeling a 50-charge counter that never
binds would be exactly the kind of unneeded complexity CLAUDE.md's "no
premature abstraction" warns against. Instead, curing is a flat
**once-per-day self-heal**, implemented with the identical shape as the
Brooch of Imog's own day-gate: `Character::lastStaffCureDay`
(`character::useStaffCure`/`staffCureAvailableToday`, mirroring
`activateBrooch`/`broochAvailableToday` exactly). Combat-only, same
simplification Webnet/Brooch already have (not exposed via the general
inventory screen — the item may or may not be in the browsable list
depending on whether it's currently equipped) — used via the same
`'i'`-in-combat handling as the Potion/Webnet/Brooch above (see the note
there: Milestone 115 replaced the original four-deep fixed priority chain
with a real chooser over everything usable).

`SaveGame.cpp` touches: one new optional `STAFFCUREDAY` line, same
backward-compatible shape as `RESTDAY`/`BROOCHDAY` (defaults to -1 if
absent, re-verified directly against the user's real save). No inventory
format change — the staff's weapon stats ride the existing `MAGICWEAPON`
line unchanged, same as any other named magic weapon.

**Frostreaver**, DLA's next magic item after the Staff of Striking/Curing,
sourced from p.94 (Weapons), visually confirmed via a rendered page image
(the item this project's own NEXT UP backlog had already flagged as a
real, buildable gap when the Staff was picked instead — see the
"Extending this later" section below, now trimmed). A quest reward
(`data/quests.txt`'s `frostreaver_salvage`), tied to the already-shipped
Ice Wall Castle location and Thanoi monster: *"the equivalent of a heavy
battle axe +4... can only be wielded by a character with a Strength of 13
or greater."* The PHB's Table 44 (Weapons, p.94) has no separate "heavy
battle axe" entry — its one axe line, plain "Battle axe," is 1d8 — so
that's the base damage die the "+4" sits on top of. Gated by
`REQUIRE str_13`, a new `game::conditionMatches` token
(`c.scores.strength >= character::kFrostreaverMinStrength`) — this
project's first quest requirement keyed on a raw ability score rather than
race/class/knight-rank, so the quest is never offered to a character who
couldn't wield the reward anyway.

The book gives Frostreaver a real weakness: above-freezing temperatures
melt it useless within a day (1d6 hours in a warm environment). This
engine has no "item destroyed by its environment" mechanic anywhere, and
building one just for this weapon would be exactly the premature
abstraction CLAUDE.md warns against — **simplified to "only carries its
+4 bonus while standing on glacier terrain,"** the exact deviation this
project's own earlier NEXT UP note already proposed. The granted
`InventoryItem` therefore carries `weaponMagicBonus = 0` (its honest
off-glacier baseline — just a mundane heavy battle axe, and
`inventoryItemLabel` shows it that way, "Frostreaver (1d8)," no "+N to
hit" suffix). The +4 is applied instead as a **this-fight-only local
bonus inside `game::GameLoop::runCombat`**, the identical mechanism
already used for spell buffs (`playerThac0Bonus`/`playerDamageBonus`, see
`combat::AttackOutcome`'s doc comment in `Combat.h`) — never written into
the character's permanent stats. `runCombat`'s only call site is
`tryMoveOverworld`, so the terrain the fight is happening on is always
`state_.x`/`state_.y`, recomputed with the same `world::terrainFor(grid_.
terrainCodeAt(...))` call `tryMoveOverworld` already makes; no new
parameter or signature change was needed anywhere in `combat::Combat.h`.
The bonus surfaces automatically in the existing "Showing the math"
bracketed roll breakdown, plus one flavor log line the round it first
applies.

Offered by Ice Wall's existing Young Knight POI (`K`) rather than a new
NPC — he already carries established Thanoi-flavor dialogue (`TALK
K`/`TOPIC K "The Walrus-Men"`), and the quest frames the axe as salvage
("an Ice Folk raider we found dead near the wall, killed by thanoi, not
us") rather than inventing a talking Ice Folk character never actually
placed at Ice Wall Castle in the novel's own scene there — the same
restraint that's kept Alhana/Derek/Gunthar off-stage. `SLAY thanoi 2`
(Thanoi are tough — 157 XP each — so 2 is calibrated down from the 3-kill
baseline weaker monsters use, matching `bazaar_road_raiders`'/
`staff_of_striking_curing`'s own 2-kill count against a comparably tough
target). Reward: 50 steel, 120 XP — same modest tier as `solamnic_armor`/
`staff_of_striking_curing`, since the item itself is the real reward.
Unsellable, same as the Staff — falls through `resaleValueStl`'s existing
`ItemKind::Weapon` case to `sellable = false` since "Frostreaver" matches
neither the class's mundane upgrade nor its `MagicWeapon`, no
special-casing needed. `SaveGame.cpp` touches: none — the `MAGICWEAPON`
line format is already fully generic over weapon name.

### Quest items (the DELIVER milestone)

`ItemKind::QuestItem`, the first inventory kind that isn't a piece of
purchasable/sourced equipment — a real, narrative object granted in the
world (via a zone POI's `GRANTS_ITEM` line, see `docs/ZONE_NOTES.md`) and
carried toward a `quest::ObjectiveKind::Deliver` objective (see
`docs/QUEST_NOTES.md`'s "DELIVER"). Unlike `ArmorId`'s small fixed table,
a quest item carries its own `questItemId`/`questItemName` directly on
the `InventoryItem` — the same "id + free display text" shape
`weaponName` already has for a `Weapon` — since quest items are one-off
objects, not a closed catalog. Never equippable, never sellable (no
established price, same treatment `SolamnicArmor` already gets), and
removed from inventory on turn-in rather than by the player directly —
there is deliberately no "drop" action. `SaveGame.cpp` touches: a new
`QUESTITEM <item-id> <display-name...>` inventory-entry keyword,
alongside `ARMOR`/`SHIELD`/`POTION`/`MAGICWEAPON`/`WEBNET`/`BROOCH`.

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
- **Spellcasting**: real multi-level spell selection/memorization now
  exists (see "Spellcasting" above) -- 49 of the 88 PHB/DQoK-sourced
  spells across Mage's 9 levels and Cleric's 7. The other 39 are sourced
  and documented but not castable, each needing a subsystem this project
  doesn't have (poison/disease/blindness/curse status, monster saving
  throws, damage-type resistance, stealth, locks/traps, ally summoning,
  NPC reactions) -- revisit individually as those subsystems get built,
  rather than all at once. Elf/Half-Elf's sleep/charm magic resistance
  still has nothing to resist (no block-category spell above rolls a
  save this engine could apply it to). Wizard Robe spell-sphere
  restrictions were a deliberate scope cut this pass (DQoK has its own
  Red/White Robe per-spell restriction, but it isn't PHB/DLA-sourced, and
  this project's Robe assignment is still flavor-only) -- a real design
  call, not an oversight, see "Spellcasting" above.
- **Equipment/inventory, past what exists now**: armor/weapon purchases,
  a carried inventory, sell-back, six shops across five towns with real
  per-location catalogs (see "Six shops, six catalogs" above), a Potion, a
  "+1" magic weapon/Solamnic Armor, (Milestone 56) a Webnet/Brooch of
  Imog, and (the DELIVER milestone) real quest items all exist now (see
  "Equipment", "Magic items", and "Quest items" above).
  Still missing: armor weight/encumbrance. Milestone 56 read the rest of
  DLA's "Magical Items of Krynn" chapter closely (Rods/Staves/Wands,
  Crystals and Gems, Miscellaneous Magic, Armor and Shields, Weapons); a
  later pass (see "Magic items" above) re-read the same chapter directly
  from rendered page images and shipped the Staff of Striking/Curing,
  modeled without the charge pool Milestone 56 originally flagged it as
  needing (see "Magic items" for why). The equipment-expansion milestone
  re-confirmed this list directly against every remaining rendered page
  image (pp.91-99) and found nothing else buildable -- see
  `docs/QUEST_NOTES.md`'s "Extending this later" for the fuller
  accounting, including the chapter's later "Special Magical Items of
  Krynn" section (Staff/Dagger of Magius, Bupu's Emerald, the Bloodstone
  of Fistandantilus, Dalamar's items, Tasslehoff's ring, the Nightjewel,
  Warbringer, and more), every one of which turned out to be a named
  artifact permanently owned by a specific canon character. What's left
  in the chapter still needs an unbuilt subsystem: creature command/charm (Golden Circlet), a
  translation flag (Glasses of Arcanist), environmental wind control
  (Flute of Wind Dancing), a plot-key/door mechanic (Keys of Quinarost),
  or moon-phase magic (Scroll of the Stellar Path) — plus Armor and
  Shields/Weapons' remaining entries are either already shipped (Solamnic
  Armor), antagonist-only (Dragonarmor), a whole quest's goal in their own
  right (Plate of Solamnus, alignment-scaled), or unique named artifacts
  (Dragonlance, Mantooth, Nightbringer, Wyrmsbane, Wyrmslayer, Shield of
  Huma), out of scope for the same reason as "Special Magical Items of
  Krynn"'s entries (see "Magic items" above). Frostreaver (p.94, Weapons),
  the one real exception the later re-read surfaced, has since shipped —
  see "Magic items" above. None of the remaining still-out-of-scope items
  are being built just to place one — see CLAUDE.md's "no premature
  abstraction."
- **Sword Knight's real healing/foresight/clerical-spell abilities and
  weekly fasting/meditation ritual** (p.18-19): not modeled, same
  "flavor-only, needs a fuller spell system" treatment already given to
  Wizard Robe spell-sphere restrictions below — this project's Sword
  Knights get the title only, not the book's limited-cleric powers. Rose
  Knights (shipped alongside Sword, see "Knights of Solamnia" above) get
  the same treatment — the book's Rose-specific perks (faster weapon/
  nonweapon proficiency advancement past what Crown/Sword already grant)
  aren't modeled either, since this project has no weapon-proficiency
  system for either order to plug into (see "Deliberate simplification"
  above).
- **Wizard Robe mechanics, for real**: Robe assignment by alignment at
  3rd level is implemented (see "Leveling / experience" above); robe-based
  spell-sphere restrictions and moon-phase (Solinari/Lunitari/Nuitari)
  bonuses still need a spell system to attach to.
- **Gully Dwarf**: needs its own ability-score generation method (e.g.
  Strength 4d4+2) instead of this project's 4d6-drop-lowest — see "Elf
  and Dwarf subraces" above for why it wasn't approximated instead.
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
