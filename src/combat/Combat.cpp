#include "combat/Combat.h"
#include "character/Ability.h"
#include "character/Dice.h"

#include <algorithm>

namespace combat {

AttackOutcome resolvePlayerAttack(const character::Character& character, const Monster& monster) {
    int strToHit = character::strengthToHitAdjustment(character.scores.strength,
                                                        character.exceptionalStrengthPercentile);

    int naturalRoll = character::roll(1, 20);
    bool hit;
    if (naturalRoll == 20) {
        hit = true;
    } else if (naturalRoll == 1) {
        hit = false;
    } else {
        hit = (naturalRoll + strToHit + character.weaponMagicBonus) >= (character.thac0 - monster.armorClass);
    }

    AttackOutcome outcome;
    outcome.hit = hit;
    if (hit) {
        int strDamage = character::strengthDamageAdjustment(character.scores.strength,
                                                              character.exceptionalStrengthPercentile);
        outcome.damage = std::max(1, character::roll(1, character.weaponDamageSides) +
                                          character.weaponDamageBonus + strDamage + character.weaponMagicBonus);
    }
    return outcome;
}

AttackOutcome resolveMonsterAttack(const Monster& monster, const character::Character& character) {
    int naturalRoll = character::roll(1, 20);
    bool hit;
    if (naturalRoll == 20) {
        hit = true;
    } else if (naturalRoll == 1) {
        hit = false;
    } else {
        hit = naturalRoll >= (monster.thac0 - character.armorClass);
    }

    AttackOutcome outcome;
    outcome.hit = hit;
    if (hit) {
        int damage = character::roll(monster.damageDiceCount, monster.damageDiceSides) + monster.damageFlatBonus;
        outcome.damage = std::max(1, damage);
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
