# Combat notes

## Accuracy: what's sourced, what's invented

The core resolution math is transcribed from a scanned copy of the 2e
Player's Handbook (revised), visually confirmed against rendered page
images (same discipline as every other rules pass in this project):

- **Table 1 (Strength, PHB p.19)**: to-hit and damage adjustments per STR
  score, including the 18/01–18/00 exceptional-strength sub-brackets.
  `docs/CHARACTER_NOTES.md` flagged this as deliberately skipped "until
  combat exists to use them" since Milestone 4 — it does now, so it's
  implemented, not deferred again. See `character::strengthToHitAdjustment`/
  `strengthDamageAdjustment` (`character/Ability.h/.cpp`).
- **Attack roll** (PHB p.119, p.121): 1d20 + STR to-hit adjustment; hit if
  the total ≥ (attacker's THAC0 − target's AC). A natural 20 always hits
  and a natural 1 always misses, checked against the *unmodified* die
  ("regardless of any modifiers applied to the die roll" — the book's own
  wording).
- **Initiative** (PHB p.124): one d10 per side per round, lower roll acts
  first. Ties are re-rolled rather than resolved "simultaneously" as the
  book describes — this project's round loop is strictly ordered (one
  side acts, then the other, with the second side skipped if the first
  already ended the fight), which has no way to represent two things
  happening at once. See `combat::playerActsFirst`. A warrior's side of
  that exchange can now be more than one swing (see Table 15 below) —
  those all resolve together within that side's own turn, still ordered
  strictly against the monster's single action.
- **Attacks per round** (PHB Table 15, p.36, "Warrior Melee Attacks per
  Round"): Fighter (this project's only implemented Warrior-group class —
  Paladin/Ranger don't exist here) gets 1/round at levels 1–6, 3/2 rounds
  at 7–12, 2/round at 13+; every other class stays at 1/round, per the
  book's own text scoping multiple attacks to "warriors." Modeled as of
  Milestone 108 via `character::meleeAttacksThisRound`, consumed by
  `GameLoop::runCombat`'s `playerAttacks` lambda — see
  `docs/CHARACTER_NOTES.md`'s "Leveling / experience" section. The "3/2
  rounds" rate's odd/even split (1 attack on odd rounds of the fight, 2 on
  even) is this project's interpretation, not printed verbatim — the book
  states the rate but not which rounds carry the extra swing.
