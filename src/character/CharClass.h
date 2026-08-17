#pragma once

#include "character/Ability.h"

#include <array>
#include <cstddef>

namespace character {

// Core four PHB classes for this milestone -- Paladin, Ranger, Druid,
// Bard, and specialist wizards are deliberately deferred (see
// docs/CHARACTER_NOTES.md).
enum class ClassId {
    Fighter,
    Mage,
    Cleric,
    Thief,
};

enum class SaveCategory {
    ParalyzationPoisonDeath,
    RodStaffWand,
    PetrificationPolymorph,
    BreathWeapon,
    Spell,
    Count, // sentinel for array sizing, not a real category
};

const char* saveCategoryName(SaveCategory c);

// Target numbers to roll at or above on d20 -- lower is better, same
// convention as 2e's actual saving throw tables.
struct SavingThrows {
    std::array<int, static_cast<size_t>(SaveCategory::Count)> values{};

    int& at(SaveCategory c) { return values[static_cast<size_t>(c)]; }
    int at(SaveCategory c) const { return values[static_cast<size_t>(c)]; }
};

struct ClassInfo {
    ClassId id;
    const char* name;
    Ability primeRequisite;
    int primeRequisiteMinimum;
    int hitDieSides;
    SavingThrows level1Saves;
    int goldDiceCount;
    int goldDiceSides;
    int goldFlatBonus;  // added to the dice roll BEFORE multiplying (Table 43's Mage row is
                         // "(1d4+1) x 10", not a flat NdM -- this field exists for that case)
    int goldMultiplier; // starting gold = (roll(goldDiceCount, goldDiceSides) + goldFlatBonus) * goldMultiplier
};

const ClassInfo& classInfo(ClassId id);

constexpr std::array<ClassId, 4> kAllClasses = {
    ClassId::Fighter,
    ClassId::Mage,
    ClassId::Cleric,
    ClassId::Thief,
};

} // namespace character
