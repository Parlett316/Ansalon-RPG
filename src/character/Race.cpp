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

    // Tinker Gnome numbers (Dragonlance Adventures, TSR 2021, p.57/printed
    // p.56), STR-1/DEX+2 -- NOT the generic PHB Gnome adjustment
    // (INT+1/WIS-1) this project used before. Krynn gnomes ARE Tinker
    // Gnomes; there's no other kind of PC gnome in this setting, same
    // precedent as Kender fully replacing Half-Orc rather than being an
    // optional variant. The book also says "Gnomes in Krynn can only be of
    // the tinker class" (a class this project doesn't implement) -- left
    // unenforced by user decision (2026), so Gnome stays selectable for all
    // four core classes; see docs/CHARACTER_NOTES.md and the caveat
    // CharacterCreator prints next to every class option for a Gnome.
    {RaceId::Gnome, "Gnome",
     {adj(Ability::Strength, -1), adj(Ability::Dexterity, 2), noAdj()},
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

namespace {

// Positional by Ability enum order (Strength, Dexterity, Constitution,
// Intelligence, Wisdom, Charisma) -- matches character/Ability.h exactly.
constexpr std::array<int, 6> abilities(int str = 0, int dex = 0, int con = 0, int intel = 0,
                                        int wis = 0, int cha = 0) {
    return {str, dex, con, intel, wis, cha};
}

// Every adjustment below verified against a scanned copy of Dragonlance
// Adventures (TSR 2021) by rendering the actual page image, not trusted
// from the scan's OCR text layer -- see docs/CHARACTER_NOTES.md for page
// citations and what's deliberately NOT modeled here (ability score
// ranges, class level limits, Gully Dwarf).
const std::array<SubraceInfo, 5> kSubraceTable = {{
    // p.60/59: identical to the base PHB Elf adjustment (CON-1/DEX+1) --
    // Silvanesti just happens to match it exactly.
    {SubraceId::SilvanestiElf, RaceId::Elf, "Silvanesti Elf", abilities(0, 1, -1, 0, 0, 0), true},

    // p.61/60: same CON-1/DEX+1 as Silvanesti.
    {SubraceId::QualinestiElf, RaceId::Elf, "Qualinesti Elf", abilities(0, 1, -1, 0, 0, 0), true},

    // p.62/61: STR+1/CON+1/DEX+2/INT-3 -- four simultaneous adjustments,
    // why SubraceInfo uses a 6-slot array instead of RaceInfo's 3-slot one.
    // Kagonesti cannot be Mages at all (unlike Silvanesti/Qualinesti).
    {SubraceId::KagonestiElf, RaceId::Elf, "Kagonesti Elf", abilities(1, 2, 1, -3, 0, 0), false},

    // p.67/66: CHA-1/CON+1, same as the base PHB Dwarf adjustment -- but
    // (Dragonlance-specific) Hill Dwarves cannot be Mages at all.
    {SubraceId::HillDwarf, RaceId::Dwarf, "Hill Dwarf", abilities(0, 0, 1, 0, 0, -1), false},

    // p.68/67: same CHA-1/CON+1 as Hill Dwarf; also cannot be Mages.
    {SubraceId::MountainDwarf, RaceId::Dwarf, "Mountain Dwarf", abilities(0, 0, 1, 0, 0, -1), false},
}};

} // namespace

const SubraceInfo* subraceInfo(SubraceId id) {
    if (id == SubraceId::None) return nullptr;
    for (const auto& info : kSubraceTable) {
        if (info.id == id) return &info;
    }
    return nullptr; // unreachable given SubraceId only has the values above
}

void applyRacialOrSubracialAdjustments(RaceId race, SubraceId subrace, AbilityScores& scores) {
    const SubraceInfo* sub = subraceInfo(subrace);
    if (sub == nullptr) {
        applyRacialAdjustments(race, scores);
        return;
    }
    for (size_t i = 0; i < sub->abilityAdjustments.size(); ++i) {
        int delta = sub->abilityAdjustments[i];
        if (delta != 0) scores.adjust(static_cast<Ability>(i), delta);
    }
}

bool effectiveCanBeMage(RaceId race, SubraceId subrace) {
    const SubraceInfo* sub = subraceInfo(subrace);
    return sub != nullptr ? sub->canBeMage : raceInfo(race).canBeMage;
}

} // namespace character
