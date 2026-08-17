#include "character/Race.h"

#include <array>

namespace character {

namespace {

AbilityAdjustment adj(Ability a, int delta) { return AbilityAdjustment{a, delta}; }
AbilityAdjustment noAdj() { return AbilityAdjustment{Ability::Strength, 0}; }

// Ability adjustments verified against a scanned copy of the 2e Player's
// Handbook (revised), Table 8 (p.27): Dwarf, Elf, Gnome, Halfling matched
// what was already here exactly; Human and Half-Elf get no adjustments
// (also confirmed -- they simply don't appear in Table 8). Kender's
// adjustments (STR-1, DEX+2) and "all standard halfling abilities" note
// are from Dragonlance Adventures (TSR 2021), p.53 -- see
// docs/CHARACTER_NOTES.md for why Kender replaces Half-Orc here.
const std::array<RaceInfo, 7> kTable = {{
    {RaceId::Human, "Human",
     {noAdj(), noAdj(), noAdj()},
     false, false, true},

    {RaceId::Dwarf, "Dwarf",
     {adj(Ability::Constitution, 1), adj(Ability::Charisma, -1), noAdj()},
     true, true, true},

    {RaceId::Elf, "Elf",
     {adj(Ability::Dexterity, 1), adj(Ability::Constitution, -1), noAdj()},
     false, false, true},

    {RaceId::Gnome, "Gnome",
     {adj(Ability::Intelligence, 1), adj(Ability::Wisdom, -1), noAdj()},
     true, false, true},

    {RaceId::HalfElf, "Half-Elf",
     {noAdj(), noAdj(), noAdj()},
     false, false, true},

    {RaceId::Halfling, "Halfling",
     {adj(Ability::Dexterity, 1), adj(Ability::Strength, -1), noAdj()},
     true, true, true},

    {RaceId::Kender, "Kender",
     {adj(Ability::Strength, -1), adj(Ability::Dexterity, 2), noAdj()},
     true, true, false},
}};

} // namespace

const RaceInfo& raceInfo(RaceId id) {
    for (const auto& info : kTable) {
        if (info.id == id) return info;
    }
    return kTable[0]; // unreachable given RaceId only has the values above
}

void applyRacialAdjustments(RaceId id, AbilityScores& scores) {
    for (const auto& a : raceInfo(id).adjustments) {
        if (a.delta != 0) scores.adjust(a.ability, a.delta);
    }
}

void applyRacialSavingThrowBonus(RaceId id, int constitution, SavingThrows& saves) {
    const RaceInfo& race = raceInfo(id);
    if (!race.hasConScaledMagicResistance) return;

    int bonus = constitutionMagicResistanceBonus(constitution);
    if (bonus == 0) return;

    saves.at(SaveCategory::RodStaffWand) -= bonus; // lower target number = easier save
    saves.at(SaveCategory::Spell) -= bonus;
    if (race.magicResistanceIncludesPoison) {
        saves.at(SaveCategory::ParalyzationPoisonDeath) -= bonus;
    }
}

} // namespace character
