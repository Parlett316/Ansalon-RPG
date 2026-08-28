#pragma once

#include "character/Character.h"
#include "combat/Monster.h"

namespace combat {

struct AttackOutcome {
    bool hit = false;
    int damage = 0; // only meaningful if hit

    // Roll breakdown behind hit/damage above, always populated (even on a
    // miss -- damageRoll/damageBonus/damage just stay 0 in that case) so a
    // caller can show the real PHB math instead of only the result -- see
    // docs/COMBAT_NOTES.md's "Showing the math" section and
    // game::describeToHit/describeDamage in GameLoop.cpp.
    int naturalRoll = 0;        // unmodified 1d20 -- natural 20 always hits, natural 1 always misses (PHB p.121)
    int toHitBonus = 0;         // every point added to naturalRoll before comparing to targetNumber
    int attackerThac0 = 0;      // attacker's THAC0 used for this roll (may include a this-fight penalty)
    int defenderArmorClass = 0; // defender's AC used for this roll (may include a this-fight bonus)
    int targetNumber = 0;       // attackerThac0 - defenderArmorClass -- naturalRoll+toHitBonus must reach this
    int damageDiceCount = 0;
    int damageDiceSides = 0;
    int damageRoll = 0; // raw dice roll, before bonuses -- already includes damageMultiplier below
    int damageBonus = 0; // every point added to damageRoll
    // Thief backstab (PHB Table 30, p.57): the raw weapon die roll is
    // multiplied BEFORE Strength/magic bonuses are added -- 1 for every
    // ordinary attack. See resolvePlayerAttack's damageMultiplier
    // parameter and game::describeDamage, which renders this only when
    // != 1 so the log never misrepresents which die was actually rolled.
    int damageMultiplier = 1;
};

// PHB p.119/p.121: roll 1d20 + attacker's to-hit adjustment; hits if the
// total >= (attacker's THAC0 - target's AC). A natural 20 always hits and
// a natural 1 always misses, checked against the UNMODIFIED die roll, "regardless
// of any modifiers." Damage is floored at 1 on a hit (a near-universal
// convention, not itself re-verified this pass -- see docs/COMBAT_NOTES.md).
// character::Character::weaponMagicBonus (a "+1" enchanted weapon, see
// character/Equipment.h's MagicWeapon) adds to both the to-hit total and
// the damage roll -- real 2e convention, applied on top of Strength.
//
// thac0Bonus/damageBonus (player) and acBonus/thac0Penalty/damagePenalty
// (monster) are this-fight-only spell buffs/debuffs (Bless, Prayer,
// Protection from Evil, Strength, Slow, Bestow Curse, ... -- see
// character/Spellcasting.h), held as local variables in
// game::GameLoop::runCombat and never written into the character's real
// saved armorClass/thac0 -- same "local to this one runCombat call, doesn't
// survive to the save file" precedent Webnet/Brooch of Imog already
// established (docs/CHARACTER_NOTES.md's "Magic items").
// damageMultiplier (Milestone 119, thief backstab): multiplies the raw
// weapon die roll before strDamage/weaponDamageBonus/weaponMagicBonus/
// damageBonus are added, per PHB Table 30 (p.57): "The weapon's standard
// damage is multiplied by the value given in Table 30. Then Strength and
// magical weapon bonuses are added." A plain 1 for every ordinary attack.
AttackOutcome resolvePlayerAttack(const character::Character& character, const Monster& monster,
                                   int thac0Bonus = 0, int damageBonus = 0, int damageMultiplier = 1);
AttackOutcome resolveMonsterAttack(const Monster& monster, const character::Character& character,
                                    int acBonus = 0, int thac0Penalty = 0, int damagePenalty = 0);

// PHB p.124: one d10 per side, lower roll acts first. Ties are rare enough
// (both parties would need to act "simultaneously," which this project's
// strictly-turn-ordered round loop can't represent) that this just
// re-rolls until there's a winner -- see docs/COMBAT_NOTES.md.
bool playerActsFirst();

// Standard 2e save convention: roll 1d20, succeed if the roll is >= the
// character's saving throw number for that category (a lower number is
// better/easier, same "at or above" convention as character::SavingThrows
// itself -- see CharClass.h). See docs/COMBAT_NOTES.md for what currently
// calls this (the Giant Spider's poison bite).
bool rollSavingThrow(const character::Character& character, character::SaveCategory category);

} // namespace combat
