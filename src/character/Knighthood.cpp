#include "character/Knighthood.h"

namespace character {

const char* knightOrderName(KnightOrder order) {
    switch (order) {
        case KnightOrder::Crown: return "Knight of the Crown";
        case KnightOrder::Sword: return "Knight of the Sword";
        case KnightOrder::Rose: return "Knight of the Rose";
        case KnightOrder::None: return "";
    }
    return "";
}

namespace {

bool isGoodAlignment(Alignment a) {
    return a == Alignment::LawfulGood || a == Alignment::NeutralGood || a == Alignment::ChaoticGood;
}

} // namespace

bool meetsKnightOfCrownRequirements(RaceId race, SubraceId /*subrace*/, const AbilityScores& scores,
                                     Alignment alignment) {
    // Every Elf and Dwarf subrace researched is excluded from the
    // Knighthood in every class-limit table checked (p.60-69) -- checking
    // the parent race covers this without needing per-subrace logic, since
    // both races require a subrace selection in this project (see
    // docs/CHARACTER_NOTES.md).
    if (race == RaceId::Elf || race == RaceId::Dwarf) return false;

    if (!isGoodAlignment(alignment)) return false;

    return scores.strength >= 10 && scores.intelligence >= 7 && scores.wisdom >= 10 &&
           scores.dexterity >= 8 && scores.constitution >= 10;
}

bool meetsKnightOfSwordRequirements(const AbilityScores& scores) {
    return scores.strength >= 12 && scores.intelligence >= 9 && scores.wisdom >= 13 &&
           scores.dexterity >= 9 && scores.constitution >= 10;
}

bool meetsKnightOfRoseRequirements(const AbilityScores& scores) {
    return scores.strength >= 15 && scores.intelligence >= 10 && scores.wisdom >= 13 &&
           scores.dexterity >= 12 && scores.constitution >= 15;
}

} // namespace character
