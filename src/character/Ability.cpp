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

int hpAdjustmentForConstitution(int constitution) {
    if (constitution <= 3) return -3;
    if (constitution <= 6) return -2;
    if (constitution <= 8) return -1;
    if (constitution <= 14) return 0;
    if (constitution <= 16) return 1;
    if (constitution <= 17) return 2;
    return 3; // 18+
}

int acAdjustmentForDexterity(int dexterity) {
    if (dexterity <= 3) return -3;
    if (dexterity <= 5) return -2;
    if (dexterity <= 6) return -1;
    if (dexterity <= 14) return 0;
    if (dexterity <= 15) return 1;
    if (dexterity <= 16) return 2;
    if (dexterity <= 17) return 3;
    return 4; // 18+
}

} // namespace character
