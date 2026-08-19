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
  side attacks, then the other, with the second attacker skipped if the
  first already ended the fight), which has no way to represent two
  things happening at once. See `combat::playerActsFirst`.
- **One attack per round at 1st level**: confirmed for Fighter via Table
  15 (Warriors); Cleric/Mage/Thief have no equivalent table in the PHB at
  all, which — combined with the text scoping multiple attacks to
  "warriors" — supports 1/round for them too in the base rules.

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

- **Goblin** (p.166): HD 1-1, AC 6, THAC0 20, 1d6 damage.
- **Kobold** (p.217): HD 1/2 (1-4 hp), AC 7, THAC0 20, 1d4 damage.
- **Hobgoblin** (p.194): HD 1+1, AC 5, THAC0 19. Damage is "by weapon" in
  the book (no fixed die); 1d8 was picked as a reasonable stand-in given
  their typical loadout (polearm/morningstar/sword).
- **Timber Wolf** ("Wolf," p.365): HD 3, AC 7, THAC0 18, damage 2-5 (1d4+1).
- **Giant Spider** (p.329): HD 3+3, AC 4, THAC0 17, bite 1d8, **Type F
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
- **Gnoll** (p.161): HD 2, AC 5, THAC0 19, damage 2-8 (2d4) — "by weapon"
  in the book, same monster-side-weapon simplification as Hobgoblin/
  Bugbear/Ogre. XP 35.
- **Ghoul** (p.134): HD 2, AC 6, THAC0 19, XP 175. Real attack is three
  hits (claw/claw/bite, 1-3/1-3/1-4-or-6 — the last die was genuinely
  ambiguous across both extractions of the scan, reading as either 1-4
  or 1-6 depending on which of the page's parallel monster columns is
  checked) — simplified to a single 1d6 hit, same "one representative
  die" treatment as Baaz's two claws. Real Ghouls also paralyze on a hit
  (save vs. paralyzation or be unable to act) — left unmodeled, same
  restraint as the Kapak's paralysis-poison bite above (no status-effect
  system exists for anyone yet).
- **Skeleton** (p.318, the base "Skeleton" column on that page — not the
  Animal or Monster skeleton variants sharing it): HD 1, AC 7, THAC0 19,
  damage 1-6 (weapon), XP 65. Real Skeletons take half damage from
  edged/piercing weapons and are immune to sleep/charm/hold/cold/fear —
  no per-monster damage-type or immunity mechanic exists in this project
  (same restraint as Baaz's unmodeled magic resistance), so it's flavor
  text in `DESC` only.
- **Zombie** (p.376, the base "Common" column — not the Monster/Ju-ju/
  Lord/Sea variants sharing it): HD 2, AC 8, THAC0 19, damage 1-8, XP 65.
  Real Zombies are immune to sleep/charm/hold/death-magic/poison/cold,
  same flavor-only treatment as the Skeleton's immunities above.

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
not just enough to stand) and they wake up at the **nearest town**
(`GameLoop::nearestTown()`, straight-line tile distance from
`world::Location::isTown` entries -- see below) rather than the run
ending. There's no "you have died" screen, no save deletion, nothing
punitive beyond the trip back to civilization.

**Which locations count as a "town"**: `world::Location` has a `bool
isTown` flag, set via an optional `TOWN` line in a `data/locations.txt`
block (see `docs/MAP_NOTES.md`). Five locations carry it -- Solace,
Haven, Kalaman, Tarsis, Palanthas -- the ones already tagged with a
civilian-settlement `TERRAIN` (forest-town/plains-town/coastal-town/dry-
plains-city/walled-port-city). Fortresses (Pax Tharkas, High Clerist's
Tower), ruins (Xak Tsaroth, Ice Wall), the nomadic Plains of Dust
village, and the elven homelands (Qualinesti/Silvanesti -- Silvanesti is
sealed to outsiders, a deliberate lore exclusion, not an oversight) are
not towns for this purpose. `nearestTown()` picks whichever `isTown`
location is closest by straight-line tile distance to where the player
fell -- no pathfinding system exists in this project, same restraint
already applied to `hoursToCross` being flat-per-tile -- falling back to
Solace only if no town is found at all (defensive; can't happen with the
current data).

## Encounters: a per-terrain chance while traveling

`GameLoop::tryMoveOverworld`, after a successful move: if the destination
tile has no `Location` (named places/towns stay safe), a chance rolls a
random monster from `combat::MonsterCatalog` and starts `runCombat`. As of
Milestone 27 the chance varies by terrain — `world::TerrainInfo` gained an
`encounterChancePercent` field alongside the existing `hoursToCross`,
filled in per terrain in `Terrain.cpp`'s `kTable` (roads safest at 2%,
mountains/forest riskiest at 12%/11%; ocean/Blood Sea/uncharted are 0,
though they're impassable anyway so it never gets checked). Like
`hoursToCross`, these numbers are tuned for pacing, not sourced from
anything. Monster *selection* is still uniform-random regardless of
terrain — terrain-specific monster pools (only spiders in forest, only
draconians near a Highlord garrison, etc.) is a natural follow-up, not
built yet.

