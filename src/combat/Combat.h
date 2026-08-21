#pragma once

#include "character/Character.h"
#include "combat/Monster.h"

namespace combat {

struct AttackOutcome {
    bool hit = false;
    int damage = 0; // only meaningful if hit
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
AttackOutcome resolvePlayerAttack(const character::Character& character, const Monster& monster,
                                   int thac0Bonus = 0, int damageBonus = 0);
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
