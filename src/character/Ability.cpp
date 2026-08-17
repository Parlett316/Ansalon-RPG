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

} // namespace character