## Player actions: Attack, Cast, Drink a Potion, and Flee

`render::Key::Flee` (`'f'`/`'F'`) ends the encounter immediately with a
retreat message, no cost or risk modeled (no PHB-style "opportunity attack
while fleeing" — a deliberate simplification). `Enter` (reused from its
existing "step in/interact" meaning elsewhere) resolves one full round as
a weapon attack.

**`render::Key::Cast` (`'m'`/`'M'`)**, new alongside the leveling-system
spellcasting work (`character::Spellcasting`, see
`docs/CHARACTER_NOTES.md`): a Mage or Cleric can spend their round casting
their one known spell instead of swinging their weapon. Pressing it
validates `character::canCastSpells` and `hasSpellSlotAvailable` first —
on failure (wrong class, no slots left today, or a racially-blocked Mage
whose slot count is always 0) it logs a message and does **not** consume
the round, same forgiving UX as any other out-of-context key press. On
success, `GameLoop::runCombat` swaps a `playerCasts()` lambda in for
`playerAttacks()` inside the exact same `playerActsFirst()`-ordered
exchange — Magic Missile damages the monster, Cure Light Wounds heals the
caster and never touches the monster, but either way the monster still
gets its own attack afterward per the usual initiative ordering. Thief,
Fighter, and Tinker never see the `m=cast` option at all (`drawCombatFrame`
only shows it for `canCastSpells` classes).

**`render::Key::Inventory` (`'i'`), reinterpreted locally as "drink a
potion" (Milestone 42)**, new alongside the Potion of Healing item (see
`docs/CHARACTER_NOTES.md`'s "Potions"): same "swap a lambda in for
`playerAttacks()`" shape as `Cast` above, drinking
`character::firstPotionIndex`'s potion instead of casting a spell. No
potion carried logs a message and doesn't consume the round, same
forgiving pattern as an unavailable `Cast`. This is the same "local key
reinterpretation instead of a new `Key` value" trick `handleShop` already
uses for this exact key (there it toggles the buy/sell view) — outside
combat `'i'` still opens the real inventory screen
(`GameLoop::handleInventory`); only inside `runCombat`'s own nested loop
does it mean "drink." `drawCombatFrame`'s footer only shows
`i=drink potion` when one is actually carried, same "only hint what's
usable" treatment `m=cast` already gets for non-casters.

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

## Rendering

`render::MapRenderer::drawCombatFrame` (new) shows both combatants'
HP/AC and a scrolling log (last 12 lines — older entries fall off, same
"most recent last" convention as a chat log), followed by the two
available actions. Monster HP is tracked as a local `int` inside
`runCombat`, not stored on `combat::Monster` itself — the `Monster` struct
is static content shared by every encounter with that monster type, the
same reasoning `Character` doesn't store per-encounter runtime state
either.

## Extending this later

- **Spellcasting past one known spell each**: Mage/Cleric now cast a real,
  sourced spell in combat (Magic Missile / Cure Light Wounds — see
  `docs/CHARACTER_NOTES.md`); a real spellbook/spell-selection system and
  spells above 1st level are still future work.
- **Equipment past the General Store's short list**: armor tiers/weapon
  upgrades exist now (see `docs/CHARACTER_NOTES.md`'s "Equipment"
  section), but there's still no carried-item inventory, no selling gear
  back, and only one shop (Solace's General Store).
- **Fighter's extra attacks per round** (level 7+, PHB Table 15): the
  round loop resolves exactly one attack per side; a second attacker
  needs the loop restructured. THAC0 diverging by level is already done
  (`docs/CHARACTER_NOTES.md`).
- **Saving throws for effects other than poison**: `rollSavingThrow` is
  general-purpose, but the Giant Spider's poison bite is still the only
  thing that calls it (see "Saving throws in combat" above). The Baaz
  Draconian's 20% magic resistance is a related but different mechanic
  (resistance to being targeted at all, not a saving throw) and still
  isn't modeled.
- **More monsters**: thirteen creatures are in the roster now (Goblin,
  Kobold, Hobgoblin, Timber Wolf, Giant Spider, Baaz/Kapak Draconian,
  Bugbear, Ogre, Gnoll, Ghoul, Skeleton, Zombie); `Monster Manual (2nd
  ed).pdf` and *Dragonlance Adventures* have far more of Krynn's actual
  bestiary still untouched (Bozak/Sivak/Aurak Draconians — the
  higher-tier ones are spellcasters or shapeshifters, real mechanics this
  project doesn't model yet — plus other ordinary Monstrous Manual
  entries). Can be added the same way, one more sourced `MONSTER` block
  at a time.
- **Terrain-specific monster pools**: `combat::MonsterCatalog::randomMonster`
  is still uniform-random regardless of which terrain triggered the
  encounter, even though the chance of an encounter now varies by terrain
  (see "Encounters" above) — pairing monster *type* to terrain (spiders in
  forest, draconian patrols near Highlord-held ground) is a natural
  follow-up, not built yet.
