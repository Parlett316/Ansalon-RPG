#include "character/Leveling.h"
#include "character/Ability.h"
#include "character/Dice.h"
#include "character/Knighthood.h"
#include "character/Race.h"

#include <algorithm>
#include <array>
#include <limits>

namespace character {

ClassGroup classGroupFor(ClassId id) {
    switch (id) {
        case ClassId::Fighter: return ClassGroup::Warrior;
        case ClassId::Mage: return ClassGroup::Wizard;
        case ClassId::Cleric: return ClassGroup::Priest;
        case ClassId::Thief: return ClassGroup::Rogue;
        case ClassId::Tinker: return ClassGroup::Wizard; // documented analogy -- see Leveling.h
    }
    return ClassGroup::Warrior;
}

int xpThresholdForLevel(ClassId id, int level) {
    if (level <= 1) return 0;

    // PHB Tables 14 (Warrior, p.36), 20 (Wizard, p.42), 23 (Priest, p.47),
    // 25 (Rogue, p.53) -- index 0 is level 1 (always 0 XP). Levels 1-20,
    // the full printed table for each class (this project doesn't model
    // level titles past 20th -- the PHB doesn't print further, single-class
    // thresholds anyway; see docs/CHARACTER_NOTES.md).
    static constexpr std::array<int, 20> kFighterXp = {
        0,       2000,    4000,    8000,    16000,   32000,   64000,   125000,  250000,  500000,
        750000,  1000000, 1250000, 1500000, 1750000, 2000000, 2250000, 2500000, 2750000, 3000000};
    static constexpr std::array<int, 20> kMageXp = {
        0,      2500,   5000,   10000,  20000,  40000,   60000,   90000,   135000,  250000,
        375000, 750000, 1125000, 1500000, 1875000, 2250000, 2625000, 3000000, 3375000, 3750000};
    static constexpr std::array<int, 20> kClericXp = {
        0,      1500,   3000,   6000,   13000,  27500,   55000,   110000,  225000,  450000,
        675000, 900000, 1125000, 1350000, 1575000, 1800000, 2025000, 2250000, 2475000, 2700000};
    static constexpr std::array<int, 20> kThiefXp = {
        0,      1250,   2500,   5000,   10000,  20000,   40000,   70000,   110000,  160000,
        220000, 440000, 660000, 880000, 1100000, 1320000, 1540000, 1760000, 1980000, 2200000};

    if (level > 20) {
        // Not modeled past level 20 -- see docs/CHARACTER_NOTES.md.
        // Returning "unreachable" rather than extrapolating keeps
        // applyPendingLevelUps from ever trying to apply a level this
        // module has no THAC0/saves table for either.
        return std::numeric_limits<int>::max();
    }

    const std::array<int, 20>* table = &kFighterXp;
    switch (id) {
        case ClassId::Fighter: table = &kFighterXp; break;
        case ClassId::Cleric: table = &kClericXp; break;
        case ClassId::Thief: table = &kThiefXp; break;
        case ClassId::Mage:
        case ClassId::Tinker:
            table = &kMageXp;
            break;
    }
    return (*table)[static_cast<size_t>(level - 1)];
}

int thac0ForLevel(ClassId id, int level) {
    // Table 53 (PHB p.121), levels 1-20.
    static constexpr std::array<int, 20> kPriest = {20, 20, 20, 18, 18, 18, 16, 16, 16, 14,
                                                       14, 14, 12, 12, 12, 10, 10, 10, 8,  8};
    static constexpr std::array<int, 20> kRogue = {20, 20, 19, 19, 18, 18, 17, 17, 16, 16,
                                                      15, 15, 14, 14, 13, 13, 12, 12, 11, 11};
    static constexpr std::array<int, 20> kWarrior = {20, 19, 18, 17, 16, 15, 14, 13, 12, 11,
                                                        10, 9,  8,  7,  6,  5,  4,  3,  2,  1};
    static constexpr std::array<int, 20> kWizard = {20, 20, 20, 19, 19, 19, 18, 18, 18, 17,
                                                       17, 17, 16, 16, 16, 15, 15, 15, 14, 14};

    int clampedLevel = std::max(1, std::min(level, 20));
    const std::array<int, 20>* table = &kWarrior;
    switch (classGroupFor(id)) {
        case ClassGroup::Priest: table = &kPriest; break;
        case ClassGroup::Rogue: table = &kRogue; break;
        case ClassGroup::Warrior: table = &kWarrior; break;
        case ClassGroup::Wizard: table = &kWizard; break;
    }
    return (*table)[static_cast<size_t>(clampedLevel - 1)];
}

namespace {

struct SaveBracket {
    int minLevel;
    int values[5]; // SaveCategory order: Para/Poison/Death, Rod/Staff/Wand,
                    // Petrification/Polymorph, Breath Weapon, Spell
};

} // namespace

SavingThrows savesForLevel(ClassId id, int level) {
    // Table 60 (PHB p.134), bracketed by level range.
    static constexpr SaveBracket kPriest[] = {
        {1, {10, 14, 13, 16, 15}}, {4, {9, 13, 12, 15, 14}}, {7, {7, 11, 10, 13, 12}},
        {10, {6, 10, 9, 12, 11}},  {13, {5, 9, 8, 11, 10}},  {16, {4, 8, 7, 10, 9}},
        {19, {2, 6, 5, 8, 7}},
    };
    static constexpr SaveBracket kRogue[] = {
        {1, {13, 14, 12, 16, 15}}, {5, {12, 12, 11, 15, 13}}, {9, {11, 10, 10, 14, 11}},
        {13, {10, 8, 9, 13, 9}},   {17, {9, 6, 8, 12, 7}},    {21, {8, 4, 7, 11, 5}},
    };
    static constexpr SaveBracket kWarrior[] = {
        {1, {14, 16, 15, 17, 17}}, {3, {13, 15, 14, 16, 16}}, {5, {11, 13, 12, 13, 14}},
        {7, {10, 12, 11, 12, 13}}, {9, {8, 10, 9, 9, 11}},    {11, {7, 9, 8, 8, 10}},
        {13, {5, 7, 6, 5, 8}},     {15, {4, 6, 5, 4, 7}},     {17, {3, 5, 4, 4, 6}},
    };
    static constexpr SaveBracket kWizard[] = {
        {1, {14, 11, 13, 15, 12}}, {6, {13, 9, 11, 13, 10}}, {11, {11, 7, 9, 11, 8}},
        {16, {10, 5, 7, 9, 6}},    {21, {8, 3, 5, 7, 4}},
    };

    const SaveBracket* table = kWarrior;
    size_t tableSize = sizeof(kWarrior) / sizeof(kWarrior[0]);
    switch (classGroupFor(id)) {
        case ClassGroup::Priest:
            table = kPriest;
            tableSize = sizeof(kPriest) / sizeof(kPriest[0]);
            break;
        case ClassGroup::Rogue:
            table = kRogue;
            tableSize = sizeof(kRogue) / sizeof(kRogue[0]);
            break;
        case ClassGroup::Warrior:
            table = kWarrior;
            tableSize = sizeof(kWarrior) / sizeof(kWarrior[0]);
            break;
        case ClassGroup::Wizard:
            table = kWizard;
            tableSize = sizeof(kWizard) / sizeof(kWizard[0]);
            break;
    }

    const SaveBracket* chosen = &table[0];
    for (size_t i = 0; i < tableSize; ++i) {
        if (table[i].minLevel <= level) chosen = &table[i];
    }

    SavingThrows result;
    result.at(SaveCategory::ParalyzationPoisonDeath) = chosen->values[0];
    result.at(SaveCategory::RodStaffWand) = chosen->values[1];
    result.at(SaveCategory::PetrificationPolymorph) = chosen->values[2];
    result.at(SaveCategory::BreathWeapon) = chosen->values[3];
    result.at(SaveCategory::Spell) = chosen->values[4];
    return result;
}

namespace {

// Each class group's chapter states this near-verbatim (PHB pp.36, 43, 47,
// 54): hit points are rolled + CON-adjusted through a cutoff level, then a
// flat amount with NO further CON bonus after that.
int hpRollCutoffForGroup(ClassGroup group) {
    switch (group) {
        case ClassGroup::Warrior: return 9;
        case ClassGroup::Priest: return 9;
        case ClassGroup::Wizard: return 10;
        case ClassGroup::Rogue: return 10;
    }
    return 9;
}

int hpFlatGainForGroup(ClassGroup group) {
    switch (group) {
        case ClassGroup::Warrior: return 3;
        case ClassGroup::Priest: return 2;
        case ClassGroup::Wizard: return 1;
        case ClassGroup::Rogue: return 2;
    }
    return 1;
}

} // namespace

int applyPendingLevelUps(Character& character, std::vector<std::string>& messages) {
    ClassGroup group = classGroupFor(character.charClass);
    bool isWarrior = group == ClassGroup::Warrior;
    int rollCutoff = hpRollCutoffForGroup(group);
    int flatGain = hpFlatGainForGroup(group);
    int levelsGained = 0;

    for (;;) {
        int nextLevel = character.level + 1;
        if (character.experience < xpThresholdForLevel(character.charClass, nextLevel)) break;

        // DMG Table 7 / Dragonlance Adventures demihuman class/level limits
        // (see Race.h's classLevelCap): XP keeps accruing past this point,
        // it just stops converting into levels -- matches the sourcebook's
        // own "cannot advance beyond the listed level" verbatim, no need to
        // cap or discard character.experience itself.
        if (nextLevel > classLevelCap(character.race, character.subrace, character.charClass)) break;

        int hpGain;
        if (nextLevel <= rollCutoff) {
            int dieRoll = roll(1, classInfo(character.charClass).hitDieSides);
            hpGain = std::max(1, dieRoll + hpAdjustmentForConstitution(character.scores.constitution, isWarrior));
        } else {
            hpGain = flatGain; // no CON bonus once rolling stops -- the book is explicit about this
        }
        character.maxHp += hpGain;
        character.currentHp += hpGain; // level-ups heal, standard convention

        character.level = nextLevel;
        character.thac0 = thac0ForLevel(character.charClass, nextLevel);
        character.saves = savesForLevel(character.charClass, nextLevel);
        applyRacialSavingThrowBonus(character.race, character.scores.constitution, character.saves);

        messages.push_back("You reach level " + std::to_string(nextLevel) + "! (+" + std::to_string(hpGain) +
                            " HP, THAC0 " + std::to_string(character.thac0) + ")");

        // Flavor-only moments -- see docs/CHARACTER_NOTES.md for why
        // Sword Knights and real Wizard Robe mechanics aren't fully
        // modeled, just marked with a beat at the level the source
        // material says something would happen.
        if (nextLevel == 3 && character.charClass == ClassId::Fighter &&
            character.knightOrder == KnightOrder::Crown) {
            messages.push_back("Word reaches you that the Order of the Sword has taken notice of your deeds.");
        }
        if (nextLevel == 4 && character.charClass == ClassId::Fighter &&
            character.knightOrder == KnightOrder::Sword) {
            messages.push_back("Word reaches you that the Order of the Rose has taken notice of your deeds.");
        }
        // No longer an automatic event -- as of the Wayreth quest (see
        // docs/CHARACTER_NOTES.md's "Wizards of High Sorcery"), the actual
        // Test and RobeColor assignment happen on turning in
        // wayreth_summons (GameLoop.cpp's REWARD_WAYRETH_ROBE handling),
        // not here. character.robeColor stays RobeColor::None until then.
        if (nextLevel == 3 && character.charClass == ClassId::Mage) {
            messages.push_back("You feel, faintly, that something has taken notice of you.");
        }

        ++levelsGained;
    }
    return levelsGained;
}

int meleeAttacksThisRound(ClassId id, int level, int roundNumber) {
    if (classGroupFor(id) != ClassGroup::Warrior) return 1;
    if (level >= 13) return 2;
    if (level >= 7) return (roundNumber % 2 == 0) ? 2 : 1;
    return 1;
}

int backstabDamageMultiplier(int level) {
    if (level >= 13) return 5;
    if (level >= 9) return 4;
    if (level >= 5) return 3;
    return 2;
}

} // namespace character