- **Specialist attacks per round** (PHB Table 35, p.71, "Specialist
  Attacks Per Round," melee weapon column): a Fighter who chose Weapon
  Specialization at creation (`character::Character::specializedWeapon` —
  see `docs/CHARACTER_NOTES.md`'s "Weapon Specialization" section) gets
  3/2 at 1–6, 2/1 at 7–12, 5/2 at 13+ instead of Table 15's rates above —
  each bracket's rate arrives roughly six levels early, plus a new top
  rate at 13+. `meleeAttacksThisRound` gained a `specialized` parameter
  (no default; every call site passes it explicitly) for this. The new
  5/2 rate reuses the same odd/even-by-round-parity convention as the
  non-specialist 3/2 case (2 attacks on odd rounds, 3 on even) — again
  this project's interpretation, not printed verbatim.

**Not specifically re-verified this pass, called out honestly rather than
overclaimed**: damage is floored at 1 on a hit
(`combat::resolvePlayerAttack`/`resolveMonsterAttack`). This is a
near-universal D&D convention across editions, but this project didn't
pull up the exact PHB page stating it for 2e specifically — if a future
pass finds a different rule, this is a one-line fix.

**Monster stats are now sourced.** `Monster Manual (2nd ed).pdf` (actually
the 1993 *Monstrous Manual* compilation, confirmed via its own title page)
was added to the project's local reference library specifically for this,
and `data/monsters.txt` was rebuilt from it — visually confirmed against
rendered page images, same discipline as everything else. This project
models Krynn specifically, which has **no orcs** (a deliberate,
long-standing Dragonlance lore point, not an oversight) — the roster below
was chosen and checked with that constraint in mind.

- **Goblin** (p.163): HD 1-1, AC 6, THAC0 20, 1d6 damage.
- **Kobold** (p.214): HD 1/2 (1-4 hp), AC 7, THAC0 20, 1d4 damage.
- **Hobgoblin** (p.191): HD 1+1, AC 5, THAC0 19. Damage is "by weapon" in
  the book (no fixed die); 1d8 was picked as a reasonable stand-in given
  their typical loadout (polearm/morningstar/sword).
- **Timber Wolf** ("Wolf," p.362): HD 3, AC 7, THAC0 18, damage 2-5 (1d4+1).
- **Giant Spider** (p.326): HD 3+3, AC 4, THAC0 17, bite 1d8, **Type F
  poison** — "causes immediate death if the victim fails the saving
  throw," no save-roll modifier printed for the standard Giant Spider
  (visually confirmed, page text search). Now modeled — see "Saving
  throws in combat" below for the mechanic and how literal "death" is
  reconciled with this project's no-permadeath rule.
- **Baaz Draconian** (*Dragonlance Adventures*, TSR 2021, p.75 — the
  Monstrous Manual has no draconians, they're Dragonlance-specific): HD 2,
  AC 4, 20% magic resistance (not modeled, same reasoning as the spider's
  poison), two claws (1d4/1d4) or one weapon attack — simplified to a
  single 1d8 weapon hit, since this project doesn't model multiple attacks
  per round for anyone. **THAC0 isn't printed in the Dragonlance book at
  all** (it only gives HD) — derived as 19 from the Monstrous Manual's own
  HD-to-THAC0 pattern (empirically consistent across every entry checked;
  the book bakes the conversion into each stat block rather than printing
  one shared table). Its "turns to stone on death" trait — the single most
  iconic thing about Baaz Draconians — gets a special victory message in
  `GameLoop::runCombat` rather than being just flavor text.
- **Bugbear** (p.32): HD 3+1, AC 5, THAC0 17, damage 2d4 (2-8). The book's
  "+3 to all melee damage" (giant Strength bonus) and "or by weapon"
  branch aren't modeled — same monster-side-weapon-bonus simplification
  already applied to Hobgoblin/Baaz.
- **Ogre** (p.272): HD 4+1, AC 5, THAC0 17, damage "1-10 or by weapon +6"
  in the book — simplified to a flat 1d10, same reasoning as Bugbear
  above.
- **Kapak Draconian** (*Dragonlance Adventures*, TSR 2021, p.74-75 — like
  the Baaz, not in the Monstrous Manual): HD 3, AC 4, damage 1d4. **THAC0
  isn't printed** (same gap as the Baaz) — derived as 18 from the same
  HD-to-THAC0 pattern, confirmed against the Timber Wolf's real printed
  HD 3 → THAC0 18. The Kapak's real venomous bite causes **paralysis**
  (2d6 turns on a failed save vs. poison) — a mechanically different
  effect from the Giant Spider's Type F death-poison this project's
  `poisonOnHit` flag models, so reusing that flag would misrepresent it.
  Left unmodeled (flavor-only), same restraint as the Baaz's unmodeled
  20% magic resistance. Its acid-pool-on-death trait is also not modeled
  (no environmental-hazard system for anyone yet).
- **Gnoll** (p.158): HD 2, AC 5, THAC0 19, damage 2-8 (2d4) — "by weapon"
  in the book, same monster-side-weapon simplification as Hobgoblin/
  Bugbear/Ogre. XP 35.
- **Ghoul** (p.131): HD 2, AC 6, THAC0 19, XP 175. Real attack is three
  hits (claw/claw/bite, 1-3/1-3/1-4-or-6 — the last die was genuinely
  ambiguous across both extractions of the scan, reading as either 1-4
  or 1-6 depending on which of the page's parallel monster columns is
  checked) — simplified to a single 1d6 hit, same "one representative
  die" treatment as Baaz's two claws. Real Ghouls also paralyze on a hit
  (save vs. paralyzation or be unable to act) — left unmodeled, same
  restraint as the Kapak's paralysis-poison bite above (no status-effect
  system exists for anyone yet).
- **Skeleton** (p.315, the base "Skeleton" column on that page — not the
  Animal or Monster skeleton variants sharing it): HD 1, AC 7, THAC0 19,
  damage 1-6 (weapon), XP 65. Real Skeletons take half damage from
  edged/piercing weapons and are immune to sleep/charm/hold/cold/fear —
  no per-monster damage-type or immunity mechanic exists in this project
  (same restraint as Baaz's unmodeled magic resistance), so it's flavor
  text in `DESC` only.
- **Zombie** (p.373, the base "Common" column — not the Monster/Ju-ju/
  Lord/Sea variants sharing it): HD 2, AC 8, THAC0 19, damage 1-8, XP 65.
  Real Zombies are immune to sleep/charm/hold/death-magic/poison/cold,
  same flavor-only treatment as the Skeleton's immunities above.
- **Bozak Draconian** (*Dragonlance Adventures*, TSR 2021, p.74): HD 4,
  AC 2, two claws (1d4/1d4) or by weapon — simplified to a single 1d8
  weapon hit, identical wording and treatment to the Baaz's own stat line.
  **THAC0 isn't printed** (same gap as Baaz/Kapak) — derived as 17 from
  this project's established HD-to-THAC0 pattern, the same bracket as
  Ogre's real printed HD 4+1 → THAC0 17. Real spellcasting (as a
  4th-level magic-user: burning hands, enlarge, magic missile, shocking
  grasp, invisibility, levitate, stinking cloud, web) — Milestone 99 models
  the signature spell, Magic Missile, as a real special attack: 40% chance
  per round (invented pacing, the book gives no frequency) to cast it
  instead of its weapon attack, dealing the same PHB p.176 math as the
  player's own `magic_missile` spell (`character::castSpell`) fixed at
  "4th-level caster" — 1d4+1 per missile, 2 missiles, no attack roll, no
  saving throw (`combat::Monster::castsMagicMissile`/
  `magicMissileChancePercent`, applied in `game::GameLoop::runCombat`'s
  `monsterAttacks` lambda before the normal `resolveMonsterAttack` call).
  The other seven favored spells, +2 saves, and 20% magic resistance stay
  unmodeled, same restraint as Baaz's own unmodeled magic resistance. Its
  bone-explosion death trait is likewise unmodeled, same treatment as
  Kapak's acid pool.
- **Sivak Draconian** (*Dragonlance Adventures*, TSR 2021, p.75): HD 6,
  AC 1, two swords (1d6/1d6) plus an armored tail (2d6) — simplified to a
  single 2d6 hit (its highest, most distinctive die), same "one
  representative die" treatment as the Ghoul's claw/claw/bite. **THAC0
  isn't printed** — derived as 15, one HD-to-THAC0 bracket further down
  from the Bozak above. Real shapeshifting (takes the form of a humanoid it
  kills, or its own killer's form on death) stays unmodeled — no persistent
  per-monster identity or NPC-disguise gameplay exists to hang it on (this
  project's `combat::Monster` is static content shared by every encounter
  with that species, not a per-instance actor), same restraint as ever.
  +2 saves and 20% magic resistance are unmodeled too. Its death-burst
  ("killed by something larger than itself... burst into flames... 2d4
  points of damage... no saving throw") is real as of Milestone 99, though
  simplified: this project has no SIZE stat to check the book's "larger
  than itself" condition against, so it always fires
  (`combat::Monster::burstsIntoFlameOnDeath`, applied in
  `game::GameLoop::runCombat`'s post-victory block right after XP/steel are
  awarded) — a genuine retaliatory hit, unlike Baaz's stone/Kapak's
  acid/Bozak's bone-explosion, which all stay flavor-only victory
  messages. Can knock the player out even after they already landed the
  killing blow — see "Death: knocked out, not killed" below for how
  that's handled.
- **Aurak Draconian** (*Dragonlance Adventures*, TSR 2021, p.73): HD 8,
  AC 0, twin energy blasts (1d8+2 each) or a spell — simplified to a
  single 1d8+2 hit. **THAC0 isn't printed** — derived as 13, the same
  pattern extended to the highest HD this project has extrapolated it to
  (see the caveat below). The most mechanically loaded monster in the
  roster: limited dimension door, suggestion/mind control, change self,
  polymorph self, at-will invisibility, and real 1st- to 4th-level
  magic-user spellcasting all stay unmodeled — none of them translate
  cleanly into this project's positionless, no-disguise-gameplay 1-vs-1
  combat loop, same restraint applied to Sivak's shapeshifting above. Its
  real three-stage death (immolation → lightning ball → explosion), 30%
  magic resistance, and +4 saves are likewise unmodeled. Its noxious-cloud
  breath weapon is real as of Milestone 99: 30% chance per round (invented
  pacing — the book's real "three times per day" has no way to track
  across stateless encounters with no monster-instance persistence, so
  it's compressed to "available this whole fight," gated only by the
  per-round roll) to breathe instead of its weapon attack, rolling
  `combat::rollSavingThrow` against the (previously dormant in live combat)
  `character::SaveCategory::BreathWeapon` — save for half of the book's 20
  damage, or full damage plus blinded (a -4 this-fight to-hit penalty,
  applied the same way as Frostreaver's/spell buffs' this-fight-only
  bonuses; the book names the condition but not a number, so this value is
  invented) (`combat::Monster::hasBreathWeapon`/
  `breathWeaponChancePercent`, same lambda as Bozak's Magic Missile above).
- **Thanoi** (*Dragonlance Adventures*, TSR 2021, p.78, "Thanoi (Walrus
  Men)"): HD 4, AC 4, damage by weapon or tusk (1d8), plus a separately
  printed "any weapon used by a thanoi does 2 more points of damage than
  usual" — both numbers directly sourced, combined as 1d8+2. THAC0 derived
  as 17, the same HD-4 bracket as the Bozak above. XP 85 + 4/hp. The
  walrus-men of Icewall Glacier — already referenced as flavor-only
  dialogue at Ice Wall since Milestone 36, now a real roster entry.
  Originally shipped (Milestone 64) with `TERRAIN_BIAS` toward glacier
  (`:`, 3x weight there but still eligible everywhere else); tightened at
  Milestone 83, at the user's explicit request, to a hard `ONLY_TERRAIN :`
  lock, since a walrus-man turning up in ordinary grassland read wrong.
  **This is an invented gameplay restriction, not a sourced one** — unlike
  Gnoll's real Monstrous Manual Climate/Terrain field, *Dragonlance
  Adventures* prints no Climate/Terrain field for the Thanoi at all, so
  there's no book text to hang a hard restriction on; it's honestly the
  user's own call, not a transcription. Real cold immunity (natural and
  magical), extra fire/heat damage, and HD loss in warm climates are all
  unmodeled — no damage-type or elemental-exposure system exists for
  anyone yet, same flavor-only restraint as Skeleton's/Zombie's immunities.
- **Owlbear** (Milestone 100, p.284): HD 5+2, AC 5, THAC0 15, XP 420. Real
  attack is three hits (claw/claw/beak, 1d6/1d6/2d6) — simplified to a
  single 2d6 hit (the beak, its most distinctive and damaging attack), same
  "one representative die" treatment as the Ghoul's/Sivak's own multi-attack
  simplifications above. Its real "hug" special attack (an 18+ claw hit
  drags the victim in for 2d8 ongoing squeeze damage per round) is left
  unmodeled — no grapple/ongoing-effect state exists for anyone yet.
- **Wight** (Milestone 100, p.360): HD 4+3, AC 5, THAC0 15, damage 1d4, XP a
  flat 1,400 (its outsized level-drain value, not a per-hp formula). Real
  Special Attacks (a level-draining touch) and Special Defenses (hit only by
  silver or +1-or-better magical weapons) are both left unmodeled — no
  level-drain or weapon-enchantment-gate mechanic exists for anyone yet,
  same restraint as the Ghoul's paralyzing touch and the Draconians'
  unmodeled magic resistance.
- **Troll** (Milestone 100, p.349, the base "Troll" column only — not the
  Two-headed/Freshwater/Saltwater/Desert/Spectral/Giant/Ice variants sharing
  the same page): HD 6+6, AC 4, THAC0 13, XP a flat 1,400 (same as Wight, no
  formula to simplify). Real attack is three hits (claw/claw/bite,
  1d4+4/1d4+4/1d8+4) — simplified to a single 1d8+4 hit (the bite), same
  treatment as the Owlbear's beak above. Real Special Defenses
  (regeneration — 3 hp/round starting three rounds after first blood,
  stopped only by fire or acid) is left unmodeled — no per-round monster HP
  recovery exists in `runCombat`.
- **Black Bear** (Milestone 107, p.17, the "Bear" comparison table): HD 3+3,
  AC 7, THAC0 17, XP 175 (all real, printed). Real attack is three hits
  (claw/claw/bite, 1-3/1-3/1-6) — simplified to a single 1d6 hit (the bite,
  its most damaging attack), same "one representative die" treatment as the
  Ghoul's/Owlbear's/Troll's own multi-attack simplifications. Treasure: Nil
  (printed) — no steel reward, same as the Wolf. Real Climate/Terrain is the
  broad "Temperate land" — weighted toward forest/hills as an invented
  flavor bias, same "informed by, not transcribed from" treatment as the
  Bugbear's/Owlbear's own bias lines.
- **Worg** (Milestone 107, p.362, the "Wolf" comparison table): HD 3+3,
  AC 6, THAC0 17, damage 2d4 (2-8), XP 120 (all real, printed). Real
  Climate/Terrain is "Any forest" — an offshoot of dire wolf stock that
  "often serve as mounts of goblins," a direct thematic tie to the
  Goblin/Hobgoblin already in this roster. Treasure: Nil (printed) — no
  steel reward, same as the Wolf.
- **Ice Bear** (Milestone 107, *Dragonlance Adventures*, TSR 2021, p.76,
  the "Creatures of Krynn" chapter — same Krynn-specific source as the
  Draconians and Thanoi): HD 6+2, AC 6, XP a per-hp formula ("475 + 8/hp",
  real, printed) simplified to a flat 707 (475 + 8×29, using the average
  roll of 6d8+2), same treatment as the Draconians' XP. Real attack is
  three hits (claw/claw/bite, 1d8/1d8/2d8) — simplified to a single 2d8 hit
  (the bite), same treatment as the Black Bear above; its real "hugs for
  2d6 if both claws hit" is left unmodeled, same restraint as the Owlbear's
  own hug. Immune to cold is real but left flavor-only, same restraint as
  the Thanoi's own cold immunity. **THAC0 isn't printed** (same gap as
  every other DLA-sourced monster) — derived as 14 from this project's
  established HD-to-THAC0 pattern: Owlbear's real HD5+2 → THAC0 15 sits one
  bracket below plain HD6's own real value (Sivak, HD6 → THAC0 15), so a
  "+2" bonus behaves like roughly one extra full Hit Die in this
  progression — applying that same step to HD6+2 lands one bracket below
  Sivak's HD6, at 14. **No Climate/Terrain field is printed** (same gap as
  the Thanoi) — `ONLY_TERRAIN` glacier is an invented restriction, not a
  sourced field, justified by the prose ("track prey over snow and ice...
  the thanoi use them for this purpose, sharing the reward") — the same
  restriction the Thanoi already carries, and a direct in-book lore tie
  between the two.

**Sourcing caveat (Milestone 64)**: none of the four draconian/Thanoi
entries above print THAC0 (Dragonlance Adventures' stat-block format never
does), so all four are derived via the same "roughly one point of THAC0
per Hit Die" empirical pattern already used and validated for Baaz/Kapak.
That pattern was previously validated only up to HD 4+1 (Ogre's real
printed THAC0 17); Sivak (HD 6 → 15) and Aurak (HD 8 → 13) extend it past
that ceiling. All four new monsters have plain, non-bonus Hit Dice, so
each lands cleanly on a bracket boundary rather than a "+" tier, which is
the least ambiguous case for this kind of extrapolation — but it's still
an extrapolation, not a directly re-confirmed value, and is flagged as
such here.

**Sourcing caveat for this batch (Milestone 34)**: no PDF-page-image
renderer was available in that session (no `pdftoppm`/ImageMagick/Python
on the machine), so these four weren't visually confirmed against a
rendered page image the way every earlier monster was. Instead, each
stat block was extracted as text twice, independently
(`pdftotext -table` and `pdftotext -raw`, two different column-alignment
heuristics against the same scanned PDF), and only values both
extractions agreed on were used — cross-checked further against this
project's own established HD-to-THAC0 pattern (roughly one point of
THAC0 per Hit Die, the same empirical relationship already used to
derive the Baaz's and Kapak's un-printed THAC0 above; all four new
monsters' printed THAC0 fits comfortably). Still real text from the
actual scanned book, not memory — just missing the usual
visual-confirmation step. Worth re-confirming against a rendered page
image if that tooling becomes available later.

**Citation correction (Milestone 57)**: while re-sourcing terrain data for
this roster (see "Terrain-specific monster pools" below), 9 of the 11 page
numbers above turned out to be off by exactly +3 — they cited the PDF
viewer's own internal page count rather than the book's printed folio
(visible at the bottom of each page). Re-confirmed against rendered page
images showing the actual printed footer number and corrected in place
above; Bugbear's and Ogre's citations were already correct.

None of these have HP dice matching their HD 1:1 in a way this project can
represent with a plain `NdM+flat` -- 2e's default monster Hit Die is d8,
which is what `data/monsters.txt` uses throughout (e.g. HD 1-1 → `1d8-1`,
HD 3+3 → `3d8+3`).

## Weapon damage and Armor Class: real equipment now, not just a placeholder

Each class still starts with the same fixed weapon as before (Fighter
longsword 1d8, Cleric mace 1d6, Thief shortsword 1d6, Mage dagger 1d4,
Tinker "a well-worn wrench" 1d4, all in `character::ClassInfo`) — but as
of the equipment system (`docs/CHARACTER_NOTES.md`'s "Equipment"
section), that's now only the *starting* weapon, seeded onto
`Character::weaponName`/`weaponDamageSides` at creation. Buying a weapon
upgrade or armor at Solace's General Store (`character::Equipment`)
changes the character's own fields, and `combat::resolvePlayerAttack`
reads `character.weaponDamageSides`/`weaponDamageBonus` directly —
`ClassInfo`'s weapon fields are never consulted mid-game, only at
creation. `Character::armorClass` is likewise no longer always
`10 - Dex adjustment`: `Equipment::recomputeArmorClass` factors in
whatever armor/shield is equipped, called both at creation (a no-op
change when nothing's equipped yet) and after every purchase.

## Saving throws in combat

`combat::rollSavingThrow(character, category)` (`Combat.h/.cpp`) is a
thin, reusable wrapper: one d20 roll, succeeds if it's >= the character's
saving throw number for that category — same "at or above" convention
`character::SavingThrows` already documents. It's general (any future
monster/effect can call it against any of the five save categories), but
only one thing calls it today:

**The Giant Spider's poison bite.** `combat::Monster` gained a single
`bool poisonOnHit` flag (not a general Type A–N poison-type system — this
project only has one poison-bearing creature, so a flag is honest scope
rather than building unused generality), set via a new `POISON` line in
`data/monsters.txt`'s `MONSTER` block grammar. In
`GameLoop::runCombat`'s `monsterAttacks` lambda, a hit from a
`poisonOnHit` monster additionally rolls
`rollSavingThrow(character, SaveCategory::ParalyzationPoisonDeath)` on
top of the physical bite damage already applied: success just logs
"You resist the poison" (matching the book — even a successful save
against Type F poison has no lingering effect beyond the bite itself);
failure logs "The poison overwhelms you!" and sets `currentHp` to 0.

**Reconciling the book's "instant death" with this project's own rule**:
Type F poison's real effect on a failed save is literal death, but this
project made an explicit, user-confirmed decision (Milestone 9) to never
permadeath the player regardless of how they reach 0 HP. Setting
`currentHp = 0` inside `monsterAttacks` deliberately reuses the *existing*
post-round `if (state_.character.currentHp <= 0)` block — the same
knockout-and-return-to-Solace code path a normal lethal hit already
takes — rather than adding a second, separate "you died" branch. A failed
poison save is exactly as survivable as any other 0-HP moment, no worse.

**Not covered by this pass**: the Baaz Draconian's 20% magic resistance
is a different mechanic entirely (a resistance-to-being-targeted roll,
not a saving throw) and is still not modeled — see "Extending this
later" below.

## Combat is not a `GameState.mode`, and isn't saved

Unlike `Mode::Overworld`/`Mode::Zone`, an encounter is a **nested loop**
inside `game::GameLoop` (`runCombat`), not a third `Mode` value. It takes
over rendering/input in its own `for(;;)` loop — same shape as
`showCharacterSheet`'s existing one-keypress block, just longer-lived —
until the fight resolves, then returns control to the ordinary loop.

This means combat state (the monster's current HP, the log) is **not**
part of `GameState` and is never written to `save.txt`. A crash or
force-close mid-fight simply loses that encounter — the player resumes on
the overworld tile where it started, monster forgotten. This is a
deliberate trade-off: modeling combat as its own `Mode` and threading it
through `SaveGame` would have meant serializing monster identity + current
HP + a log, for a feature whose whole failure mode (the user's own choice
— see below) is already low-stakes.

## Death: knocked out, not killed

**User decision**, explicitly chosen over permadeath (which would have
fit the Caves-of-Qud inspiration this project was originally built
around): when the player's HP reaches 0, it's set to `maxHp` (a full heal,
not just enough to stand) and they wake up at the **nearest refuge**
(`GameLoop::nearestRefuge()`, straight-line tile distance from
`world::Location::isTown`/`seaLocked` entries -- see below) rather than
the run ending. There's no "you have died" screen, no save deletion,
nothing punitive beyond the trip back to safety.

**Which locations count as a "refuge"**: `world::Location` has a `bool
isTown` flag, set via an optional `TOWN` line in a `data/locations.txt`
block (see `docs/MAP_NOTES.md`). Five locations carry it -- Solace,
Haven, Kalaman, Tarsis, Palanthas -- the ones already tagged with a
civilian-settlement `TERRAIN` (forest-town/plains-town/coastal-town/dry-
plains-city/walled-port-city). Fortresses (Pax Tharkas, High Clerist's
Tower), ruins (Xak Tsaroth, Ice Wall), the nomadic Plains of Dust
village, and the elven homelands (Qualinesti/Silvanesti -- Silvanesti is
sealed to outsiders, a deliberate lore exclusion, not an oversight) are
not towns for this purpose.

**`seaLocked` (Milestone 90)**: a second, independent flag, also set via
an optional argument-less `data/locations.txt` line (`SEA_LOCKED`),
carried only by Ice Wall Castle. Added to close a real softlock the user
hit: the `frostreaver_salvage` quest requires killing Thanoi, which are
hard-locked to the glacier terrain immediately around Ice Wall Castle
(Milestone 83), and the *only* way to reach Ice Wall Castle at all is
Tarsis's Knight's Runner one-time `BOAT` voyage (Milestone 88, kept
deliberately one-time -- see "Sea travel" in `docs/ARCHITECTURE.md`). A
knockout on that glacier previously sent the player to the nearest actual
`isTown` location (Tarsis, ~56 tiles away), stranding them for good --
the boat back doesn't fire twice. `seaLocked` locations aren't civilian
settlements, but they're still a valid, safe place to wake up -- treating
them as "not a refuge" is exactly the bug. `nearestRefuge()` picks
whichever `isTown`- or `seaLocked`-flagged location is closest by
straight-line tile distance to where the player fell -- no pathfinding
system exists in this project, same restraint already applied to
`minutesToCross` being flat-per-tile -- falling back to Solace only if
nothing is found at all (defensive; can't happen with the current data).

**Two call sites (Milestone 99)**: the Sivak Draconian's real death-burst
(see the Draconian roster above) can finish the player off *after* they
already landed the killing blow -- an ordinary monster attack dropping the
player to 0 is no longer the only way into this ending. `GameLoop::
runCombat` factors the ending itself into a local `knockedOutBy(cause)`
lambda, called both from the original mid-fight check and from the new
post-victory burst check; the player still keeps the kill's XP/steel/tally
either way, since those are applied before the burst roll.

## Encounters: a per-terrain chance while traveling

`GameLoop::tryMoveOverworld`, after a successful move: if the destination
tile has no `Location` (named places/towns stay safe), a chance rolls a
random monster from `combat::MonsterCatalog` and starts `runCombat`. As of
Milestone 27 the chance varies by terrain — `world::TerrainInfo` gained an
`encounterChancePercent` field alongside the existing `minutesToCross`,
filled in per terrain in `Terrain.cpp`'s `kTable` (roads safest at 2%,
mountains/forest riskiest at 12%/11%; ocean/Blood Sea/uncharted are 0,
though ocean/Blood Sea were impassable anyway when this was written so it
never got checked). **All water terrain is 0% as of Milestone 84** —
shallow water (`r`) still had a nonzero 5% until then, an oversight from
before `GameState::hasBoat` (Milestone 36) existed: per
`docs/MAP_NOTES.md`, `r` is essentially always coastal water in the
generated grid (real river fords never survived downsampling), so any
boat route along a coastline crossed a long run of `r` tiles, each
independently rolling that 5% — reported directly by the user as "a lot
of monster battles in ocean squares." Zeroed to match ocean/Blood Sea's
already-established reasoning: no sea monsters exist in the roster, so a
nonzero chance here can only draw a land monster into the water. Like
`minutesToCross`, these numbers are tuned for pacing, not sourced from
anything. Monster *selection* is terrain-weighted as of Milestone 57 — see
"Terrain-specific monster pools" below.

## Terrain-specific monster pools

`combat::MonsterCatalog::randomMonster(char terrainCode, int
distanceToNearestTown)` (Milestone 57, gained its second parameter at
Milestone 83 — see "Town-proximity monster pools" below) weights which
monster gets picked by the terrain that triggered the encounter, instead
of the flat uniform-random pick every earlier milestone used. Two
separate, honestly-labeled data sources feed the terrain half of this,
both as optional lines in a `data/monsters.txt` `MONSTER` block (grammar
documented in that file's own header comment):

- **`EXCLUDE_TERRAIN <codes>`** — a real, sourced Climate/Terrain hard
  restriction. Re-checking every roster monster's actual Monstrous Manual
  "Climate/Terrain" field (via rendered page images, not just OCR text,
  which proved unreliable for this book's multi-column shared stat tables)
  found it far more generic than expected: Goblin, Kobold, Hobgoblin, Giant
  Spider, and Ogre are all simply "Any (non-arctic) land"; Ghoul, Skeleton,
  and Zombie are "Any," full stop. **Giant Spider is explicitly not
  forest-locked in the book** — the "spiders in forest" idea this section
  used to speculate about doesn't hold up under the real text. Only one
  monster has an exclusion worth encoding: Gnoll's real terrain is "Any
  tropical to temperate, non-desert," so it carries `EXCLUDE_TERRAIN _`
  (salt flat, this project's closest terrain analog to desert). Bugbear's
  real terrain ("Any subterranean") and Timber Wolf's ("Non-tropical") don't
  map cleanly onto this project's specific overworld terrain codes as a hard
  exclusion, so they're expressed as bias instead (below). Baaz/Kapak/
  Bozak/Sivak/Aurak Draconian (Dragonlance Adventures, not the Monstrous
  Manual) have no Climate/Terrain field at all — the book frames them as
  garrison troops/infiltrators/special agents, a faction/location thing
  this terrain-code system can't represent honestly, so they carry
  neither line and stay uniform.
  **Glacier gap closed (Milestone 64)**: this section used to note that
  "nothing in the roster is Arctic-flavored," so glacier fell back to the
  full uniform pool. The Thanoi (`data/monsters.txt`) closes that gap --
  Icewall Glacier's own walrus-men, sourced from Dragonlance Adventures
  p.78. Originally carried `TERRAIN_BIAS :` (bias, not a hard lock — the
  rest of the roster could still turn up on glacier, same as every other
  biased terrain), the same "not invented beyond what the book supports"
  restraint as everywhere else in this section, since DLA gives Thanoi no
  formal Climate/Terrain field either.
  **Tightened to `ONLY_TERRAIN :` at Milestone 83**, at the user's
  explicit request: unlike every other line in this section, this one is
  *not* claimed as book-supported restraint — it's an invented, purely
  gameplay-driven hard lock (see `ONLY_TERRAIN` below and "Town-proximity
  monster pools"), because a walrus-man turning up outside its sourced
  glacier habitat read wrong in play.
- **`TERRAIN_BIAS <codes>`** — invented flavor weighting (a biased monster
  is 3x as likely to be picked on that terrain as an unbiased one,
  `combat::kBiasWeight` in `Monster.cpp`), informed by each monster's real
  Habitat/Society prose but explicitly *not* claimed as sourced — the same
  "tuned for pacing, not sourced" honesty `encounterChancePercent` above
  already gets. Goblin/Kobold lean hills/forest (caves, ruins, mining
  terrain); Timber Wolf leans forest/grassland; Giant Spider leans
  forest/bog (web-spinner ambush terrain); Bugbear leans hills/mountains (a
  proxy for its real "any subterranean"); Gnoll leans forest/hills/bog;
  Thanoi leaned glacier this way until Milestone 83 (see below). Hobgoblin,
  Ogre, Baaz, Kapak, Bozak, Sivak, Aurak, Ghoul, Skeleton, and Zombie are
  left deliberately uniform (terrain-wise) — Ogre's own book text says
  "found anywhere, from deep caverns to mountaintops," the three undead
  have no ecological terrain link, and the five draconians' real
  differentiator (faction/location) isn't one this system can express
  without forcing it.
- **`ONLY_TERRAIN <codes>`** (Milestone 83) — the inverse of
  `EXCLUDE_TERRAIN`: a hard restriction to *only* the listed codes, not
  weighting. Unlike every other line in this list, this one is not framed
  as book-sourced restraint — it exists purely as an invented gameplay
  restriction (only Thanoi carries it, locked to glacier `:`; see "Glacier
  gap closed" above and "Town-proximity monster pools" below).

## Town-proximity monster pools

Milestone 83, prompted directly by the user hitting it in play: high-HD
Ogres and Draconians could appear one step outside Solace, a brand-new
character's starting town, because the terrain-only system above has no
notion of "near civilization" at all — Ogre and all five Draconians carry
neither `EXCLUDE_TERRAIN` nor `TERRAIN_BIAS`, so they were uniformly
eligible on every passable tile on the entire 480x320 map.

A new optional `data/monsters.txt` line, `MIN_TOWN_DISTANCE <n>`: the
monster is never eligible unless the encounter tile is at least `n` tiles
(straight-line, `combat::Monster::minTownDistance`) from the nearest
civilian town (`world::Location::isTown` — Solace, Haven, Kalaman, Tarsis,
Palanthas). `GameLoop::tryMoveOverworld` computes this distance inline
right before rolling an encounter (state_.x/y are already updated to the
destination tile at that point) and passes it into
`MonsterCatalog::randomMonster`'s new second parameter, which folds it
into the same eligibility filter as `EXCLUDE_TERRAIN`/`ONLY_TERRAIN`
(same "fall back to the full roster if everything gets excluded"
defensive behavior as before). Invented gameplay tuning, not sourced —
same honesty as `encounterChancePercent`/`kBiasWeight` above.

Applied to the tier the user called out as "too many high-powered
Draconians/Ogres near Solace," tuned to a rough danger-scaled distance
curve: **Ogre** and **Kapak Draconian** (`MIN_TOWN_DISTANCE 20`) — Kapak
is only HD3, weaker on paper than Ogre's HD4+1, but its real
paralysis-poison bite is disproportionately punishing for a low-level
character, so the user asked for it grouped with the higher tier rather
than left with Baaz; **Bozak Draconian** (`25`); **Sivak Draconian**
(`35`); **Ettin** (`40`, added at Milestone 112 — HD10 is the highest in
the roster, and its two guaranteed club hits out-damage Aurak's own
average round in raw combat math, even though Aurak stays the most
*magically* loaded thing in the roster, unmodeled abilities included);
**Aurak Draconian** (`45`, kept farthest out). Left deliberately
untouched — no `MIN_TOWN_DISTANCE`, same as ever: Goblin, Kobold,
Hobgoblin, Timber Wolf, Giant Spider, Baaz Draconian, Bugbear, Gnoll,
Ghoul, Skeleton, Zombie, Lizard Man, Giant Toad — the HD1-3 "line
troop"/wildlife tier. "Lower hit die monsters around Solace and other
cities" (the user's own framing) falls out naturally from this exclusion
list rather than needing a second, positive-bias mechanism: once the HD4+
threats and Kapak are gated out near town, the remaining uniform pool
*is* the low/mid-HD tier.

**Milestone 156 extended this scale past Aurak's 45 for the first time.**
Calibrated against modeled HD/damage output, not the raw XP label, since
several of the new monsters' scariest real traits (fear, disease, energy
drain, silver-or-better-to-hit) are left unmodeled the same way Wight's
level drain already is — a monster whose real terror is mostly flavor
text shouldn't be gated as if that terror were live in combat. **Wereboar**
(`20`, same HD5+2 bracket as Owlbear); **Hydra**/**Wraith** (`30`, one
step above Owlbear/Wight — Hydra's real multi-head regrowth is simplified
away entirely, so only its base HD5 remains); **Weretiger** (`35`, same
HD6-ish bracket as Sivak/Troll/Harpy/Griffon); **Gorgon**/**White
Pudding**/**Mummy**/**Spectre** (`40`, the same "big HD or big real XP,
but no spellcasting once its specials are left unmodeled" bracket as
Ettin); **Brown Pudding** (`45`, tied with Aurak — HD11, the single
highest Hit Dice added this batch); **Shambling Mound** (`55`, a
genuinely new ceiling — its real XP, 6,000, is more than double anything
else in this roster even though its own two identical grip attacks are
simplified to one representative die same as every other multi-attack
monster here, so this gate honors that real XP gap rather than
understating it). See Milestone 156 in `docs/MILESTONES.md` for the full
per-monster reasoning.

**Known limitation, not addressed by this pass**: this only keeps
dangerous monsters away from *civilian* population centers. It does not
attempt to place Draconians preferentially near actual war-front
locations (High Clerist's Tower, Neraka, Pax Tharkas, Kalaman) — that
would need a location/faction-aware placement system, real future-engine
territory, not a data-only tuning pass. Fortresses that happen to sit
close to a `TOWN` location (e.g. the High Clerist's Tower is ~16 tiles
from Palanthas) get the same civilian safety-bubble effect as everywhere
else near a town; this is an accepted simplification, not a bug.

## Player actions: Attack, Move, Cast, Use, and Flee

`render::Key::Flee` (`'f'`/`'F'`) ends the encounter immediately with a
retreat message, no cost or risk modeled (no PHB-style "opportunity attack
while fleeing" — a deliberate simplification, distinct from the real,
sourced opportunity attack Milestone 114 *did* add for ordinary
retreating movement — see "Positional combat grid" below). `Enter`
(reused from its existing "step in/interact" meaning elsewhere) resolves
one full round as a weapon attack; `w`/`a`/`s`/`d` move instead (Milestone
114).

**`render::Key::Cast` (`'m'`/`'M'`)**, alongside the leveling-system
spellcasting work (`character::Spellcasting`, see
`docs/CHARACTER_NOTES.md`, a real multi-level spellbook, not one fixed
spell): a Mage or Cleric can spend their round casting instead of
swinging their weapon. Pressing it validates `character::canCastSpells`
and `hasMemorizedSpellsAvailable` first — on failure (wrong class, nothing
currently memorized, or a racially-blocked Mage whose slot count is always
0) it logs a message and does **not** consume the round, same forgiving UX
as any other out-of-context key press. With exactly one distinct memorized
spell left it casts directly (the original one-spell UX, unchanged); with
more than one, an in-frame chooser ("Cast
which spell?" — see "In-frame combat actions" below) asks which before
spending the round. On success, `GameLoop::runCombat` swaps a
`playerCasts(spellId)` lambda in for `playerAttacks()` inside the exact
same `playerActsFirst()`-ordered exchange, dispatching on
`character::SpellEffect` (damage, heal, block the monster's attacks, a
this-fight THAC0/AC/damage buff or debuff, or an outright instant defeat —
see `docs/CHARACTER_NOTES.md`'s spell census for which spell does which)
— the monster still gets its own attack afterward per the usual
initiative ordering except when blocked. This-fight buffs/debuffs thread
through as new optional parameters on `resolvePlayerAttack`/
`resolveMonsterAttack` (`thac0Bonus`/`damageBonus` for the player,
`acBonus`/`thac0Penalty`/`damagePenalty` for the monster), held as local
variables in `runCombat` and never written into the character's real
saved `armorClass`/`thac0`. Thief, Fighter, and Tinker never see `CAST` in
the command row at all (`drawCombatFrame` only shows it for
`canCastSpells` classes with something memorized).

The same `playerThac0Bonus`/`playerDamageBonus` locals also carry
**Frostreaver's terrain-gated +4** (`docs/CHARACTER_NOTES.md`'s "Magic
items"): seeded once at the top of `runCombat`, not by a spell effect, if
the character's equipped weapon is the Frostreaver and the fight's tile
(`state_.x`/`state_.y`, always the overworld tile just stepped onto —
`runCombat`'s only call site is `tryMoveOverworld`) is glacier terrain.
Mechanically identical to a standing buff spell that happens to already be
"cast" before the fight starts, rather than a permanent weapon stat —
picked specifically to avoid a new `resolvePlayerAttack` parameter or any
change to `combat::AttackOutcome`/`Combat.h`.

**`render::Key::Inventory` (`'i'`), reinterpreted locally as "use an
item"**: a real chooser over every carried/owned consumable usable this
round (Potion of Healing, Webnet, Brooch of Imog, Staff of Striking's cure
function — see `docs/CHARACTER_NOTES.md`'s "Magic items"), built by
`character::availableCombatItems`. Auto-selects with no chooser when
exactly one is usable (the common early-game case, still identical to the
original Milestone 42 "drink the one potion" UX); with more than one, an
in-frame chooser ("Use which item?" — see "In-frame combat actions"
below) asks which before spending the round. Nothing usable logs a
message and doesn't consume the round, same forgiving pattern as an
unavailable `Cast`. This is the same "local key reinterpretation instead
of a new `Key` value" trick `handleShop` already uses for this exact key
(there it toggles the buy/sell view) — outside combat `'i'` still opens
the real inventory screen (`GameLoop::handleInventory`); only inside
`runCombat`'s own nested loop does it mean "use." Before Milestone 115
this was instead a hand-coded fixed priority (Potion, then Webnet, then
Brooch, then Staff) that silently stopped at the first match — a
character carrying both a Potion and a Webnet could never actually reach
the Webnet through `'i'` at all; `availableCombatItems` lists everything
usable instead, in that same order.

## Victory: steel, then XP, then leveling

`GameLoop::runCombat` awards steel first (Steel Pieces, Krynn's currency —
see `docs/CHARACTER_NOTES.md`'s "Gold -> Steel" note; unchanged mechanic
from Milestone 9, renamed in Milestone 22), then
`combat::Monster::xpValue` (sourced from the Monster Manual's own "XP
Value" field — Goblin 15, Kobold 7, Hobgoblin 35, Wolf 120, Giant Spider
420; Baaz Draconian's real formula, "81 + 1/hp," is simplified to a flat
90 near its average roll rather than adding per-hp-XP complexity to the
`Monster` struct for one creature), then calls
`character::applyPendingLevelUps` (`character/Leveling.h/.cpp`) and
appends whatever messages it returns to the same combat log — level-ups,
if any, show up as part of the victory screen. See
`docs/CHARACTER_NOTES.md`'s "Leveling / experience" section for the full
sourcing (PHB Tables 14/20/23/25 for XP, Table 53 for THAC0, Table 60 for
saves, now all the way to level 20) and what's still deferred (Fighter's
extra attacks per round).

## Bug fixed: killing a no-steel-drop monster crashed the game

Timber Wolf, Skeleton, and Zombie all have `STEEL 0 0 0` in
`data/monsters.txt` (deliberately -- an animal or the animated dead
carries no coin). Killing any of them called
`character::roll(monster.steelDiceCount, monster.steelDiceSides)` as
`roll(0, 0)`, and `roll` unconditionally constructed
`std::uniform_int_distribution<int> die(1, sides)` before ever looking at
`count` -- `die(1, 0)` violates that distribution's own precondition
(`min <= max`) and crashed the whole process with a debug assertion,
every single time, for as long as those three monsters have existed in
the roster. Reported by the user via the crash dialog captured mid-fight
against a Timber Wolf, kept here as a record now that it's resolved:

![The MSVC debug assertion dialog interrupting a Timber Wolf fight, "invalid min and max arguments for uniform_int_distribution"](../Bugs/errorinbattle.png)

Fixed at the root in `character::roll` (`character/Dice.cpp`): `count <=
0` now returns `0` immediately, before constructing any distribution --
"roll zero dice" is a real, meaningful call (exactly what `STEEL 0 0 0`
means), not a caller error, so it needs to be a safe no-op rather than
something every call site has to remember to guard against. `sides <= 0`
with a nonzero count is left to assert -- that would be a genuine data
bug elsewhere (no monster's `HP`/`DAMAGE` lines have this problem; checked
directly against every line in `data/monsters.txt`) and should still fail
loudly rather than be silently masked. Verified via a throwaway self-test
(`roll(0, 0)`/`roll(0, 8)`/`roll(0, 20)` all return 0 without crashing;
`roll(1, 8)`/`roll(3, 6)` stay in range across 500 rolls each) and a
clean `/W4` rebuild.

## Showing the math

`combat::AttackOutcome` (`Combat.h`) carries the full roll breakdown behind
every hit/miss result, not just the final damage number: the unmodified
`naturalRoll`, `toHitBonus` (every point added to it -- STR/magic/spell for
the player, always 0 for a monster, which has no additive to-hit term
modeled), the `attackerThac0`/`defenderArmorClass` actually used for that
roll (already folded in any this-fight buff/penalty -- see "Player actions"
above), the resulting `targetNumber` (`attackerThac0 - defenderArmorClass`),
and, on a hit, the `damageDiceCount`/`damageDiceSides`/`damageRoll` plus the
total `damageBonus` added to it. `resolvePlayerAttack`/`resolveMonsterAttack`
populate every field unconditionally (damage fields just stay 0 on a miss)
so a caller never has to re-derive the math from scratch.

`game::describeToHit`/`describeDamage` (`GameLoop.cpp`, file-local) turn
that breakdown into a short bracketed suffix appended to the existing
hit/miss log line, e.g. `You hit the Goblin for 6. [d20 14 +2 = 16 vs
THAC0 18 - AC 6 (need 12)] [1d8 5 +1 = 6]`, or, on the PHB's natural-20/
natural-1 override, `[d20 20 -- natural 20, automatic hit]` with damage
math following normally. Only `playerAttacks`/`monsterAttacks` (the two
`resolvePlayerAttack`/`resolveMonsterAttack` call sites) get this treatment
-- spell damage (`playerCasts`'s `SpellEffect::DamageMonster` case) already
states its amount plainly and has no PHB "roll vs AC" attack math to show,
so it's out of scope here, same restraint as everywhere else in this
project.

Each hit entry typically wraps to two physical lines in the combat log
instead of one (see `drawCombatFrame` below), so `kMaxLogLines` was trimmed
from 12 to 8 in the same change, keeping the box roughly the height it was
before.

## Rendering

`render::MapRenderer::drawCombatFrame` shows the player's HP/AC, every
monster instance's HP/AC (as of Milestone 113, an encounter can have more
than one -- see "Monster encounter groups" below), and a scrolling log
(last 8 entries — older entries fall off, same "most recent last"
convention as a chat log; see "Showing the math" above for why this was
trimmed down from the original 12), followed by the available actions.
Monster HP is tracked as a local `std::vector` inside `runCombat`, not
stored on `combat::Monster` itself — the `Monster` struct is static
content shared by every encounter with that monster type, the same
reasoning `Character` doesn't store per-encounter runtime state either.

## Monster encounter groups (Milestone 113)

The first phase of a Gold Box (SSI's Pool of Radiance ... Dark Queen of
Krynn) -inspired combat-screen pass, scoped down from a full tactical grid
to something self-contained: multiple monsters of the same type per
encounter, individually tracked HP, lettered identity, and a target
picker. **No position/grid/movement exists** -- this project's combat
stays a strictly-ordered, non-positional exchange (see "Combat is not a
`GameState.mode`" above); a real tactical grid, if it happens, is separate,
later, deliberately-unstarted work.

**Group size is sourced, not invented.** Every one of the 26 roster
monsters' real Monstrous Manual / *Dragonlance Adventures* "No. Appearing"
field was re-checked via rendered page images (a field this project never
needed before, since combat was always 1-vs-1) -- see the citations on
each `GROUP` line in `data/monsters.txt`. The real range is then clamped
to one small **invented, explicitly flagged** playability cap -- **4** --
for screen-space/pacing/solo-PC-balance reasons, the same "tuned for
pacing, not sourced" honesty already used for `encounterChancePercent`/
`terrainBias`/`MIN_TOWN_DISTANCE`.

**Deliberately applied to only 14 of the 26 monsters this pass.** The
research turned up real No. Appearing data supporting groups for nearly
the *entire* roster, including several already-dangerous, currently
ungated monsters: Wight (2-16, real level-drain + silver/magic-only
defense), Troll (1-12, huge HP, unmodeled regeneration), Thanoi (1-20),
plus the already-`MIN_TOWN_DISTANCE`-gated Ogre (2-20), Ettin (the book's
own text frames a group as a rare 1d4 exception, not the norm), and
Baaz/Kapak/Bozak/Sivak (all a flat `2d10`, real Dragon Highlord squad/
garrison numbers). Grouping any of those without also re-tuning their
existing danger gates would have shipped an untested difficulty spike, so
-- at the user's explicit direction after this finding was flagged --
**this pass only groups the roster's low/mid-HD "line troop"/wildlife
tier**, exactly the set that already carries no `MIN_TOWN_DISTANCE` gate
today:

| Monster | Real No. Appearing | `GROUP` (clamped) |
|---|---|---|
| Goblin | 4-24 (4d6), p.163 | 4 4 |
| Kobold | 5-20 (5d4), p.214 | 4 4 |
| Hobgoblin | 2-20 (2d10), p.191 | 2 4 |
| Timber Wolf | 2-12 (99% of the time), p.362 | 2 4 |
| Bugbear | 2-8 (2d4), p.32 | 2 4 |
| Ogre | 2-20 (2d10), p.272 | *(left solo -- see above)* |
| Gnoll | 2-12 (2d6), p.158 | 2 4 |
| Ghoul | 2-24 (2d12), p.131 | 2 4 |
| Skeleton | 3-30 (3d10), p.315 | 3 4 |
| Zombie | 3-24, p.373 | 3 4 |
| Baaz Draconian | 2-20 (2d10), DLA p.74 | 2 4 |
| Kapak/Bozak/Sivak Draconian | 2-20 (2d10) each, DLA p.74-75 | *(left solo)* |
| Aurak Draconian | 1-2, DLA p.73 | *(left solo -- barely supports >1 anyway)* |
| Thanoi | 1-20 (1d20), DLA p.78 | 1 4 *(Milestone 125)* |
| Owlbear | 1 (2-8 only in its own lair), p.284 | *(left solo permanently -- real data doesn't support a wandering group)* |
| Wight | 2-16 (2d8), p.360 | *(left solo)* |
| Troll | 1-12, p.349 | *(left solo)* |
| Black Bear | 1-3, p.17 | 1 3 (unclamped -- already under the cap) |
| Worg | 3-12, p.362 | 3 4 |
| Ice Bear | 1-4 (1d4), DLA p.76 | 1 4 (unclamped) *(Milestone 125)* |
| Lizard Man | 8-15 (1d8+7), p.227 | 4 4 |
| Giant Toad | 1-12, p.345 | 1 4 |
| Ettin | 1, rarely 1-4, p.135 | *(left solo permanently -- the book itself frames a group as the exception)* |
| Giant Spider | 1-8, p.326 | 1 4 *(Milestone 125)* |

Revisiting the remaining solo tier above (Ogre/Kapak/Bozak/Sivak/Aurak/
Wight/Troll, all `MIN_TOWN_DISTANCE`-gated, with matching gate re-tuning)
is real, scoped future work, not an oversight -- see "Extending this
later" below. Owlbear and Ettin are a different, permanent case: their own
real No. Appearing data prints a solitary number for a wandering
encounter, so there's no sourced group to add for either regardless of
future re-tuning.

## Monster encounter groups, continued (Milestone 125)

Closed part of the gap the table above tracks. Re-verified all 12
still-solo monsters' real No. Appearing directly against rendered
rulebook page images (Monster Manual pp.135, 272, 284, 326, 349, 360;
*Dragonlance Adventures* pp.73-76, 78) rather than trusting Milestone
113's citations at face value -- every figure matched, including two
worth calling out on their own terms rather than just re-confirming a
number: **Owlbear**'s real entry prints "1 (2-8)" and **Ettin**'s prints
"1 or 1-4" -- in both cases the *wandering* encounter (this project's only
encounter type; there's no lair concept) is a real, sourced 1, with the
larger figure explicitly a lair-only (Owlbear) or rare-gathered-band
exception (Ettin) in the book's own prose. Grouping either would misread
the source, so both move from "deferred" to **permanently solo** in the
table above.

That left 10 genuinely groupable monsters, but 7 of them (Ogre, Kapak,
Bozak, Sivak, Aurak, Wight, Troll) are `MIN_TOWN_DISTANCE`-gated
specifically for being too dangerous solo -- grouping them without also
re-tuning those gates risks the same untested difficulty spike Milestone
113 already flagged and declined to ship (up to 4 Auraks, say). Asked the
user directly this session: leave that 7-monster tier solo for now, and
only ship the 3 monsters that carry no distance/danger gate at all --
Thanoi, Ice Bear, and Giant Spider (see the table above for their new
`GROUP` lines).

**Honest caveat, not smoothed over**: "no distance gate" isn't the same
as "low danger" for two of these three. Thanoi (HD4, comparable to Ogre)
and Ice Bear (HD6+2, XP 707, comparable to the Sivak tier) only lack
`MIN_TOWN_DISTANCE` because their `ONLY_TERRAIN` glacier lock already
keeps them off a starting town's doorstep by a different mechanism --
grouping them does reopen a sliver of the same stacking-danger concern
the 7-monster tier above was just held back for, just gated by terrain
instead of raw tile distance. Giant Spider's own Milestone 113 exclusion
was a separate concern entirely: stacking its real, modeled poison bite
(a saving throw per hit) up to 4-deep. All three shipped anyway this
pass, per the user's explicit direction after this was flagged -- a
real, acknowledged trade-off, not an oversight.

No C++ changes -- pure `data/monsters.txt` additions through the
already-exercised `GROUP` grammar (proven by the 14 Milestone 113 users).
Verified via a clean `/W4` rebuild (zero new warnings) and the piped
save-slot smoke test, which loads `MonsterCatalog` before the save-slot
menu renders. Seeing an actual Thanoi/Ice Bear/Giant Spider band in a live
fight has no piped-testable path (`_getch()` blocks that, same as every
other combat-facing milestone) -- confirming it in practice still needs
the user's own keyboard.

**Targeting is driven by how many instances are alive right now, not the
group size rolled at the start.** A solo fight (still the overwhelming
majority of encounters, since only 14 of 26 monsters carry a `GROUP` line
at all) always auto-targets its one monster with no letter and no picker
-- byte-for-byte the same log wording every single-monster fight has had
since before this milestone. A group fight only shows a target picker
while 2+ instances are still alive; fought down to its last survivor, it
auto-targets exactly like a solo fight. Letters themselves are decided
once from the *starting* group size, so a fight's labels stay stable
("Goblin A"/"Goblin B") even as members die. **As originally shipped this
picker was a separate `drawPickerFrame` screen; Milestone 114 (see below)
added real positions to target, and Milestone 115 moved targeting onto
the combat grid itself -- see "In-frame combat actions" further down.**

**A Fighter's multi-attacks-per-round all land on the one target chosen
for that round** (`character::meleeAttacksThisRound`), matching Gold
Box's "pick a target once per round" convention -- **as originally
shipped**. DQoK.pdf's own manual, re-read while planning Milestone 114,
gives the real rule instead: "If the first target goes down with the
first attack, you can aim the remaining attack at another target" --
fixed in that same milestone (`playerAttacks` now re-picks a target if
the current one dies mid-volley, only giving up if nothing eligible is
left). See "Positional combat grid (Milestone 114)" below.

**Non-damage spell effects that used to implicitly target "the monster"
now ask which one** (`BlockMonsterAttacks` -- Sleep/Hold/Charm/Confusion/
Fear; `DebuffMonsterThac0`/`DebuffMonsterDamage` -- Bestow Curse-style
effects), same "one representative target" simplification this project
already uses elsewhere (e.g. a multi-attack monster simplified to its
single most damaging hit). Webnet asks the same way. The Brooch of Imog's
globe of invulnerability is the one exception -- it wards the *player*,
not a debuff on a monster, so it still blocks every instance's attack each
round it's active, no target needed.

**Every per-monster special ability now rolls independently per living
instance** -- Bozak's Magic Missile chance, Aurak's breath weapon chance,
the Giant Spider's poison bite -- via the same shared `combat::Monster`
stat block, just checked once per alive instance in the monsters' turn
instead of once total. None of the three monsters carrying those flags
are in the grouped tier this pass, so this is currently exercised as
"loop of exactly one" in practice, same as before -- but it's already
correct if a future pass groups any of them.

**A kill's steel/XP/quest-tally is applied the instant that instance's HP
reaches 0**, not deferred to the end of the round -- so a group fight's
log reads in the order things actually happened, and a multi-kill fight
(e.g. killing 2 of 4 Goblins in one round via a Fighter's double attack)
correctly advances a `SLAY goblin 3` objective by 2 in one go, with no
quest-system changes needed. The "Press any key to continue" pause only
fires once, when the *last* instance falls -- individual kills mid-group
just scroll into the log like any other line, so a 4-goblin fight doesn't
interrupt for a keypress after every single kill.

**Baaz Draconians always group now (2-4), never solo** -- the only
already-shipped monster whose *default* behavior actually changes this
pass, since its real No. Appearing never supported a lone Baaz to begin
with. Its "turns to stone" death message now names the specific instance
("Baaz Draconian B falls and crumbles to stone...") rather than the
generic singular line every earlier Baaz encounter had.

**`GROUP <min> <max>`** (`data/monsters.txt` grammar, `MonsterLoader.cpp`,
`combat::Monster::groupMin`/`groupMax`, default 1/1 -- untouched for every
monster without the line): fails fast on `min <= 0` or `max < min`, same
idiom as every other line in this loader. `combat::rollGroupSize(monster)`
(`Monster.h`/`.cpp`) is the one place the actual roll happens -- extracted
as its own function specifically so it's unit-testable, same reasoning as
`character::meleeAttacksThisRound`.

**Rendering**: `MapRenderer::drawCombatFrame` takes a
`std::vector<CombatMonsterView>` (name -- already letter-suffixed by the
caller when relevant, hp, maxHp, armorClass, alive) instead of a single
`combat::Monster` + HP pair. Every instance is always shown, defeated ones
marked `(defeated)` at 0 HP rather than disappearing, so the roster
visibly shrinks over the fight. The footer only adds a `(choose target)`
hint once 2+ are actually alive.

**Interactive verification is required for this milestone more than
most** -- `_getch()` means none of the actual play loop (the target
picker, multiple monsters each attacking per round, a multi-attack
landing all swings on one target, a target dying mid-volley, per-instance
special abilities, a multi-kill fight progressing a quest) can be driven
headlessly. A throwaway self-test covers only what's actually extractable
and pure: `MonsterLoader`'s `GROUP` parsing (valid data + malformed
fail-fast cases) and `combat::rollGroupSize`'s bounds. See
`docs/CURRENT_WORK.md` for the specific scenarios still needing a real
playthrough.

## Positional combat grid (Milestone 114)

Phase 2 of the Gold Box-style combat pass Milestone 113 started: a real
tactical grid with player/monster positions and movement, instead of the
strictly turn-ordered, positionless exchange combat had used until now.
Unlike Milestone 113's group sizes, this system has a real, checkable
source: `References/DQoK.pdf` (Dark Queen of Krynn, an actual SSI Gold
Box Dragonlance game, already used for the Hoopak's weapon table at
Milestone 111) has its own "COMBAT" section (manual pp.9-11) describing
this exact system, re-read before finalizing the design. Everything below
tagged **(sourced)** is transcribed from that section; everything tagged
**(invented)** is this project's own Gold Box-inspired design, same
"tuned for pacing/presentation, not a 2e rulebook mechanic" honesty as the
"3/2 rounds" odd/even split and the sell-back half-price convention --
core 2e AD&D combat itself is abstract "melee range," not squares.

- **The grid** (invented sizing, sourced concept): originally 11x7 cells,
  **bumped to 15x9 at Milestone 129** after the user asked for a bigger
  battlefield, "like the SSI Gold Box games"
  (`render::MapRenderer::kCombatGridWidth`/`kCombatGridHeight`), small
  enough to read inside the existing "organic" combat box, big enough
  that closing distance takes a couple of real rounds. At 15 cells wide, a
  grid row (3 columns per cell plus a 2-column border, 47 total) stays
  well under `kProseWrapWidth`'s floor of 71 even on the smallest
  supported console (`kAbsoluteMinColumns`), so it can never wrap
  mid-row -- see `MapRenderer.h`'s own comment on the constants for the
  exact math. Width must stay odd either way, for
  `GameLoop::runCombat`'s player-centered, symmetric monster spawn. The
  manual's own
  words: "Battle takes place on a tactical combat map that is a detailed
  view of the terrain that the party was in when the combat began...set
  up with an invisible square grid." The real tile's `world::TerrainInfo`
  (`world::terrainFor(grid_.terrainCodeAt(state_.x, state_.y))`, the same
  lookup Frostreaver's glacier check already makes) still decides what
  terrain the fight is happening on. **Color is not carried over**: this
  project's "organic" screen family (`writeBoxed`/`BoxLine`, see
  `docs/ARCHITECTURE.md`'s Milestone 32 note) only supports one color per
  whole line, not per cell, so the grid renders in plain text.
- **Presentation restyle (Milestone 120, invented).** Originally the
  terrain's *glyph* was tiled across every empty cell, so a forest fight
  filled the board with 77 `%` characters and a bog fight with `"`. That
  was flavorful in principle but noisy in practice -- it drowned out the
  `@`/companion/monster letter glyphs that are the only things a player
  actually reads during a round, and forest is one of the highest-
  encounter-chance terrains (11%), so it came up constantly. Three
  changes, all confined to `MapRenderer::drawCombatFrame`, no gameplay
  effect whatsoever:
  1. Empty cells are a uniform `.` (`kCombatFloorGlyph`) regardless of
     terrain.
  2. The terrain is named once instead, on a `kSectionLabelColor` (bright
     cyan) "Battlefield: forest" label line above the grid, built from
     `TerrainInfo::name` -- so the manual's "detailed view of the terrain
     the party was in" is still honored, just stated rather than tiled.
     Every passable terrain's `name` reads correctly after "Battlefield: "
     (combat only ever triggers from `tryMoveOverworld`, so the
     impassable ocean/Blood Sea names never appear here).
  3. The grid gained its own ASCII border (`+---...---+` top and bottom,
     `|` on each row) so the battlefield reads as a bounded map rather
     than floating text inside the much wider combat box. Rows are
     11 cells * 3 columns + 2 border columns = 35, still far under
     `kProseWrapWidth` (76), so nothing wraps.
  The `[X]` target-picker bracket (Milestone 115) is unchanged and still
  legible against the new border, including in the edge columns where it
  abuts the `|` directly.
- **Starting layout** (invented): the player begins centered on the
  bottom row; monster instances spread evenly across the top row,
  centered and spaced two cells apart -- the full 8-row vertical span as
  of Milestone 140 (originally rows `height-2`/`2`, a 5-row gap, widened
  after the user found it too easy to close in one or two rounds).
  Purely local state (`GameLoop::runCombat`'s own `combat::GridPos
  playerPos` and `std::vector<combat::GridPos> instancePositions`), never
  touching `GameState`/`SaveGame`, same "combat isn't saved" precedent as
  monster HP/the log.
- **Movement** (invented mechanic, sourced as a real action in the
  manual): `w`/`a`/`s`/`d` (`render::Key::North/South/East/West`,
  previously ignored inside combat) move the player one cell as a full
  round action -- validated up front (grid bounds, not occupied) the same
  "reject before it costs a round" way an unusable `Cast`/`Inventory`
  press already is, then resolved through the same initiative-ordered
  `playerActs` dispatch as any other action.
- **Melee requires adjacency** (sourced): an ordinary weapon attack can
  only target an instance within `combat::isAdjacent` (Chebyshev distance
  1, the 8 surrounding cells) of the player. If nothing eligible is
  adjacent, the attack is refused ("You're too far away to attack.") and
  the round isn't spent, same forgiving pattern as an unusable spell
  press.
- **Ranged weapons -- this project's first-ever melee/ranged distinction**
  (sourced): the manual's own words, "A character with a missile weapon
  (bow, sling, etc.) may not attack when adjacent to an enemy," and
  separately, "Missile weapons cannot be fired if there is an adjacent
  opponent." The Tinker's Light Crossbow (`character::kLightCrossbowName`,
  `Equipment.h`, same plain-string-compare pattern as
  `kFrostreaverName` -- no new `Character` field) can hit *any* alive
  instance on the grid while the player isn't adjacent to anyone; the
  instant an enemy closes to melee range, ranged attacks are refused
  outright ("An enemy is too close to fire your crossbow!") -- the player
  must melee, move away to re-open ranged options, or flee. This project
  has no other ranged weapon; the Hoopak's own existing melee-only stat
  choice (`docs/CHARACTER_NOTES.md`) is a separate, already-made decision,
  not revisited here. **Not modeled**, explicitly flagged rather than
  silently dropped: 2 arrows/3 darts per turn and real short/medium/long
  range brackets -- the crossbow attacks once per round and hits anywhere
  on the board while unengaged, same no-ammunition-tracking
  simplification this project's crossbow already had.
- **A real opportunity attack** (sourced): "if you move away from an
  adjacent enemy, he gets a free attack at your back and has an improved
  chance to hit." Triggers per-instance, only when a move genuinely takes
  the player out of *that* instance's reach (was adjacent, won't be at
  the destination) -- sidestepping while staying adjacent to it doesn't
  provoke one. Rolled as an ordinary `resolveMonsterAttack`, since the
  manual names the effect ("improved chance to hit") but prints no
  specific numeric bonus to transcribe. Respects the globe of
  invulnerability, `incapacitatedRestOfFight`, and `blockedAttacksRemaining`
  (a webbed/asleep/blocked monster doesn't get a free swing either); does
  **not** replicate Bozak/Aurak's special-ability rolls or the Giant
  Spider's poison check -- an explicit simplification, "a plain weapon
  strike as you turn to leave," not a full re-run of `monstersAct`'s
  entire per-instance logic. If an opportunity attack drops the player to
  0 HP, the move never completes and the ordinary knockout ending fires
  immediately (`GameLoop::runCombat`'s `fightAlreadyEnded` flag, renamed
  from Milestone 113's `fightEndedByBurst` now that more than a death-burst
  can trigger it).
- **Monster movement AI** (invented): no monster in this roster has a
  ranged attack, so every instance is melee-locked by the same adjacency
  rule as the player. An instance not adjacent to the player (and not
  this round's Bozak/Aurak special-ability roll, which stays
  position-independent, same reasoning as player spells below) spends its
  turn closing the distance via `combat::stepToward` instead of attacking
  ("The Goblin A closes in."), rather than attacking from wherever it
  happens to be.
- **Spells, potions, and items stay position-independent** (invented
  scope cut, reasoned from the source): 2e spell ranges are already "at
  range" in spirit, and gating potions/Webnet/Brooch by position would be
  new complexity this pass doesn't need -- `playerCasts`'s
  target-needing effects (damage, block, debuff, instant defeat) and
  `playerUsesWebnet` still use the same unfiltered "any alive instance"
  target list Milestone 113 already established.
- **A Milestone 113 discrepancy found and fixed in the same pass**: the
  manual's real rule for a Fighter's multi-attack is "if the first target
  goes down with the first attack, you can aim the remaining attack at
  another target" -- Milestone 113 originally wasted the remaining swings
  instead ("Your remaining attack finds no target left standing.").
  `playerAttacks` now re-picks a target (via the same eligibility filter)
  each time the current one falls mid-volley, only giving up ("No targets
  remain for your last attack.") when nothing eligible is left at all.

**Deliberately out of scope, sourced findings included** (real DQoK.pdf
mechanics, honestly flagged as not-adopted rather than silently ignored):
segmented (1-10) initiative (this project keeps its existing single
"which side goes first" roll, `combat::playerActsFirst`); variable
movement speed from carried weight/strength/armor (no encumbrance system
exists -- every character moves exactly 1 cell/round); speed-based,
edge-of-map `Flee` (`Flee` keeps its existing "always succeeds
immediately, no cost" behavior, already a documented deliberate
simplification predating this milestone). Thief backstab shipped later,
at Milestone 119 -- see "Thief backstab and Fighter sweep attacks" below.

**Interactive verification is required, same heavier-than-usual flag as
Milestone 113** -- `_getch()` blocks the actual play loop from being
driven headlessly. A throwaway self-test covers only the pure,
extractable pieces: `combat::isAdjacent` across all 8 neighbors plus
self/distance-2, and `combat::stepToward`'s bounds-respecting,
collision-avoiding, greedy approach. See `docs/CURRENT_WORK.md` for the
specific scenarios still needing a real playthrough.

## In-frame combat actions (Milestone 115)

Milestones 113-114 built a real tactical grid, but every sub-choice
inside a fight -- which enemy to attack, which spell to cast -- still
popped a full-screen `render::MapRenderer::drawPickerFrame`, which clears
the terminal and replaces the whole combat frame with a bare list. The
grid, the HP roster, and the log all vanished at exactly the moment the
player needed them to decide -- the user's own complaint ("the picking
who to attack takes you away from the screen"). This is a real deviation
from the source, not just a taste call: `References/DQoK.pdf`'s own
manual targets *on the battle map itself*. Fireball: "Use the CENTER
command to determine who will be in the area of effect... if the spell is
targeted in the center of the screen." Hold Person: "you may aim a hold
person spell at up to 3 targets (use the EXIT command to target fewer)."
Both describe a cursor moved over the visible map, never a separate list
screen -- and the manual's own command vocabulary (CAST, USE, DELAY,
DETECT, CENTER, EXIT) is a menu of named commands shown *during* the
fight, not bare hotkeys with a hint line.

**`render::MapRenderer::CombatPrompt`** (`MapRenderer.h`) is the new
plumbing: built fresh by `GameLoop::runCombat` each redraw, never stored,
and passed as `drawCombatFrame`'s new (defaulted) trailing parameter.
Three states, distinguished by its own `title`/`options` fields (see its
doc comment for the full contract):
- **`title` empty (the default)**: no chooser is open. The footer renders
  a real command row instead of the old fixed hint text -- `ATTACK
  (Enter)   MOVE (wasd)   CAST (m)   USE: <item> (i)   FLEE (f)` -- naming
  only what's actually legal right now (`CAST` only for a caster with
  something memorized, via the same `hasMemorizedSpellsAvailable` check
  the old `m=cast` hint used; the `USE:` hint names whichever item
  `character::availableCombatItems` would open first, so it can never
  drift out of sync with what `'i'` actually does the way the old
  hand-duplicated potion/webnet/brooch priority chain in both
  `drawCombatFrame` and `GameLoop::runCombat` could).
- **`title` set, `options` empty**: target picking. The grid **is** the
  picker now -- `gridCursorIndex` (an index into the `monsters` vector)
  draws that instance's cell as `[X]` instead of ` X ` (cells widened from
  1 to 3 columns to fit the bracket -- 11 * 3 = 33 columns, still well
  under `kProseWrapWidth`), and its HP-roster line gets a `> ` cursor
  prefix, same convention `drawPickerFrame`/`drawShopFrame` already use
  elsewhere. `GameLoop::runCombat`'s `pickTarget` lambda is unchanged in
  every way except which function it calls to render each keystroke --
  same candidate list, same up/down cycling, same Enter/Quit handling.
- **`title` set, `options` non-empty**: an in-frame list chooser (spell or
  item selection, neither of which has a grid cell to point at) --
  `options` render as their own cursor list under the grid, same visual
  shape `drawPickerFrame` used before this milestone. Used by the "Cast
  which spell?" chooser (unchanged logic, just redirected off
  `drawPickerFrame`) and the new "Use which item?" chooser below.

**`character::availableCombatItems(character, today)`** (`Equipment.h`/
`.cpp`) replaces the old fixed-priority `'i'`-key handling in
`GameLoop::runCombat` (Potion, then Webnet, then Brooch, then Staff,
stopping at the first match) with the full list of everything actually
usable this round, in that same order -- built from the four existing
`firstPotionIndex`/`firstWebnetIndex`/`broochAvailableToday`/
`staffCureAvailableToday` checks, so it can't drift from what those
already track. This closes a real, previously-live gap, not just a
presentation one: a character carrying both a Potion and a Webnet could
never reach the Webnet through `'i'` before this milestone, since the
Potion always matched first and the old code never looked further.
`GameLoop::runCombat` auto-selects with no chooser when exactly one item
qualifies (the common early-game case, unchanged UX), and opens the
in-frame "Use which item?" chooser only when there's an actual choice --
same "no picker needed for one candidate" rule `pickTarget` already
follows.

**Deliberately not adopted, flagged rather than silently dropped** (real
DQoK.pdf mechanics, out of scope this pass):
- **A free-roaming cursor over empty grid squares.** The real game's
  cursor walks squares, not just enemies; this instead cycles between
  eligible targets with up/down, same as before. Justified because no
  fight in this roster exceeds 4 enemies and no spell here targets empty
  ground -- but it becomes genuinely insufficient the moment
  area-of-effect spells are added (Fireball's blast, Lightning Bolt's
  8-square line reflecting off walls), which is exactly when a real
  square-cursor should be built instead of patched onto this one.
- **DELAY, QUIC (computer control), and multi-target CENTER/EXIT as named
  commands.** Real manual commands, out of scope: DELAY belongs with
  segmented initiative (not being taken on -- see "Positional combat
  grid" above); QUIC and multi-target EXIT both presuppose a party, which
  this project doesn't have (see "Extending this later" below).
- **A selectable command row.** The row displays each command and its
  key; it is not itself cursor-navigable. `Console::readKey` already maps
  a fixed key set, and a second way to issue the same commands would add
  nothing.

**Interactive verification is required**, same `_getch()` limitation as
every other combat-facing milestone. A throwaway self-test covered the
one pure, extractable piece: `character::availableCombatItems` across no
items, one item, all four available, a Brooch/Staff already used today
(excluded), and a Brooch used yesterday (available again). See
`docs/CURRENT_WORK.md` for the specific scenarios still needing a real
playthrough.

## Thief backstab and Fighter sweep attacks (Milestone 119)

Both flagged since Milestone 113/114 as real, sourced abilities that
structurally couldn't exist with a solo PC -- both need a second party
member on the grid, which the multi-companion roster (Milestones 116-118)
now provides.

**Sweep** (Fighter-type only, `character::ClassGroup::Warrior`):
`References/DQoK.pdf`'s own manual (p.9-10 of the printed manual): "Fighter-types
may also 'sweep' through several weak opponents in one combat round. When
a character 'sweeps,' he automatically attacks all of the weak
opponents." The manual gives no numeric definition of "weak" in the
extracted text (likely a table on the original page 50 that didn't
survive OCR) -- this project defines it as `combat::isSweepEligible`
(`Monster::hpDiceCount <= 1`). Not an arbitrary invention: `hpDiceCount`
is already how this project encodes a monster's real 2e Hit Dice (N HD
rolls N HP dice), confirmed against `data/monsters.txt` -- Goblin,
Kobold, Hobgoblin, and Skeleton (real HD 1-1, ~1/2, 1+1, and 1
respectively) are exactly the four monsters with `HP 1 ...`, the same
"line troop" tier Milestone 113's own GROUP-feature comments already
singled out. Since an encounter is always N copies of one `Monster`
(never a mixed group), eligibility is a single check for the whole fight,
not per-instance. At the moment a Fighter-type (player or companion)
would attack, if the monster is sweep-eligible AND 2+ alive instances are
currently adjacent, sweep replaces the normal attack entirely: no target
picker, one attack roll against **each** adjacent weak instance (matches
"automatically attacks all of the weak opponents" literally), no to-hit/
damage bonus (the manual describes none -- action economy only). With 0-1
adjacent weak instances, or a non-weak monster, falls through unchanged
to the existing single-target flow (`pickTarget` +
`character::meleeAttacksThisRound`'s level-gated multi-attack). Sweep
replaces that multi-attack progression for the round rather than
stacking with it -- DQoK's text doesn't describe an interaction, and "one
swing per weak adjacent enemy" is the literal reading; flagged here as an
invented scoping call, not sourced.

**Backstab** (Thief-type only, `character::ClassGroup::Rogue`): DQoK.pdf's
own manual gives its own **positional** version, distinct from the
classic PHB surprise/unaware-target rule: "A thief 'back stabs' if he
attacks a target from exactly opposite the first character to attack the
target. The thief may not 'back stab' if he has readied armor heavier
than leather. A 'back stab' has a better chance of hitting the defender
and does additional damage." This project follows DQoK's positional
version exclusively (the classic PHB version is NOT modeled), consistent
with this project's established preference for DQoK's own combat-chapter
wording over the classic PHB text when they differ (e.g. Milestone 113's
"if the first target goes down, retarget" rule). Three gates, all must
hold:
- **Armor**: `character::canBackstab` -- `Character::equippedArmor` is
  `ArmorId::None` or `ArmorId::Leather` (StuddedLeather AC7 and heavier
  excluded), and the class group is Rogue.
- **Positional**: `GameLoop::runCombat` tracks, per monster instance, the
  *identity* of the first party member (player or a specific companion,
  by index) to attack it -- **not reset per round**; "the first
  character to attack the target," full stop, for that instance's whole
  lifetime in the fight. A later attacker backstabs when their current
  position equals `combat::oppositeSide(targetPos, firstAttacker's
  CURRENT position)` -- an identity rather than a frozen position, so the
  check always reflects where that first attacker actually is right now
  (they almost always hold position while meleeing, so this only matters
  for the rare retreat). A first attacker who has since been knocked out
  provides no flank (no living ally to backstab around). The player
  always acts before companions each round (`playerActs()` precedes
  `companionActs()` in both initiative branches), so with a *per-round*
  reset a player-Thief could never backstab off a companion's own
  same-round engagement -- persistent, fight-long tracking avoids that
  asymmetry and lets either direction work: a companion backstabbing
  around the player's engagement, or a Thief player backstabbing around a
  companion's (from an earlier round, the common case -- e.g. Bren Alder
  holding one flank while a Thief moves to the other).
- **Numeric effect**: +4 to-hit (the existing generic `thac0Bonus`
  parameter `resolvePlayerAttack` already took for spell buffs) and a
  damage multiplier from `character::backstabDamageMultiplier` (PHB Table
  30, p.57 -- DQoK doesn't print its own numbers, so this project reuses
  the PHB's, same convention already used for magic weapon Steel prices):
  level 1-4 = x2, 5-8 = x3, 9-12 = x4, 13+ = x5, applied to the raw
  weapon die roll only, before Strength/magic/other bonuses (Table 30's
  own text: "multiplied... before modifiers... are applied. Then Strength
  and magical weapon bonuses are added"). `combat::resolvePlayerAttack`
  gained a `damageMultiplier` parameter (default 1) for this;
  `AttackOutcome` gained a matching field so `game::describeDamage`
  renders it honestly (`[1d8 5 x3 +2 = 17]`) instead of silently implying
  a 1d8 rolled higher than 8.
- **Not modeled, flagged**: the classic PHB surprise/unaware-target
  version (DQoK's positional version is used instead); PHB's "ignores the
  target's shield and Dexterity AC bonus" nuance (this project's AC is
  already a single flattened number with no shield/Dex decomposition at
  the point of attack resolution -- same "positionless" simplification
  already flagged elsewhere in this file).

New pure helpers, same "extracted for unit-testability" reasoning as
`isAdjacent`/`stepToward`/`chebyshevDistance`: `combat::oppositeSide`
(`CombatGrid.h/.cpp`), `combat::isSweepEligible` (`Monster.h/.cpp`),
`character::canBackstab` (`Equipment.h/.cpp`, alongside `canWearArmor`),
`character::backstabDamageMultiplier` (`Leveling.h/.cpp`, alongside
`meleeAttacksThisRound`). Both abilities apply symmetrically to the
player and to every companion, in `GameLoop::runCombat`'s `playerAttacks`
and `companionActs` -- one shared `backstabBonus` lambda so the geometry
is written once rather than duplicated.

Verified via a throwaway self-test (`oppositeSide` across all 8 offsets
plus an involution check; `isSweepEligible` at HD 1/2/4;
`canBackstab` across None/Leather/StuddedLeather/ChainMail and a non-Thief
class; `backstabDamageMultiplier` across all four level-band boundaries;
`resolvePlayerAttack`'s new `damageMultiplier` parameter with a pinned
1-sided weapon die -- 26 assertions, all passed, deleted after), a clean
`/W4` rebuild (zero new warnings), and the piped smoke test (real saves
moved aside and restored byte-for-byte). **Interactive verification
needed**, same `_getch()` limitation as every other combat-facing
milestone -- see `docs/CURRENT_WORK.md` for the specific scenarios still
needing a real playthrough (a Fighter sweeping a weak group; a Thief
backstabbing while another party member holds a monster from the
opposite grid side).

## Fireball/Delayed Blast Fireball: a real area-effect burst (Milestone 152)

Until this milestone, every damage spell in this roster -- including
Fireball -- was single-target, even though the positional combat grid
(Milestone 114) has carried everything a real burst needs since it shipped:
`combat::GridPos`, `combat::chebyshevDistance`, and `instancePositions`.
Fireball's damage math (`fireballLikeDamage`, PHB p.192/194: 1d6/level
capped at 10d6) was already correct; only its shape was wrong.

**Research, not assumption.** `References/phb.txt` (the checked-in OCR
extraction) turned out to be page-bled right at Lightning Bolt's entry --
its stat block was followed by an unrelated Meteor Swarm passage, not
Lightning Bolt's own body text -- so the actual PHB page images (pp.193-194,
201-202, 211-212) were read directly instead, per this project's own rule
for OCR-unreliable content. Confirmed:

- **Fireball** (PHB p.192): a true 20-ft-radius sphere, save for half (no
  monster saving-throw system exists in this engine -- see this file's
  "no monster saving throws" note elsewhere -- so, same as every other
  damage spell here, the full roll always applies).
- **Lightning Bolt** (PHB p.194): a directional **line** (a single 5ft x
  80ft bolt, or forked 10ft x 40ft, caster's choice, bouncing off
  unyielding barriers back toward the caster) -- not a radius at all.
  `References/DQoK.pdf`'s own Lightning Bolt entry confirms the same
  shape ("8 squares long in a line...send the bolt down a row of
  opponents...also reflect off walls"). **Deliberately left untouched**
  (still `SpellEffect::DamageMonster`, single-target) -- forcing it into
  a radius burst would misrepresent the one thing that makes it a
  lightning bolt rather than a small fireball (lining up a row, using
  corridors/walls). Real line-shaped targeting is a separate, larger
  follow-up (needs a direction, not just a center point), documented here
  rather than silently dropped, same "not pursued absent a concrete
  reason" posture as the deferred items below.
- **Cloudkill/Cone of Cold/Ice Storm**: also real PHB area spells, also
  **not touched** this pass. Cloudkill is a multi-round drifting poison
  cloud with tiered-HD save-or-die, already modeled differently
  (`SpellEffect::InstantDefeat`) -- not a fit for "add a radius" at all.
  Cone of Cold is a directional cone, same geometry problem as Lightning
  Bolt. Ice Storm is already a deliberate simplification to DQoK p.29's
  own flat single-target number rather than the PHB's real dual-mode
  (hail damage vs. sleet battlefield-effect) area version -- a prior
  sourcing decision, not revisited here without a concrete reason to.

**The targeting model, sourced from DQoK.pdf.** Rather than invent a new
free-aim-at-an-empty-cell input, `References/DQoK.pdf`'s own Fireball entry
describes exactly this project's actual precedent: "Use the CENTER command
to determine who will be in the area of effect... the spell is targeted in
the center of the screen." So `playerCasts` (`GameLoop.cpp`) reuses the
*exact same* `pickTarget` picker every other targeted spell already uses --
the chosen enemy's cell becomes the epicenter, and every alive instance
within a new `SpellCastResult::radius` (Chebyshev distance) of it takes the
same damage roll. No new input mode, no new picker.

**The radius itself is invented, not a feet-per-cell conversion** (this
project has never established such a scale -- the grid's own sizing is
already documented as invented, see "Positional combat grid" above).
`kFireballAreaRadius = 2` (`Spellcasting.cpp`) was chosen because
Milestone 113's own group-spawn layout spaces instances exactly 2 cells
apart, so a burst centered on one group member reliably also catches its
neighbor without trivializing every group fight.

**New `SpellEffect::DamageArea`** (`Spellcasting.h`), carrying the new
`SpellCastResult::radius` field; `fireball`/`delayed_blast_fireball` are
the only two spells using it. `playerCasts`'s new case mirrors the
Fighter-sweep loop's shape (`fightAlreadyEnded` guard per iteration,
`handleInstanceDeath` per kill) rather than duplicating that pattern
differently, and logs one line naming every instance actually caught
("Your Fireball engulfs the Goblin A and Goblin B for 18 each.") -- the
player-visible "hits multiple squares" payoff for this milestone, in
text. A colored/animated version of the same burst is separate, later
rendering work, not part of this change.

Verified via a clean `/W4` rebuild (zero new warnings) and the piped
character-creation smoke test (real `save1.txt`/`save2.txt` untouched,
empty slot 3 used). No throwaway self-test -- the new logic lives entirely
inside `GameLoop::runCombat` (same standing `_getch()` limitation as every
other combat milestone), and the one new pure piece used
(`combat::chebyshevDistance`) is pre-existing, already covered by
Milestone 117's own precedent. **Interactive verification still needed**:
fight a multi-instance group, memorize Fireball, cast it at one instance
while a second is within 2 cells, and confirm both take the same damage
with both named in the log; separately confirm a solo target (or one with
nothing else in range) still reads as a clean single-target hit.

## Bigger battlefield and real per-round movement (Milestone 185, SFML only)

The user supplied a real Dark Queen of Krynn battle screenshot and asked for
"something like this" -- a roughly 50x25 tactical map, impassable walls
forming rooms/cover, and multi-square creatures (the screenshot's dragons
span 2x2). This milestone is the first of a four-part chain covering just
the grid size and real per-round movement; walls, line of sight, and
multi-square creatures are separate, later milestones (186-188), not
attempted here.

- **`ansalon_sfml_phase1` only, deliberately diverging from `ansalon_rpg`.**
  `sfml_phase1/main.cpp`'s own `kCombatGridWidth`/`kCombatGridHeight` grew
  from 15x9 to 50x25; `render::MapRenderer::kCombatGridWidth`/`Height` (the
  console build) stay at 15x9, unchanged. The console's ASCII grid
  physically cannot follow -- 15 was chosen there specifically so a row (3
  columns/cell + a 2-column border) stays under `kProseWrapWidth`'s floor
  (see "Positional combat grid" above); a 50-wide row would blow past that
  on any supported terminal width. Recorded as a deliberate, permanent
  divergence in `docs/PARITY_MATRIX.md`, not a gap to close later. Width no
  longer needs to stay odd the way the console's own symmetric monster
  spawn required -- the SFML spawn just centers on `kCombatGridWidth / 2`,
  well-defined for an even width too (see the constant's own comment).
- **Scrolling camera.** The whole 50x25 field no longer fits the viewport at
  a readable tile size (kept at 56px, unchanged), so the combat draw branch
  now reuses the exact clamped-follow-the-player math the Overworld branch
  already had (`std::clamp(focusPx, halfViewport, max(halfViewport, mapPx -
  halfViewport))`), rather than the old fixed centered draw. The camera
  follows the player while Idle/moving, and the currently-highlighted
  candidate while `PickingTarget`, so an off-screen target is never picked
  blind. The sidebar roster gained a `dist N` figure per living instance
  (Chebyshev distance from the player) as the cheap compensation for losing
  the old whole-map-always-visible view.
- **Real per-round movement, sourced from DQoK.pdf's own Armor Table
  (p.51, visually confirmed via a rendered page image this session --
  `References/dqok.txt`'s OCR extraction badly mangles this specific table,
  columns bleeding together, so the PDF page image was read directly
  instead, per CLAUDE.md's rule for table-heavy/OCR-unreliable content).**
  DQoK's manual: "the character's movement range is displayed... during the
  character's segment in combat" -- previously meaningless here, since one
  step always ended the whole round.
  - **`character::movementSquares(const Character&)`** (`Equipment.h`/
    `.cpp`) resolves `equippedArmor` into a squares-per-round figure via a
    new `ArmorInfo::maxMovementSquares` column. None/Leather/Studded
    Leather/Chain Mail/Splint Mail/Plate Mail all appear in DQoK's table by
    name and are transcribed verbatim (12/12/9/9/6/6) -- notably **not** a
    simple function of AC: DQoK's own table is non-monotonic (its Leather,
    AC8, is faster than its heavier Padded, also AC8; its Elfin Chain, AC5,
    is faster than its own Chain Mail, also AC5), so these are exact name
    matches, not a derived formula. HideArmor/FieldPlate/SolamnicArmor have
    no DQoK counterpart at all (both are this project's own PHB-table
    additions -- see `ArmorId`'s doc comment -- and SolamnicArmor is a
    unique quest reward), so each gets DQoK's own established floor of 6
    squares, a flagged, invented choice, not a sourced one. A Shield does
    NOT reduce movement (DQoK's own Shield row has no movement entry, and
    its footnote only ever describes the AC effect) -- confirmed via the
    throwaway self-test's own shield-doesn't-change-it checks. **Not
    modeled**: DQoK's own carried-weight footnote ("a character carrying
    many objects... can be limited to a minimum of 3 squares per turn") --
    this project has no encumbrance/carried-weight system to hang it on,
    and Milestone 185 doesn't add one (see CLAUDE.md's "no premature
    abstraction").
  - **`combat::Monster::moveSquares`** (new field, default 12) and a
    matching `MOVE <n>` keyword in `data/monsters.txt`'s grammar. Sourced
    for 26 of the roster's 43 entries from the "Base Movement" stat in
    three SSI Gold Box Dragonlance games' own bestiary exports already
    sitting in `References/` (`Champions of Krynn - Monster Manual.html`,
    `Death Knights of Krynn - Monster Manual.html`, `The Dark Queen of
    Krynn - Monster Manual.html` -- fan-wiki exports of each game's actual
    in-engine stat blocks, the same tier of source this project already
    treats `dqok.txt` as for the grid/squares system itself), matched by
    creature name; a couple of cross-checks against the real Monstrous
    Manual increase confidence in the source's fidelity (Wraith's DQoK
    Base Movement 24 matches the real book's own Fl 24 flying rate;
    Spectre's Base Movement 30 is corroborated by two of the three games
    independently). The remaining 17 entries (Kobold, Timber Wolf,
    Bugbear, Gnoll, Thanoi, Owlbear, Black Bear, Worg, Ice Bear, Lizard
    Man, Giant Toad, Harpy, Griffon, Stirge, White Pudding, Brown Pudding)
    have no matching entry in any of the three games and use the
    documented default of 12 instead -- each block's own inline comment in
    `data/monsters.txt` says which case it is. See that file's own `MOVE`
    grammar comment for the full citation.
  - **Round restructure** (`sfml_phase1/main.cpp`): a move now spends one
    square from a new `CombatSession::movementRemaining` (refreshed every
    round in `combatWrapUpRound`) instead of ending the round outright.
    Only an attack, a cast, an item, or the new **Space** "hold action/end
    turn" key (previously unbound) still end the round, via the existing
    `combatFinishPlayerAction`. `combatRollGoFirstAndMaybeActMonsters` (the
    initiative roll + the monsters' whole turn if they win it) is now
    idempotent per round, gated on a new `CombatSession::
    initiativeRolledThisRound` flag reset alongside the movement budget --
    so whichever action comes first in a round (a move, or a direct
    attack) is the one that actually rolls, and every later action that
    same round is a no-op call. Monster AI (`combatMonstersAct`) and
    companion AI (`combatCompanionActs`) each now loop their own existing
    single `combat::stepToward` call up to their own movement budget
    (`monster.moveSquares` / `character::movementSquares(companion)`),
    stopping early once adjacent to their target -- still move-*or*-attack
    per turn, never both, same simplification the pre-185 single-step
    version already had. One small, deliberate behavior change: a move that
    gets cancelled because something moved into the destination cell mid-
    resolution (the "the way is blocked now" case) used to end the round;
    now it just cancels that one step, since the player may still have
    movement and their action left to spend differently.
  - Sidebar gained a `Movement: N/Max` readout (DQoK's own line, quoted
    above) and the Help overlay's combat section documents Space.

**Verified**: a throwaway `MovementSelfTest.cpp` (22 checks -- every armor
tier's `movementSquares` value with and without a Shield, `MonsterLoader`'s
new `MOVE` keyword including its default-when-absent and its two
malformed-line failure cases, and `combat::stepToward` looped the same way
the new AI movement loops use it -- converges to adjacency within budget on
an open board, spends a too-small budget without overspending, and stops
immediately rather than spinning when fully boxed in) all passed, then
deleted along with its temporary CMake target; clean rebuild of all three
targets (zero new `/W4` warnings); an `ansalon_sfml_phase1` launch smoke
test against a copy of real `save1.txt` confirmed every catalog -- including
the enlarged monster roster's new `MOVE` lines -- loads correctly; a piped
`ansalon_rpg` character-creation run (empty slot 3) confirmed the same
shared, now-larger `data/monsters.txt` still parses cleanly for the console
build too. **Not yet interactively confirmed** (no desktop/GUI access this
session): the scrolling camera actually following the player/target, taking
several move steps before acting, Space actually holding/ending a turn,
movement genuinely running out mid-round, and monster/companion AI closing
distance at their own differing rates -- all added to
`docs/CURRENT_WORK.md`'s Playtest backlog.

**Live playtest follow-ups (same session, 2026-09-11)** -- the user then
actually played the build, surfacing three real issues the smoke test
couldn't catch:

- **Monster/companion multi-step moves teleported.** A whole turn's worth
  of steps resolved silently before the next real frame, so a fast
  creature's token visibly jumped straight to its final cell instead of
  appearing to walk there. Fixed with a new `combatAnimateAiStep(GridPos
  focus)` lambda, called once per single step from
  `combatMonstersAct`/`combatCompanionActs`' own movement loops: redraws
  just the combat map/tokens (not the sidebar) and pauses briefly
  (`kAiStepAnimationMs`, an invented pacing number -- tuned from an initial
  70ms up to 130ms per the user's own "slow them down a tick" feedback).
  The camera argument centers on whichever cell is actually moving that
  step (a companion or monster walking far from the player used to animate
  off-screen when the camera stayed fixed on the player).
- **Stray line-fragment artifacts trailing a moving token.**
  `combatAnimateAiStep` deliberately skips `window.clear()` (which would
  also blank the sidebar's separate viewport for that one frame) and
  instead relies on the tile loop's full board repaint every call -- but
  the tiles' own 1px inter-tile gaps didn't fully overwrite a marker/glyph
  pixel left there by the previous step, showing up as small stray colored
  line fragments. Fixed by painting an opaque rectangle over exactly the
  currently-visible map region (computed from the current camera bounds)
  before the tile loop runs, rather than clearing the whole window.
- **The new `dist N` sidebar text could run past the sidebar's own edge**
  for a longer monster name (e.g. "Giant Centipede A"), clipped off the
  window with the number invisible -- the exact same class of bug this
  file already fixed once for the command-row prompt lines (see this
  section's own code comment citing that prior fix). Fixed by switching
  the monster roster line from plain `drawLine` to the existing
  `drawWrappedLine` helper.

Verified the same way as the rest of this milestone: clean `/W4` rebuild
after each change, a launch smoke test, then the user's own live keyboard
retest confirming each fix before moving to the next.

## VIEW command and status-effect visibility (Milestone 186, SFML only)

The user also shared `References/BattleFrames.zip` -- 225 PNG frames
sampled from their own gameplay recording of an SSI Gold Box Dragonlance
game (the same trilogy `DQoK.pdf` already sources this project's combat
grid, Hoopak stats, and spell census from). Reviewing a sample across the
sequence surfaced real UI depth this project didn't have: selecting any
unit shows a full stat card (name/HP/AC/weapon) with any active status
printed on it, reached through a dedicated `VIEW` command separate from
attacking. (The sampled frames also confirmed this project's spellbook
picker already shows a remaining-charge count per spell, e.g. "Fireball
(x2)" -- `combatBeginCast`'s existing `spellPickLabels` -- so nothing
needed building there.)

This milestone is pure **presentation over state already tracked** -- no
new game mechanic, every buff/debuff/block it surfaces was already a real,
sourced spell effect (`combatApplySpellEffect`) with nowhere for the player
to actually see it in force.

- **`v` while Idle opens a "View who?" picker** (`combatBeginView`,
  `sfml_phase1/main.cpp`) listing the player, every alive companion, and
  every alive monster instance. **Costs no round** -- no call to
  `combatRollGoFirstAndMaybeActMonsters` -- same "pure info window"
  treatment as Help/Journal/the character sheet. Confirming a candidate
  (`combatConfirmView`) builds a read-only stat card (HP/AC/THAC0/Weapon
  for the player or a companion; HP/AC/THAC0/Damage-dice for a monster
  instance, which has no individually-named weapon) drawn via the existing
  `drawPickerOverlay` primitive, same reused-not-rebuilt shape every other
  picker-style screen in this file already has.
- **Status tags, derived from state, not tracked separately.**
  `combatPlayerStatusTags()`/`combatMonsterStatusTags(idx)` read
  `CombatSession`'s existing this-fight buff/debuff/block fields (Hasted,
  a THAC0/damage/AC bonus or penalty, Held, N-more-attacks-blocked, Globe
  of Invulnerability active) and turn them into short human-readable
  labels. Getting the sign right mattered: `monsterThac0Penalty`/
  `monsterAcPenalty` are stored as a positive number meaning *worse for the
  monster* (`combat::resolveMonsterAttack`/`resolvePlayerAttack`'s own
  documented convention -- a positive `thac0Penalty` raises the monster's
  effective THAC0, a positive `monsterAcPenalty` inflates its AC), so the
  tags print the stored sign directly with a clarifying parenthetical
  ("+2 THAC0 (harder for it to hit you)") rather than negating it.
  **Companions have no equivalent** -- no per-companion buff/debuff
  tracking exists anywhere in this engine -- so a companion's card simply
  carries no Status line, an honest reflection of what's real rather than
  a gap papered over.
- **The same tags also appear passively in the sidebar roster**, on the
  player's own line and each monster instance's line (companions
  unchanged, since they have nothing to show) -- so an active effect is
  visible every round without opening VIEW. Both lines already go through
  (or, for the player line, were switched to) `drawWrappedLine` rather
  than raw `drawLine`, so the added tag text wraps instead of risking the
  exact clipped-edge bug the `dist N` follow-up above just fixed.
- **Deliberately not built**: `AIM`/`QUICK` commands from the reference --
  ranged targeting already works through the existing attack picker, and
  `QUICK`/`DONE` already map to this engine's existing Enter/Space. No new
  sprite animation or real art either, unrelated to this UI-depth pass and
  out of scope per this project's established placeholder-shapes-now/
  art-later posture.

Verified: clean `/W4` rebuild of all three targets (zero new warnings), an
`ansalon_sfml_phase1` launch smoke test against a copy of real `save1.txt`.
No throwaway self-test -- the whole feature lives in `sfml_phase1/main.cpp`
local lambdas, same situation as the round-restructure work above.
**Interactive confirmation pending** -- see `docs/CURRENT_WORK.md`'s
Playtest backlog.

## Keypad diagonal movement (Milestone 187, SFML only)

Combat movement gained real 8-directional movement via the numpad,
alongside the identical addition to overworld/zone movement -- see
`docs/MAP_NOTES.md`'s own "Keypad diagonal movement" section for the key
bindings and the Milestone 63 diagonal-removal history this doesn't
reverse.

- **No change needed to `combatBeginPlayerMove` itself** to make diagonal
  steps work: it already computed `destination = playerPos + (dx, dy)`
  and only ever checked the one resulting cell -- bounds, occupancy, and
  `combat::isAdjacent`'s own Chebyshev-distance adjacency rule are all
  already diagonal-aware (the DQoK.pdf-sourced 8-surrounding-cells
  definition "Positional combat grid" already established). Only the key
  bindings needed to change.
- **Deliberately NO corner-cutting check here**, unlike overworld/zone.
  Combat has no wall/obstacle concept yet -- the only thing a diagonal
  step could conceivably "cut around" today is another combatant's
  occupied cell (`combatCellOccupied`), and squeezing diagonally past an
  occupied cell isn't the same kind of problem corner-cutting solves for
  solid terrain geometry, so it's left alone rather than inventing a rule
  for it. **Milestone 188 (walls) will need to revisit this** once real
  wall geometry exists on the combat grid -- flagged here so that
  milestone doesn't have to rediscover the gap.
- **Movement cost stays 1 square per diagonal step**, same as a cardinal
  step -- no invented diagonal penalty, same reasoning as the overworld/
  zone note.
- **Fixed a cosmetic bug this surfaced**: `combatBeginPlayerMove`'s
  `dirLabel` computation was a 4-way ternary chain (`y<old ? "north" :
  y>old ? "south" : x<old ? "west" : "east"`) that silently mislabeled
  every diagonal move as just "north" or "south" (dy took priority over
  dx by construction, so a northeast step logged as "You move north.").
  Replaced with a proper 8-way lookup (adding northeast/northwest/
  southeast/southwest) now that a diagonal `destination` is actually
  reachable.

Verified: clean `/W4` rebuild of all three targets (zero new warnings), an
`ansalon_sfml_phase1` launch smoke test, then confirmed working live by
the user (2026-09-11).

## Battlemap walls and wall-aware pathing (Milestone 188, SFML only)

Fourth of the six-part Gold Box-style battlefield chain (see Milestone
185's own intro) -- the DQoK screenshot that started the chain also
showed impassable walls forming rooms/cover, deferred until now. Purely
invented content (nothing in `References/DQoK.pdf`'s own combat section
specifies wall geometry) laid over the already-sourced grid/movement
system.

- **`data/battlemaps/<terrain-name>.txt`, one per encounter-capable
  terrain.** Of `world::Terrain.cpp`'s 12 codes, only the 9 that are both
  passable and have a nonzero `encounterChancePercent` get a file
  (glacier, mountains, hills, forest, bog, salt flat, savannah,
  grassland, road -- ocean/Blood Sea/shallow water are impassable and
  never trigger combat, same guarantee `combat::MonsterCatalog::
  randomMonster` already relies on). Each file is the smallest possible
  version of the raw `GRID`/`ENDGRID` block `docs/ZONE_NOTES.md` already
  documents for zones (literal, no trimming, fails fast on ragged rows)
  -- no `NAME`/`ENTRY`/`POI` section, since a battlemap has no NPCs, no
  entry point, and nothing to look at. Only two characters appear: `.`
  (open) and `#` (wall) -- no separate tree/water/doorway vocabulary the
  way `world::ZoneTile` has for hand-authored interiors; nothing in this
  engine's combat presentation shows a per-cell name or description for
  either, so a second vocabulary would be pure unused indirection (see
  CLAUDE.md's "no premature abstraction").
- **`world::BattleMap`/`BattleMapLoader`/`BattleMapCatalog`**
  (`src/world/BattleMap*.{h,cpp}`, `ansalon_sfml_phase1`-only in
  `CMakeLists.txt` -- `ansalon_rpg` and the parked `ansalon_sfml_trial`
  spike don't get these files at all). `BattleMap` is a much smaller
  sibling of `Zone`, not a reuse of it -- no POIs, no entry point, just
  `isWall(x, y)` (true for any out-of-bounds coordinate too, mirroring
  `Zone::tileCodeAt`'s "edges are a hard boundary" convention). No
  dependency on `combat::` -- `world::` stays decoupled from
  `combat::`/`character::`, same architecture boundary `docs/
  ZONE_NOTES.md` already documents for `Zone`/`PointOfInterest`.
  `BattleMapLoader` reuses `ZoneLoader.cpp`'s own `trim`/`stripCR`/
  `fail(path, lineNumber, message)` idiom, and fails fast on: the file
  not opening, a row width/count mismatch against the caller-supplied
  expected size (`sfml_phase1/main.cpp`'s own `kCombatGridWidth`/
  `kCombatGridHeight`, 50x25), any character other than `.`/`#`, and --
  the one genuinely new validation rule this milestone adds -- **a
  non-fully-open row 0 or last row.** Monster instances always spawn on
  row 0 and the player/companions always spawn on the last row
  (`combatStartEncounter`), so this guarantees a hand-authored map can
  never accidentally trap a spawn; interior rows are where each
  terrain's walls actually go. `BattleMapCatalog::loadAll` loads all 9
  at startup (`sfml_phase1/main.cpp`, alongside the other catalogs) from
  a small fixed terrain-code-to-filename table mirroring `Terrain.cpp`'s
  own `kTable` style (flagged to stay in sync if a terrain type is ever
  added/removed/renamed); a missing or malformed file throws, caught by
  `main()`'s existing outer try/catch as a FATAL EXCEPTION, same as
  every other startup catalog. `forTerrain(code)` returns `nullptr` for
  an uncovered code (defensive only -- combat never actually starts on
  one of the 3 excluded terrains).
- **Wall density flavored per terrain, generated from an explicit,
  hand-specified rectangle list** (a one-off scratchpad script, same
  "generated from explicit intent" spirit as `tools/
  generate_overworld.py` producing `data/overworld.grid` -- not a
  runtime dependency, and not random placement): road sparsest (~1%,
  the safest terrain), grassland/savannah/salt flat light (~2-3%),
  bog/glacier moderate (~8-10%, water pools/ice formations), hills/
  forest/mountains densest (~11-14%, rock outcrops/tree stands).
- **`combat::stepTowardBfs`** (`src/combat/CombatGrid.{h,cpp}`) --
  `combat::stepToward`'s own greedy single-axis heuristic (try the
  bigger-gap axis, fall back to the other, else stand still) can
  permanently dead-end against a wall: given a straight wall segment
  directly between mover and target with no vertical component left to
  fall back on, it tries the blocked axis, has nothing else to try, and
  gets stuck one cell short forever. The new function instead computes a
  real BFS distance field outward from the target (cardinal directions
  only, matching `stepToward`'s own existing cardinal-only movement --
  this is a pathing-*quality* fix around walls, not new AI diagonal
  capability, a deliberately separate, unrequested scope expansion),
  then steps to whichever of the mover's own unblocked cardinal
  neighbors has the smallest distance to the target (ties broken by
  straight-line closeness to the target). Same "stands still" contract
  as `stepToward` when nothing reachable is unblocked. Doesn't require
  the target's own cell to be unblocked -- callers already always
  include a live target's own occupied cell in the `blocked` list they
  pass, same as `stepToward` always required.
  `combatMonstersAct`/`combatCompanionActs` (`sfml_phase1/main.cpp`) are
  the only two callers, each now appending `CombatSession::
  wallPositions` (this fight's whole wall layout, flattened once in
  `combatStartEncounter` rather than re-querying `BattleMap::isWall` for
  all 1250 cells on every single movement check) into the same
  `blocked` vector they already built from occupied cells, then calling
  `stepTowardBfs` instead of `stepToward`. `stepToward` itself is
  completely untouched and still used by `ansalon_rpg`'s own console
  combat AI, which has no walls to route around.
- **Player movement** (`combatBeginPlayerMove`) gained a wall check
  ("Blocked: cannot walk onto a wall.", same message shape as the
  overworld's "Blocked: cannot walk onto X.") and, for a diagonal step,
  the exact corner-cutting check Milestone 187's own doc comment flagged
  as needing revisiting once real wall geometry existed: both flanking
  cardinal cells must also be wall-free, not just the destination itself
  ("Blocked: can't cut across the wall."), checking walls only, not
  occupancy -- corner-cutting is about solid terrain geometry, not
  another combatant's square, same reasoning the overworld/zone version
  already established.
- **Rendering**: a new `combatWallTileShape` (a darker, cooler flat
  color than the existing uniform floor tile) drawn in place of the
  ordinary floor tile wherever `BattleMap::isWall` is true, in both of
  this screen's two documented, deliberate duplicate tile loops
  (`combatAnimateAiStep`'s per-step animation draw, and the main
  per-frame combat branch). Same "flat color, no sprite art yet"
  placeholder philosophy already established for this whole screen --
  no new visual vocabulary beyond one more color.

**Deliberately not attempted**: line of sight (Milestone 189 needs this
milestone's real wall geometry to exist first); multi-square creatures
(Milestone 190); AI diagonal movement (stays cardinal-only, matching
`stepToward`'s own pre-existing behavior); any per-terrain wall color,
name, or description (no such concept exists anywhere in this engine's
combat presentation, and DQoK's manual doesn't call for one either).

Verified: a throwaway self-test (`BattleMapSelfTest.cpp`, deleted after
-- 53 checks) covering `BattleMapLoader`'s valid-parse case and all five
fail-fast cases (bad width, bad row count, invalid character, non-open
top row, non-open bottom row, plus a missing file), and
`combat::stepTowardBfs`: open-board convergence to adjacency, a
constructed wall-corridor scenario proving `stepToward` really does
dead-end there while `stepTowardBfs` correctly detours through a gap,
and the fully-boxed-in "stands still" case. Clean `/W4` rebuild of all
three targets (zero new warnings, confirming `ansalon_rpg` and
`ansalon_sfml_trial` are both genuinely untouched by this milestone). An
`ansalon_sfml_phase1` launch smoke test against a copy of a real save
confirmed all 9 battlemaps load at startup alongside every other
catalog; a piped `ansalon_rpg` character-creation run (empty slot 3)
confirmed the console build and its shared data files are unaffected.
**Interactive confirmation still needed** (no desktop/GUI access this
session) -- see `docs/CURRENT_WORK.md`'s Playtest backlog: walking into
a wall logs the right message, a diagonal corner-cut against a wall is
blocked, monster/companion AI visibly detours around a wall cluster
instead of getting stuck, and wall tiles render as visually distinct
from open floor.

## Line of sight (Milestone 189, `ansalon_sfml_phase1` only)

Sixth-of-the-original, fifth-shipped part of the Gold Box battlefield
chain. Milestone 188 gave the combat grid real walls but nothing yet
checked them for anything other than movement -- the Light Crossbow (this
project's only ranged weapon) and every targeted spell could still hit
any alive instance on the board regardless of walls in between. This
milestone closes that gap with a real sightline check, now that wall
geometry actually exists to check against.

- **`combat::hasLineOfSight(GridPos a, GridPos b, const std::vector<GridPos>& walls)`**
  (`src/combat/CombatGrid.h`/`.cpp`) -- a standard integer Bresenham line
  walk from `a` to `b`, false the moment an intermediate cell is in
  `walls`. The two endpoints themselves are never checked (a shooter's/
  caster's own cell and a live target's own cell can't be walls). `walls`
  takes the same flattened-list shape `stepToward`/`stepTowardBfs`'s own
  `blocked` parameter already does, so a fight's existing
  `CombatSession::wallPositions` plugs straight in with no new per-call
  flattening. Deliberately a *plain* sightline check -- it does NOT apply
  the movement-only corner-cutting nuance Milestones 187/188 layer onto
  diagonal steps; a line that grazes between two diagonal wall cells isn't
  treated specially. This is a scope call, not an oversight: extending
  corner-cutting from movement to sightlines is a separate, unrequested
  refinement, not a hole an actual sourced rule fills.
- **Ranged weapon (`combatBeginPlayerAttack`)**: the Light Crossbow's
  existing "hits anyone on the grid while unengaged" candidate list now
  also requires `hasLineOfSight` to each candidate. Refused with a
  distinct message ("Nothing in your line of sight.") when the only
  reason no target qualifies is a blocked sightline, keeping the
  pre-existing "You're too far away to attack." for the genuine
  nothing-alive/nothing-adjacent case -- the player is told the real
  reason either way.
- **Spell targeting (`combatCommitSpellChoice`)**: the same "every alive
  instance is eligible" candidate list every `needsTarget` spell effect
  builds (DamageMonster, DamageArea's epicenter pick, BlockMonsterAttacks,
  both debuffs, the combined buff/debuff, InstantDefeat) now also
  requires `hasLineOfSight`. **AoE splash is deliberately not re-filtered
  by this**: once Fireball/Delayed Blast Fireball's epicenter cell itself
  passes the sightline check, every alive instance within `result.radius`
  of it still takes the blast regardless of walls between *them* and the
  epicenter -- 2e's real burst-vs-wall diagramming rules are their own
  system this project has already declined to model elsewhere (see this
  file's other "explicitly flagged, not modeled" notes), and a fireball
  that already detonated doesn't stop mid-explosion at a wall corner.
- **A genuinely new state this creates**: 0 line-of-sight-eligible spell
  targets. Before this milestone, at least one alive instance was always
  eligible (no filter existed at all), so this case was structurally
  impossible. Handled the same way `combatBeginPlayerAttack` already
  handles 0 eligible attack targets -- the round is still spent, and here
  the spell is *also* already consumed (`character::castSpell` runs
  before targeting, unchanged from before this milestone) -- logging
  "Your `<spellName>` finds no target in sight." rather than silently
  doing nothing.
- **Companions and monster AI are untouched**: companions never cast
  spells or use a ranged weapon in combat (melee-adjacency or
  `stepTowardBfs`-pathing only), and no monster in this roster has a
  ranged attack -- see this file's own "Monster movement AI" note above.
  Nothing to gate on either side.
- **Not attempted**: Lightning Bolt's real directional line/wall-bounce
  shape (PHB p.194, DQoK.pdf's own matching entry) -- already documented
  above in this file's Fireball/Lightning Bolt section as "a separate,
  larger follow-up" needing direction-based targeting (a line, not a
  center point), not something a sightline primitive alone provides. This
  milestone only removes the blocker that note names (no wall geometry
  existed before Milestone 188); the bolt still resolves as an ordinary
  single-target `DamageMonster` effect, gated by the same LOS check as
  any other targeted spell above, nothing more.
- **No new rendering**: no drawn sightline, no grayed-out picker entries
  -- a blocked candidate is simply absent from the list, same "logic
  only" precedent adjacency filtering already established.

Verified: a throwaway self-test (`LineOfSightSelfTest.cpp`, deleted after
-- 12 checks) covering open-board LOS in all three axis types, a wall
directly on a horizontal/vertical/diagonal line (blocked), a wall off the
line (not blocked), blocking at the nearer of two walls on one line, and
the documented endpoints-never-checked contract (including a same-cell
degenerate case). Clean `/W4` rebuild of all three targets (zero new
warnings). An `ansalon_sfml_phase1` launch smoke test against a copy of
real `save1.txt` confirmed every catalog, including `BattleMapCatalog`,
still loads cleanly; a piped `ansalon_rpg` character-creation run (empty
slot 3) confirmed the console build is unaffected. **Interactive
confirmation still needed** (no desktop/GUI access this session) -- see
`docs/CURRENT_WORK.md`'s Playtest backlog: firing the crossbow or
casting a targeted spell at something behind a wall refuses with the new
message, and doing either with a clear sightline still works exactly as
before.

## Multi-square creatures + a real Dragon (Milestone 190, `ansalon_sfml_phase1` only)

Sixth and final part of the Gold Box battlefield chain. The chain's own
motivating case -- the DQoK screenshot's 2x2 dragons -- had no roster
monster to exercise it (this project had zero dragons); asked the user how
to handle that, and they chose to both build the multi-square mechanic and
add a real dragon in this milestone, giving the exact footprint convention
the Gold Box games actually use.

- **`Monster::footprintWidth/Height`** (`src/combat/Monster.h`, new `SIZE
  <width> <height>` grammar line, default 1x1) -- **not a 2e stat**: the
  real Monstrous Manual's own SIZE field (S/M/L/H/G + a printed dimension,
  e.g. the Blue Dragon's "G (42' base)") never ties to a grid footprint,
  that's purely a Gold Box rendering choice. The footprint numbers actually
  used come from the user's own stated convention: tall bipedal creatures
  are 1 wide x 2 tall (**Ogre, Troll**), wide creatures are 2 wide x 1 tall
  (**Griffon**), and a dragon is 2x2 (both) -- the original DQoK screenshot
  that started this whole chain showed exactly that. Applied to exactly
  those three existing monsters plus the new Dragon below; nothing else in
  the roster changed (Ettin, also a tall giant, was deliberately left at
  the 1x1 default since the user's own examples named only Ogre/Troll --
  extending further is a separate, unrequested call for a future session).
  No Pegasus was added either -- the user named it only as a second example
  of the "wide" convention, not a roster request.
- **Solo-only, enforced.** Every SIZE-carrying monster in this roster
  (Ogre/Troll/Griffon already, the Dragon by design) has no `GROUP` line --
  this milestone never has to solve two overlapping big footprints at once.
  `MonsterLoader` fail-fasts if a monster's `SIZE` area is > 1 while its
  `GROUP` allows more than one instance, protecting a future session from
  silently hitting the unhandled case.
- **New `combat::` helpers** (`CombatGrid.h`/`.cpp`, alongside
  `isAdjacent`/`stepToward`/`stepTowardBfs`): `footprintCells`,
  `isAdjacentToFootprint`, `footprintFits`, `nearestFootprintCell`, and
  `stepFootprintToward` -- the last one deliberately the SAME simple greedy
  "prefer the larger-gap axis, fall back, stand still if both blocked"
  heuristic `stepToward` used *before* Milestone 188, just validated
  against a whole rectangle instead of one cell. **No multi-cell BFS
  routing was attempted** -- a real distance field over footprint
  *placements* (not single cells) is a non-trivial generalization of
  `stepTowardBfs`; since every multi-cell creature in this roster is solo,
  an occasional wall-corner dead-end a smarter path would avoid is an
  accepted, documented restraint, not an oversight.
- **Every footprint-aware call site degenerates to the exact pre-existing
  1x1 behavior**: melee/ranged eligibility, opportunity-attack triggers,
  companion/monster "am I adjacent" checks, `combatCellOccupied`'s
  wall/occupant blocking, Milestone 189's `hasLineOfSight` endpoint (now
  the nearest footprint cell, not the bare anchor), and the sidebar's
  `dist N` all switched from bare `GridPos` equality/`isAdjacent` to the
  footprint-aware helpers above -- for a `footprintWidth == footprintHeight
  == 1` monster (every one of them before this milestone) these compute
  identically to the old code, verified by the self-test below reusing the
  same helpers at 1x1. One exception, deliberately NOT made footprint-aware:
  `combatAdjacentWeakInstances` (Fighter sweep eligibility) -- sweep only
  ever applies to `isSweepEligible` ("weak", `hpDiceCount <= 1`) monsters,
  and no SIZE-carrying monster in this roster is anywhere near that weak,
  so the path is unreachable in practice.
- **Spawn placement was NOT changed.** The existing centered top-row
  placement (`centerX, 0`) already leaves enough clearance for any
  footprint this roster uses (2 cells in either dimension against a 50x25
  grid) -- adding a general clamp/fit step for a case that can't actually
  overflow would be unneeded complexity, not a real gap.
- **Rendering**: the existing placeholder circle marker
  (`combatMonsterMarker`) is resized/recentered per-instance to span the
  whole footprint (radius scaled by `max(width, height)`, centered on the
  footprint's own midpoint) in both of this screen's two documented
  duplicate tile/unit-draw loops, plus the target-picker's highlight
  rectangle. No new shape object -- same "flat placeholder, no sprite art
  yet" philosophy as every other combat visual so far.
- **Breath weapon generalized** (previously hardcoded to the Aurak's exact
  "noxious cloud, 20/10 damage, blinds" case in both `GameLoop.cpp` and
  `sfml_phase1/main.cpp`): new `Monster::breathDamageDiceCount/Sides/
  FlatBonus`, `breathWeaponBlindsOnFail`, `breathWeaponName` fields and a
  matching `BREATH_DAMAGE <count> <sides> <flat> <blinds 0|1> <name...>`
  grammar line, the only way to give the new Dragon a differently-flavored
  breath (lightning, no blind) without a second copy-pasted special case.
  Half-on-save now rounds down at runtime (`fullDamage / 2`, the real 2e
  rule) instead of a second hardcoded constant -- Aurak's own
  `BREATH_DAMAGE 0 0 20 1 noxious cloud` (a dice-triple's honest expression
  of a flat, non-rolled 20, using `character::roll`'s existing "count<=0
  returns 0" contract already relied on by every `STEEL 0 0 0` monster)
  reproduces its original 20-full/10-half numbers byte-for-byte, verified
  by the self-test below.
- **The new Blue Dragon** (`MONSTER dragon_blue`, `data/monsters.txt`) --
  the War of the Lance's iconic evil chromatic dragon color (Highlord
  Verminaar's own dragon armies in *Dragons of Autumn Twilight*), matching
  this project's Chronicles-era focus the way every other roster pick
  already does:
  - Real Monster Manual (2nd ed).pdf p.66 (a rendered page image read
    directly this session -- a dragon's age-category table is exactly the
    "table-heavy" content this project reads as an image rather than
    trusting OCR for) gives an adult-tier reference (AC 0/THAC0 7/HD 14)
    far above this roster's existing HD11 ceiling, so core combat stats
    (HP/AC/THAC0/MOVE/XP) are instead sourced from Champions of Krynn's own
    bestiary (References/"Champions of Krynn - Monster Manual.html", "BLUE
    DRAGON" -- one of the same three SSI Gold Box games Milestone 185
    already established as a valid `MOVE` source): AC 2, HP 50 (`HP 10 8
    5`), THAC0 10, MOVE 24, XP 3650. Bite damage (`DAMAGE 3 8 0`) is sourced
    from BOTH: the book's printed "3-24" and the game's own "3d8" agree
    exactly.
  - Breath weapon: the book's own text ("a 5' wide bolt of lightning that
    streaks 100' in a straight line... save vs. breath weapon for half
    damage") and its age-category table's "Young" row (age 3 of 12) give
    `BREATH_DAMAGE 6 8 3 0 bolt of lightning` -- picked to match the
    physical stat line above (itself sourced from the game rather than the
    book's much higher adult-tier numbers), no blind (the book gives
    lightning breath no such secondary effect, unlike the Aurak's noxious
    cloud). Same invented 30%-per-round pacing as the Aurak (`BREATH_WEAPON
    30`) -- the book gives no real frequency for either.
  - `ONLY_TERRAIN _` (salt flat): the real Climate/Terrain is "Arid
    deserts" -- salt flat is this project's own established desert analog,
    same reasoning already on record for the Gnoll's `EXCLUDE_TERRAIN _`.
  - `STEEL 6 10 20` (avg ~53, above the roster's prior ceiling of `5 10 0`)
    and `MIN_TOWN_DISTANCE 60` (above the prior ceiling of 55) are both
    invented -- dragons hoard treasure but the real TREASURE field just
    prints "Special," no formula to source from, and this is meant to be
    the roster's single most dangerous, farthest-gated encounter. Its XP
    (3650) is honestly reported as-is, even though Shambling Mound's real
    book XP (6000) is still higher -- this project doesn't force XP to
    track danger/rarity 1:1, each number is whatever its own source prints.
  - `SIZE 2 2`, solo (no `GROUP` line).
- Verified: a throwaway self-test (`MultiSquareSelfTest.cpp`, deleted after
  -- 19 checks: `footprintCells` for 1x1/1x2/2x1/2x2, `isAdjacentToFootprint`
  true/false including a diagonal corner, `footprintFits` against bounds
  and a blocked cell, `nearestFootprintCell` picking the actual closest
  cell, `stepFootprintToward` making progress/refusing an edge-hanging
  step/standing still when boxed in, and the regeneralized breath weapon
  reproducing Aurak's exact original 20/10 numbers). Clean `/W4` rebuild of
  all three targets (zero new warnings). An `ansalon_sfml_phase1` launch
  smoke test against a copy of real `save1.txt` confirmed all 44 monsters
  (43 before, the new Dragon included) load without throwing; a piped
  `ansalon_rpg` character-creation run (empty slot 3) confirmed the console
  build and shared data files are unaffected. **Interactive confirmation
  still needed** (no desktop/GUI access this session) -- see
  `docs/CURRENT_WORK.md`'s Playtest backlog: a 2x2 Dragon or 1x2/2x1
  Ogre/Troll/Griffon actually rendering at the right size, blocking the
  right cells for pathing/adjacency, and the Dragon's lightning breath log
  text.

This closes the six-part Gold Box battlefield chain (185-190) in full.

## Extending this later

- **A party of up to six characters.** The single biggest remaining gap
  between this project and a real Gold Box game -- DQoK.pdf's entire
  combat chapter assumes it: deployment order, front-line/back-line
  positioning ("always keep magic-users and missile weapons safe behind
  the front line"), per-character turns, NPC control (the `UIC` command),
  and unconscious-but-not-dead party members left on the field. It's also
  what unblocks several other real, sourced mechanics that structurally
  can't exist with a solo PC: thief backstab (needs a second attacker
  standing opposite the target), Fighter "sweep" attacks against multiple
  weak opponents, and the QUIC/multi-target EXIT commands flagged above.
  Recorded here honestly as a real candidate, not proposed lightly --
  this would touch the save format, character creation, and every combat
  (and probably several non-combat) screen.
  - **Phase 1 (Milestone 116) shipped the smallest real slice**: one
    recruitable companion (`character::buildCompanion()`, a fixed Human
    Fighter), joined via a new `RECRUIT` zone-grammar line, shown on the
    character sheet and HUD, and persisted via a single save-format bool
    -- see `docs/ARCHITECTURE.md`'s "Party companions" and
    `docs/CHARACTER_NOTES.md`'s "Party companion" section. Combat was
    completely untouched: `runCombat` didn't read the companion at all.
  - **Phase 2 (Milestone 117) shipped full mutual combat**, chosen over a
    smaller "free companion" (deals damage, can't be hit) slice via an
    `AskUserQuestion` scope check -- a companion immune to harm would be a
    hollow half-measure. The companion now occupies its own cell on the
    tactical grid, auto-attacks or paths toward the nearest alive instance
    each round (AI-controlled, no player-directed control yet), and
    monsters pick between the player and companion as their melee target
    (adjacent-to-both is a coin-flip; adjacent-to-neither paths toward
    whichever is nearer via the new `combat::chebyshevDistance`). Real HP
    now persists (`GameState::companion.currentHp`, a second field on the
    save format's `COMPANION` line, backward-compatible with the
    Milestone-116 one-token form). A knocked-out companion (HP <= 0) stops
    participating for the rest of that fight but doesn't end it; `Rest`/
    `BedRest` heal the companion the same way they heal the player. Needed
    zero changes to `combat::resolvePlayerAttack`/`resolveMonsterAttack`/
    `rollSavingThrow` -- all three already took a generic `const
    character::Character&`, exactly the payoff Phase 1's design doc
    predicted. Deliberately still deferred, honestly flagged: the Brooch of
    Imog's globe wards the player only; Bozak's Magic Missile and Aurak's
    breath weapon stay hardcoded player-only attacks (never pick the
    companion, same family as the "no square-cursor for AoE" gap below);
    Sivak's death-burst still only damages the player regardless of who
    lands the kill; companion movement never provokes/takes opportunity
    attacks; there's no "finish off a downed ally" mechanic; and every cure
    spell (`character::SpellEffect::HealCaster` -- Cure Light Wounds and
    the rest of that family, `Spellcasting.cpp:385/406/410/426`) can only
    ever target whoever cast it, with no target picker at all, so a Cleric
    standing next to an injured companion has no way to heal them --
    surfaced by live playtesting 2026-09-10 (`GameLoop.cpp:2664`,
    `sfml_phase1/main.cpp:2286`, same self-only behavior in both builds, so
    not something the SFML port introduced). A real fix would need
    `HealCaster` to become a real targeted heal (reusing the existing
    `PickingTarget`/target-picker machinery both builds already have for
    attacks/offensive spells, not adjacency-restricted, same as other
    non-melee spell targeting) offered whenever an ally is actually
    injured, auto-resolving to self when no companion is hurt. See
    `docs/ARCHITECTURE.md`'s "Party companions" section for the full design.
  - **Phase 3a (Milestone 118) shipped a real multi-companion roster.**
    `GameState::companions` grew from a single `hasCompanion`/`companion`
    pair to a `std::vector<game::RecruitedCompanion>`, with no hardcoded
    numeric cap -- bounded by content, not a constant. A second companion,
    Dessa Corrin (a Human Thief, id `dessa_corrin`, recruited at Haven),
    joins Bren Alder; `RECRUIT` gained a real id payload (`RECRUIT <char>
    <companion-id>`), cross-checked by `main.cpp` against `character::
    isKnownCompanionId` the same deferred way `QUEST`/`SHOP_LOCKED` ids
    already are. Combat control stayed exactly AI-only -- this phase grew
    the *count* dimension, not the *control* dimension: `companionActs()`
    now loops every companion in roster order, and monster AI's old
    player/companion coin-flip generalized to a `PartyTarget` list (player
    + every alive companion) picked among uniformly at random via
    `character::roll(1, N)` when more than one is adjacent. `SaveGame`'s
    `COMPANION` line gained an id token (`COMPANION <id> <currentHp>`,
    repeatable, one per recruit), backward-compatible with the legacy
    one-token `COMPANION 1` form (mapped to id `bren_alder`). See
    `docs/ARCHITECTURE.md`'s "Party companions" section for the full
    design.
  - **Backstab and sweep (Thief/Fighter-type party abilities that need a
    second party member) shipped at Milestone 119** -- see "Thief backstab
    and Fighter sweep attacks" below.
  - **Still open**: player-directed control in combat (a UIC-style toggle
    -- DQoK.pdf's own manual: "You control the actions of PCs. The
    computer controls the actions of monsters, NPCs, and PCs set to
    computer control with the UIC command"), deployment order, plus the
    smaller Phase 2 gaps listed just above (still true for however many
    companions exist).
- **The rest of the roster's real group sizes**: Milestone 125 (see
  "Monster encounter groups, continued" above) closed part of the gap
  Milestone 113 left open, grouping Thanoi/Ice Bear/Giant Spider and
  ruling Owlbear/Ettin permanently solo (their own real No. Appearing is
  itself a wandering-encounter 1). Still open: Ogre, Kapak, Bozak, Sivak,
  Aurak, Wight, and Troll -- all seven already `MIN_TOWN_DISTANCE`-gated,
  all with real No. Appearing data supporting groups, deliberately left
  solo again at the user's direction rather than shipping an untested
  difficulty spike. Would need a gate re-tuning pass (raising the
  distance thresholds, or an invented lower group cap for this
  elite/dangerous tier) before grouping is safe to ship for them.
- **Spellcasting**: Mage/Cleric now select and cast from a real,
  PHB/DQoK-sourced multi-level spellbook (50 implemented spells across
  Mage's 9 levels and Cleric's 7 — see `docs/CHARACTER_NOTES.md`'s
  spellcasting section for the full census, including what's sourced but
  intentionally not castable yet).
- **Equipment past the General Store's short list**: armor tiers/weapon
  upgrades exist now (see `docs/CHARACTER_NOTES.md`'s "Equipment"
  section), but there's still no carried-item inventory, no selling gear
  back, and only one shop (Solace's General Store).
- **Saving throws for effects other than poison/breath weapon**:
  `rollSavingThrow` is general-purpose, but the Giant Spider's poison bite
  and (as of Milestone 99) the Aurak's breath weapon are still the only
  things that call it (see "Saving throws in combat" above) --
  `character::SaveCategory::PetrificationPolymorph`/`RodStaffWand`/`Spell`
  all exist on the character sheet but nothing in live combat rolls
  against them yet. The Baaz/Bozak/Sivak/Aurak Draconians' magic
  resistance (a related but different mechanic -- resistance to being
  targeted at all, not a saving throw) still isn't modeled for any of
  them.
- **More monsters**: forty-three creatures are in the roster now (Goblin,
  Kobold, Hobgoblin, Timber Wolf, Giant Spider, Baaz/Kapak/Bozak/Sivak/
  Aurak Draconian, Bugbear, Ogre, Gnoll, Ghoul, Skeleton, Zombie, Thanoi,
  Owlbear, Wight, Troll, Black Bear, Worg, Ice Bear, Lizard Man, Giant
  Toad, Ettin, Harpy, Griffon, Stirge, Giant Rat, Giant Centipede, Ghast,
  Wereboar, Weretiger, Boring Beetle, Gorgon, Hydra, Wraith, White/Brown
  Pudding, Mummy, Spectre, Shambling Mound); `Monster Manual (2nd ed).pdf`
  and *Dragonlance Adventures* still have more of Krynn's actual bestiary
  untouched, though the DLA well (its "Common Creatures of Krynn"
  chapter) is now confirmed dry beyond Ice Bear — see Milestone 112's
  writeup in `docs/MILESTONES.md` for why Dreamshadow/Dreamwraith/Fetch/
  Minotaur/Shadowpeople/Spectral Minion don't fit this project's
  wandering-encounter model. Milestone 142 added the roster's first
  flying/aerial predators (Harpy p.184, Griffon p.178) plus a low-tier
  swarm pest (Stirge p.332). Milestone 156 cross-referenced three
  fan-transcribed Gold Box video-game monster manuals (References/*.html,
  video-game balance numbers, not sourcing on their own) against this
  roster purely as a "what's missing" gap-finder, then added 14 real,
  independently-sourced monsters the cross-reference surfaced — see that
  entry for the full page-citation table, the new danger ceiling it
  introduces (Shambling Mound, `MIN_TOWN_DISTANCE 55`), and why Purple
  Worm/Otyugh/Umber Hulk/Black Pudding/Rhinoceros Beetle/Disir were all
  deliberately left out. All new monsters (both milestones) were visually
  confirmed against rendered Monster Manual page images — see the
  terrain-code honesty notes (no coast/subterranean code exists in this
  engine, so a purely aquatic or purely subterranean real monster can
  never be represented) and gating rationale in each entry. Can be added
  the same way, one more sourced `MONSTER` block at a time. A new monster
  with real terrain flavor can also carry `TERRAIN_BIAS`/`EXCLUDE_TERRAIN`
  lines — see "Terrain-specific monster pools" above. A new low/mid-HD
  monster can also carry a sourced `GROUP <min> <max>` line from the
  start — see "Monster encounter groups" above.
- **The rest of Bozak/Sivak/Aurak's abilities** (Milestone 99 shipped the
  parts that ground out in this engine's real combat math -- Bozak's
  signature Magic Missile, Aurak's breath weapon, Sivak's death-burst; see
  the Draconian roster above): what's left is either a flat stat
  (magic resistance, save bonuses for all three -- would need a
  monster-side resistance-roll/save-bonus mechanic, related to but
  distinct from `rollSavingThrow`) or genuinely doesn't fit this project's
  positionless, no-monster-persistence combat loop: Sivak's shapeshifting
  (no per-instance monster identity or
  NPC-disguise gameplay to hang it on), and Aurak's dimension door,
  suggestion/mind control, change self/polymorph self, at-will
  invisibility, full 1st-4th level spell list, and three-stage death
  sequence (immolation → lightning ball → explosion). Not being pursued
  further absent a concrete reason one of these would newly fit (e.g. a
  future NPC-disguise or monster-status-effect system built for an
  unrelated reason that this could then hang off of).
