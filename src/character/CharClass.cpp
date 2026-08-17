#include "character/CharClass.h"

#include <array>

namespace character {

namespace {

SavingThrows makeSaves(int paralyzationPoisonDeath, int rodStaffWand, int petrificationPolymorph,
                        int breathWeapon, int spell) {
    SavingThrows s;
    s.at(SaveCategory::ParalyzationPoisonDeath) = paralyzationPoisonDeath;
    s.at(SaveCategory::RodStaffWand) = rodStaffWand;
    s.at(SaveCategory::PetrificationPolymorph) = petrificationPolymorph;
    s.at(SaveCategory::BreathWeapon) = breathWeapon;
    s.at(SaveCategory::Spell) = spell;
    return s;
}

// Level-1 saving throw target numbers and starting gold are best-effort
// from memory of the 2e PHB (roughly Tables 46-49 for saves, the "Starting
// Gold" entries in each class description) -- not copied from the book
// directly. Spot-check against your own copy; each row here is the one
// place to correct if a number is off.
const std::array<ClassInfo, 4> kTable = {{
    {ClassId::Fighter, "Fighter", Ability::Strength, 9, 10,
     makeSaves(14, 16, 15, 17, 17), 5, 4, 10},
    {ClassId::Mage, "Mage", Ability::Intelligence, 9, 4,
     makeSaves(14, 11, 13, 15, 12), 2, 4, 10},
    {ClassId::Cleric, "Cleric", Ability::Wisdom, 9, 8,
     makeSaves(10, 14, 13, 16, 15), 3, 4, 10},
    {ClassId::Thief, "Thief", Ability::Dexterity, 9, 6,
     makeSaves(13, 14, 12, 16, 15), 2, 4, 10},
}};

} // namespace

const char* saveCategoryName(SaveCategory c) {
    switch (c) {
        case SaveCategory::ParalyzationPoisonDeath: return "Paralyzation/Poison/Death";
        case SaveCategory::RodStaffWand: return "Rod/Staff/Wand";
        case SaveCategory::PetrificationPolymorph: return "Petrification/Polymorph";
        case SaveCategory::BreathWeapon: return "Breath Weapon";
        case SaveCategory::Spell: return "Spell";
        case SaveCategory::Count: return "";
    }
    return "";
}

const ClassInfo& classInfo(ClassId id) {
    for (const auto& info : kTable) {
        if (info.id == id) return info;
    }
    return kTable[0]; // unreachable given ClassId only has the 4 values above
}

} // namespace character
