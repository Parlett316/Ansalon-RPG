#include "character/Ability.h"

namespace character {

int AbilityScores::get(Ability a) const {
    switch (a) {
        case Ability::Strength: return strength;
        case Ability::Dexterity: return dexterity;
        case Ability::Constitution: return constitution;
        case Ability::Intelligence: return intelligence;
        case Ability::Wisdom: return wisdom;
        case Ability::Charisma: return charisma;
    }
    return 0;
}

void AbilityScores::adjust(Ability a, int delta) {
    switch (a) {
        case Ability::Strength: strength += delta; break;
        case Ability::Dexterity: dexterity += delta; break;
        case Ability::Constitution: constitution += delta; break;
        case Ability::Intelligence: intelligence += delta; break;
        case Ability::Wisdom: wisdom += delta; break;
        case Ability::Charisma: charisma += delta; break;
    }
}

const char* abilityName(Ability a) {
    switch (a) {
        case Ability::Strength: return "Strength";
        case Ability::Dexterity: return "Dexterity";
        case Ability::Constitution: return "Constitution";
        case Ability::Intelligence: return "Intelligence";
        case Ability::Wisdom: return "Wisdom";
        case Ability::Charisma: return "Charisma";
    }
    return "";
}

int hpAdjustmentForConstitution(int constitution, bool isWarrior) {
    if (constitution <= 3) return -2;
    if (constitution <= 6) return -1;
    if (constitution <= 14) return 0;
    if (constitution == 15) return 1;
    if (constitution == 16) return 2;
    if (constitution == 17) return isWarrior ? 3 : 2;
    return isWarrior ? 4 : 2; // 18+
}

int acAdjustmentForDexterity(int dexterity) {
    if (dexterity <= 3) return -4;
    if (dexterity == 4) return -3;
    if (dexterity == 5) return -2;
    if (dexterity == 6) return -1;
    if (dexterity <= 14) return 0;
    if (dexterity == 15) return 1;
    if (dexterity == 16) return 2;
    if (dexterity == 17) return 3;
    return 4; // 18+
}

int constitutionMagicResistanceBonus(int constitution) {
    if (constitution < 4) return 0;
    if (constitution <= 6) return 1;
    if (constitution <= 10) return 2;
    if (constitution <= 13) return 3;
    if (constitution <= 17) return 4;
    return 5; // 18-19
}

int strengthToHitAdjustment(int strength, int exceptionalPercentile) {
    if (strength <= 3) return -3;
    if (strength <= 5) return -2;
    if (strength <= 7) return -1;
    if (strength <= 16) return 0;
    if (strength == 17) return 1;
    // strength == 18 (or, defensively, higher -- not reachable via this
    // project's 3d6 generation, but the 18/00 row is a sane fallback).
    if (exceptionalPercentile <= 0) return 1;    // flat 18, no percentile rolled
    if (exceptionalPercentile <= 50) return 1;   // 18/01-18/50
    if (exceptionalPercentile <= 99) return 2;   // 18/51-18/99
    return 3;                                    // 18/00
}

int strengthDamageAdjustment(int strength, int exceptionalPercentile) {
    if (strength <= 5) return -1;
    if (strength <= 15) return 0;
    if (strength <= 17) return 1;
    if (exceptionalPercentile <= 0) return 2;    // flat 18
    if (exceptionalPercentile <= 50) return 3;   // 18/01-18/50
    if (exceptionalPercentile <= 75) return 3;   // 18/51-18/75
    if (exceptionalPercentile <= 90) return 4;   // 18/76-18/90
    if (exceptionalPercentile <= 99) return 5;   // 18/91-18/99
    return 6;                                    // 18/00
}

} // namespace character
