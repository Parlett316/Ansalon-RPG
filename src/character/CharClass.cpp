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
// gold against Table 43 (p.89) -- the PHB's own generic-D&D table calls it
// gold; this project applies the same dice as starting Steel Pieces
// instead, per Dragonlance's own currency (see docs/CHARACTER_NOTES.md's
// "Gold -> Steel" note). The only errors this verification found (now
// fixed) were the starting-currency dice for Mage/Cleric/Thief -- see
// docs/CHARACTER_NOTES.md for the full list of what was checked.
//
// Tinker (Dragonlance Adventures, TSR 2021, pp.22-23/printed pp.21-22):
// prime requisite Intelligence 10 and hit die d4 are confirmed directly
// from the book's own "Gnome Advancement Table." Saving throws and
// starting gold are NOT printed anywhere in that book -- confirmed absent
// by a targeted research pass (the book's own front matter states it
// deliberately avoids duplicating PHB/DMG material) -- so this table
// reuses Mage's numbers as a documented analogy: same d4 hit die, same
// Intelligence prime requisite, same fragile-caster archetype (renamed to
// Steel like everything else -- see docs/CHARACTER_NOTES.md). See
// docs/CHARACTER_NOTES.md for the full citation and what's still missing
// (the actual device-building subsystem -- a real, sizable set of tables
// this project doesn't implement yet).
const std::array<ClassInfo, 5> kTable = {{
    {ClassId::Fighter, "Fighter", Ability::Strength, 9, 10,
     makeSaves(14, 16, 15, 17, 17), 5, 4, 0, 10,    // 5d4 x10
     "longsword", 8},
    {ClassId::Mage, "Mage", Ability::Intelligence, 9, 4,
     makeSaves(14, 11, 13, 15, 12), 1, 4, 1, 10,    // (1d4+1) x10
     "dagger", 4},
    {ClassId::Cleric, "Cleric", Ability::Wisdom, 9, 8,
     makeSaves(10, 14, 13, 16, 15), 3, 6, 0, 10,    // 3d6 x10
     "mace", 6},
    {ClassId::Thief, "Thief", Ability::Dexterity, 9, 6,
     makeSaves(13, 14, 12, 16, 15), 2, 6, 0, 10,    // 2d6 x10
     "shortsword", 6},
    {ClassId::Tinker, "Tinker", Ability::Intelligence, 10, 4,
     makeSaves(14, 11, 13, 15, 12), 1, 4, 1, 10,    // saves/steel: Mage analogy, see above
     "well-worn wrench", 4},
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
    return kTable[0]; // unreachable given ClassId only has the values above
}

} // namespace character
