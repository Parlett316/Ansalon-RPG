#pragma once

#include "character/Ability.h"

#include <array>
#include <cstddef>

namespace character {

// Core four PHB classes for this milestone -- Paladin, Ranger, Druid,
// Bard, and specialist wizards are deliberately deferred (see
// docs/CHARACTER_NOTES.md). Tinker is the exception: not a core PHB class,
// but Dragonlance Adventures (TSR 2021) makes it THE Gnome class -- forced
// automatically onto every Gnome PC in CharacterCreator rather than offered
// as a menu choice (see docs/CHARACTER_NOTES.md). Added at the end, after
// Thief, so existing SUBRACE/CLASS ints in old save.txt files keep meaning
// what they meant (see docs/GOTCHAS.md on raw-enum-int save fragility).
enum class ClassId {
    Fighter,
    Mage,
    Cleric,
    Thief,
    Tinker,
};

// Total number of ClassId values, INCLUDING Tinker -- used where the full
// enum range matters (e.g. SaveGame's bounds check), as opposed to
// kAllClasses below (the core four offered in the ordinary class menu;
// Tinker is deliberately excluded from it, see above).
constexpr int kClassIdCount = 5;

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
    int steelDiceCount;
    int steelDiceSides;
    int steelFlatBonus;  // added to the dice roll BEFORE multiplying (Table 43's Mage row is
                          // "(1d4+1) x 10", not a flat NdM -- this field exists for that case)
    int steelMultiplier; // starting steel = (roll(steelDiceCount, steelDiceSides) + steelFlatBonus) * steelMultiplier

    // Assumed starting weapon -- a placeholder pending a real equipment/
    // inventory system (see docs/CHARACTER_NOTES.md and docs/COMBAT_NOTES.md),
    // same spirit as Character::armorClass always assuming unarmored.
    // Damage = roll(1, weaponDamageSides) + character::strengthDamageAdjustment(...).
    const char* weaponName;
    int weaponDamageSides;
};

const ClassInfo& classInfo(ClassId id);

constexpr std::array<ClassId, 4> kAllClasses = {
    ClassId::Fighter,
    ClassId::Mage,
    ClassId::Cleric,
    ClassId::Thief,
};

} // namespace character
