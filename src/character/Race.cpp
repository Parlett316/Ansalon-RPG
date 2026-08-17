#include "character/Race.h"

#include <array>

namespace character {

namespace {

AbilityAdjustment adj(Ability a, int delta) { return AbilityAdjustment{a, delta}; }
RacialSaveBonus save(SaveCategory c, int bonus) { return RacialSaveBonus{c, bonus}; }
RacialSaveBonus noSave() { return RacialSaveBonus{SaveCategory::ParalyzationPoisonDeath, 0}; }
AbilityAdjustment noAdj() { return AbilityAdjustment{Ability::Strength, 0}; }

// Ability adjustments and saving-throw bonuses are simplified/best-effort
// from memory of the 2e PHB race descriptions -- notably, real 2e dwarves
// and halflings have a saving-throw bonus that scales with Constitution
// rather than a flat number, which is simplified here to a flat +1. See
// docs/CHARACTER_NOTES.md. Spot-check against your books; each row here is
// the one place to correct if a number is off.
const std::array<RaceInfo, 7> kTable = {{
    {RaceId::Human, "Human",
     {noAdj(), noAdj(), noAdj()},
     {noSave(), noSave()}},

    {RaceId::Dwarf, "Dwarf",
     {adj(Ability::Constitution, 1), adj(Ability::Charisma, -1), noAdj()},
     {save(SaveCategory::ParalyzationPoisonDeath, 1), save(SaveCategory::RodStaffWand, 1)}},

    {RaceId::Elf, "Elf",
     {adj(Ability::Dexterity, 1), adj(Ability::Constitution, -1), noAdj()},
     {save(SaveCategory::Spell, 1), noSave()}},

    {RaceId::Gnome, "Gnome",
     {adj(Ability::Intelligence, 1), adj(Ability::Wisdom, -1), noAdj()},
     {save(SaveCategory::RodStaffWand, 1), noSave()}},

    {RaceId::HalfElf, "Half-Elf",
     {noAdj(), noAdj(), noAdj()},
     {noSave(), noSave()}},

    {RaceId::Halfling, "Halfling",
     {adj(Ability::Dexterity, 1), adj(Ability::Strength, -1), noAdj()},
     {save(SaveCategory::ParalyzationPoisonDeath, 1), save(SaveCategory::BreathWeapon, 1)}},

    {RaceId::HalfOrc, "Half-Orc",
     {adj(Ability::Strength, 1), adj(Ability::Charisma, -1), noAdj()},
     {noSave(), noSave()}},
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

} // namespace character
