#include "combat/Combat.h"
#include "character/Ability.h"
#include "character/Dice.h"

#include <algorithm>

namespace combat {

AttackOutcome resolvePlayerAttack(const character::Character& character, const Monster& monster,
                                   int thac0Bonus, int damageBonus, int damageMultiplier,
                                   int monsterAcPenalty) {
    int strToHit = character::strengthToHitAdjustment(character.scores.strength,
                                                        character.exceptionalStrengthPercentile);

    int naturalRoll = character::roll(1, 20);
    int toHitBonus = strToHit + character.weaponMagicBonus + thac0Bonus;
    // monsterAcPenalty (Slow, PHB p.196) worsens the monster's own AC number
    // -- adding, not subtracting, matches the "penalty" naming convention
    // resolveMonsterAttack's thac0Penalty/damagePenalty already use below.
    int defenderArmorClass = monster.armorClass + monsterAcPenalty;
    int targetNumber = character.thac0 - defenderArmorClass;

    bool hit;
    if (naturalRoll == 20) {
        hit = true;
    } else if (naturalRoll == 1) {
        hit = false;
    } else {
        hit = (naturalRoll + toHitBonus) >= targetNumber;
    }

    AttackOutcome outcome;
    outcome.hit = hit;
    outcome.naturalRoll = naturalRoll;
    outcome.toHitBonus = toHitBonus;
    outcome.attackerThac0 = character.thac0;
    outcome.defenderArmorClass = defenderArmorClass;
    outcome.targetNumber = targetNumber;
    if (hit) {
        int strDamage = character::strengthDamageAdjustment(character.scores.strength,
                                                              character.exceptionalStrengthPercentile);
        outcome.damageDiceCount = 1;
        outcome.damageDiceSides = character.weaponDamageSides;
        outcome.damageMultiplier = damageMultiplier;
        outcome.damageRoll = character::roll(1, character.weaponDamageSides) * damageMultiplier;
        outcome.damageBonus = character.weaponDamageBonus + strDamage + character.weaponMagicBonus + damageBonus;
        outcome.damage = std::max(1, outcome.damageRoll + outcome.damageBonus);
    }
    return outcome;
}

AttackOutcome resolveMonsterAttack(const Monster& monster, const character::Character& character,
                                    int acBonus, int thac0Penalty, int damagePenalty) {
    int naturalRoll = character::roll(1, 20);
    // thac0Penalty makes the monster a worse attacker (effectively raises
    // its THAC0, the same direction a real THAC0 penalty works), not a
    // bonus to the player's roll -- folded into attackerThac0 rather than a
    // separate toHitBonus field, since the monster's own to-hit math has no
    // additive term on the die roll itself (unlike the player's STR/magic
    // bonus), only a shifted target number.
    int attackerThac0 = monster.thac0 + thac0Penalty;
    int defenderArmorClass = character.armorClass - acBonus;
    int targetNumber = attackerThac0 - defenderArmorClass;

    bool hit;
    if (naturalRoll == 20) {
        hit = true;
    } else if (naturalRoll == 1) {
        hit = false;
    } else {
        hit = naturalRoll >= targetNumber;
    }

    AttackOutcome outcome;
    outcome.hit = hit;
    outcome.naturalRoll = naturalRoll;
    outcome.attackerThac0 = attackerThac0;
    outcome.defenderArmorClass = defenderArmorClass;
    outcome.targetNumber = targetNumber;
    if (hit) {
        outcome.damageDiceCount = monster.damageDiceCount;
        outcome.damageDiceSides = monster.damageDiceSides;
        outcome.damageRoll = character::roll(monster.damageDiceCount, monster.damageDiceSides);
        outcome.damageBonus = monster.damageFlatBonus - damagePenalty;
        outcome.damage = std::max(1, outcome.damageRoll + outcome.damageBonus);
    }
    return outcome;
}

bool rollSavingThrow(const character::Character& character, character::SaveCategory category) {
    return character::roll(1, 20) >= character.saves.at(category);
}

bool playerActsFirst() {
    // Re-roll on a tie rather than modeling PHB's "simultaneous" resolution
    // -- this project's round loop is strictly ordered (one side attacks,
    // then the other, so the second attacker can be skipped if the first
    // already won), which has no way to represent "both happen at once."
    for (;;) {
        int playerRoll = character::roll(1, 10);
        int monsterRoll = character::roll(1, 10);
        if (playerRoll != monsterRoll) return playerRoll < monsterRoll;
    }
}

} // namespace combat
