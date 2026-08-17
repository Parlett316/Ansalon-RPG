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

// Verified directly against a scanned copy of the 2e Player's Handbook
// (revised): prime requisites/hit dice/ability minimums against each
// class's own page (Fighter p.36, Mage p.44, Cleric p.48, Thief p.54),
// saving throws against Table 60 (p.134, grouped by Warrior/Wizard/
// Priest/Rogue -- our four classes map 1:1 onto those groups), starting
// gold against Table 43 (p.89). The only errors this verification found
// (now fixed) were the starting-gold dice for Mage/Cleric/Thief -- see
// docs/CHARACTER_NOTES.md for the full list of what was checked.
const std::array<ClassInfo, 4> kTable = {{
    {ClassId::Fighter, "Fighter", Ability::Strength, 9, 10,
     makeSaves(14, 16, 15, 17, 17), 5, 4, 0, 10},   // 5d4 x10
    {ClassId::Mage, "Mage", Ability::Intelligence, 9, 4,
     makeSaves(14, 11, 13, 15, 12), 1, 4, 1, 10},   // (1d4+1) x10
    {ClassId::Cleric, "Cleric", Ability::Wisdom, 9, 8,
     makeSaves(10, 14, 13, 16, 15), 3, 6, 0, 10},   // 3d6 x10
    {ClassId::Thief, "Thief", Ability::Dexterity, 9, 6,
     makeSaves(13, 14, 12, 16, 15), 2, 6, 0, 10},   // 2d6 x10
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
