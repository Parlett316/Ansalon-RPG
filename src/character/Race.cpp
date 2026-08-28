#include "character/Race.h"

#include <array>

namespace character {

namespace {

AbilityAdjustment adj(Ability a, int delta) { return AbilityAdjustment{a, delta}; }
AbilityAdjustment noAdj() { return AbilityAdjustment{Ability::Strength, 0}; }

// Positional by Ability enum order (Strength, Dexterity, Constitution,
// Intelligence, Wisdom, Charisma), matching character/Ability.h exactly --
// same convention Race.cpp's own kSubraceTable uses for abilityAdjustments
// below.
constexpr std::array<AbilityRange, 6> ranges(AbilityRange str, AbilityRange dex, AbilityRange con,
                                              AbilityRange intel, AbilityRange wis, AbilityRange cha) {
    return {str, dex, con, intel, wis, cha};
}

// Ability adjustments verified against a scanned copy of the 2e Player's
// Handbook (revised), Table 8 (p.27): Dwarf, Elf, Gnome matched what was
// already here exactly; Human and Half-Elf get no adjustments (also
// confirmed -- they simply don't appear in Table 8). Kender's adjustments
// (STR-1, DEX+2) and "all standard halfling abilities" note are from
// Dragonlance Adventures (TSR 2021), p.53 -- see docs/CHARACTER_NOTES.md
// for why Kender replaces both Half-Orc and Halfling here (Krynn has no
// separate Halfling race in Dragonlance canon).
//
// abilityRange values verified against the same PHB Table 7 (p.27) and, for
// Kender, Dragonlance Adventures p.53 -- both rendered as page images, not
// read from the scan's badly-garbled OCR text layer. Human's range is
// unused (see meetsAbilityRange -- Human has no Table 7 entry at all) and
// filled with the universal 3-18 roll range as a harmless placeholder;
// Dwarf/Elf's are the real Table 7 numbers but are also unused in
// practice, since both races always resolve to a subrace (see
// kSubraceTable below) before a range check matters.
const std::array<RaceInfo, 6> kTable = {{
    {RaceId::Human, "Human",
     {noAdj(), noAdj(), noAdj()},
     false, false,
     ranges({3, 18}, {3, 18}, {3, 18}, {3, 18}, {3, 18}, {3, 18})},

    {RaceId::Dwarf, "Dwarf",
     {adj(Ability::Constitution, 1), adj(Ability::Charisma, -1), noAdj()},
     true, true,
     ranges({8, 18}, {3, 17}, {11, 18}, {3, 18}, {3, 18}, {3, 17})},

    {RaceId::Elf, "Elf",
     {adj(Ability::Dexterity, 1), adj(Ability::Constitution, -1), noAdj()},
     false, false,
     ranges({3, 18}, {6, 18}, {7, 18}, {8, 18}, {3, 18}, {8, 18})},

    // Tinker Gnome numbers (Dragonlance Adventures, TSR 2021, p.57/printed
    // p.56), STR-1/DEX+2 -- NOT the generic PHB Gnome adjustment
    // (INT+1/WIS-1) this project used before. Krynn gnomes ARE Tinker
    // Gnomes; there's no other kind of PC gnome in this setting, same
    // precedent as Kender fully replacing Half-Orc rather than being an
    // optional variant. The book also says "Gnomes in Krynn can only be of
    // the tinker class" -- CharacterCreator now hard-enforces this (every
    // Gnome PC is unconditionally a Tinker, no class prompt shown), so
    // classLevelCap's PHB Table 7 Gnome entries (which cap ordinary
    // Fighter/Cleric/Thief and forbid Mage) are moot in practice; see
    // docs/CHARACTER_NOTES.md.
    {RaceId::Gnome, "Gnome",
     {adj(Ability::Strength, -1), adj(Ability::Dexterity, 2), noAdj()},
     true, false,
     ranges({6, 18}, {3, 18}, {8, 18}, {6, 18}, {3, 18}, {3, 18})},

    {RaceId::HalfElf, "Half-Elf",
     {noAdj(), noAdj(), noAdj()},
     false, false,
     ranges({3, 18}, {6, 18}, {6, 18}, {4, 18}, {3, 18}, {3, 18})},

    {RaceId::Kender, "Kender",
     {adj(Ability::Strength, -1), adj(Ability::Dexterity, 2), noAdj()},
     true, true,
     ranges({6, 16}, {8, 19}, {10, 18}, {6, 18}, {3, 16}, {6, 18})},
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

// Every adjustment and ability range below verified against a scanned copy
// of Dragonlance Adventures (TSR 2021) by rendering the actual page image,
// not trusted from the scan's OCR text layer, which badly garbles these
// tables -- see docs/CHARACTER_NOTES.md for page citations. Class
// eligibility/level limits live in classLevelCap() below instead of here,
// alongside the same sourcebook's per-subrace class-limit tables; Gully
// Dwarf remains deliberately not included -- see docs/CHARACTER_NOTES.md.
const std::array<SubraceInfo, 5> kSubraceTable = {{
    // Adjustments p.60/59 (identical to the base PHB Elf adjustment --
    // Silvanesti just happens to match it exactly); ability range from the
    // "Silvanesti Elves Ability Scores" table, p.59.
    {SubraceId::SilvanestiElf, RaceId::Elf, "Silvanesti Elf", abilities(0, 1, -1, 0, 0, 0),
     ranges({3, 18}, {7, 19}, {6, 18}, {10, 18}, {6, 18}, {12, 18})},

    // Adjustments p.61/60 (same CON-1/DEX+1 as Silvanesti); ability range
    // from "Qualinesti Elves Ability Scores," p.60.
    {SubraceId::QualinestiElf, RaceId::Elf, "Qualinesti Elf", abilities(0, 1, -1, 0, 0, 0),
     ranges({7, 18}, {7, 19}, {7, 18}, {8, 18}, {6, 18}, {8, 18})},

    // Adjustments p.62/61: STR+1/CON+1/DEX+2/INT-3 -- four simultaneous
    // adjustments, why SubraceInfo uses a 6-slot array instead of
    // RaceInfo's 3-slot one. Ability range from "Kagonesti Elves Ability
    // Scores," p.61.
    {SubraceId::KagonestiElf, RaceId::Elf, "Kagonesti Elf", abilities(1, 2, 1, -3, 0, 0),
     ranges({8, 18}, {8, 19}, {8, 18}, {3, 12}, {8, 18}, {8, 18})},

    // Adjustments p.67/66: CHA-1/CON+1, same as the base PHB Dwarf
    // adjustment. Ability range from "Hill Dwarf Ability Scores," p.66.
    {SubraceId::HillDwarf, RaceId::Dwarf, "Hill Dwarf", abilities(0, 0, 1, 0, 0, -1),
     ranges({9, 18}, {3, 17}, {14, 19}, {3, 18}, {3, 18}, {3, 12})},

    // Adjustments p.68/67: same CHA-1/CON+1 as Hill Dwarf. Ability range
    // from "Mountain Dwarf Ability Scores," p.67.
    {SubraceId::MountainDwarf, RaceId::Dwarf, "Mountain Dwarf", abilities(0, 0, 1, 0, 0, -1),
     ranges({8, 18}, {3, 17}, {12, 19}, {3, 18}, {3, 18}, {3, 16})},
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

namespace {

constexpr std::array<Ability, 6> kAbilities = {
    Ability::Strength, Ability::Dexterity,   Ability::Constitution,
    Ability::Intelligence, Ability::Wisdom,  Ability::Charisma,
};

bool withinRange(const std::array<AbilityRange, 6>& range, const AbilityScores& scores) {
    for (Ability a : kAbilities) {
        int score = scores.get(a);
        const AbilityRange& r = range[static_cast<size_t>(a)];
        if (score < r.min || score > r.max) return false;
    }
    return true;
}

struct ClassCaps {
    int fighter;
    int mage;
    int cleric;
    int thief;
};

// DMG Table 7 ("Racial Class and Level Limits," p.15 -- a DM-facing table;
// the PHB itself (p.27) explicitly defers this to "ask your DM for the
// level limits imposed on nonhuman characters," which is why a second
// rulebook was needed). 20 stands in for "U"/Unlimited (this project's own
// real level ceiling -- see Leveling.h); 0 stands in for "--"/N/E, not
// eligible for the class at all. Elf/Dwarf/Gnome's rows are real sourced
// values but unused in practice (Elf/Dwarf always resolve to a subrace --
// see subraceClassCaps below; Gnome is unconditionally forced into Tinker,
// which classLevelCap special-cases before ever consulting this table).
ClassCaps raceClassCaps(RaceId id) {
    switch (id) {
        case RaceId::Human: return {20, 20, 20, 20};
        case RaceId::Dwarf: return {15, 0, 10, 12};
        case RaceId::Elf: return {12, 15, 12, 12};
        case RaceId::Gnome: return {11, 0, 9, 13};
        case RaceId::HalfElf: return {14, 12, 14, 12};
        case RaceId::Kender: return {5, 0, 12, 20};
    }
    return {20, 20, 20, 20}; // unreachable given RaceId only has the values above
}

// Dragonlance Adventures' own per-subrace class-limit tables (Silvanesti/
// Qualinesti/Kagonesti Elves, pp.59-61; Hill/Mountain Dwarves, pp.66-67).
// Mage and Cleric are mapped onto the sourcebook's "Wizard of High
// Sorcery" and "Holy Orders of the Stars" rows specifically, not "Magic-
// User (Renegade)"/"Cleric (Heathen)" -- see classLevelCap's declaration
// in Race.h and docs/CHARACTER_NOTES.md for why.
ClassCaps subraceClassCaps(SubraceId id) {
    switch (id) {
        case SubraceId::None: break; // unreachable -- classLevelCap checks first
        case SubraceId::SilvanestiElf: return {10, 20, 20, 0};
        case SubraceId::QualinestiElf: return {14, 20, 20, 20};
        case SubraceId::KagonestiElf: return {20, 0, 7, 20};
        case SubraceId::HillDwarf: return {20, 0, 10, 10};
        case SubraceId::MountainDwarf: return {20, 0, 10, 8};
    }
    return {20, 20, 20, 20}; // unreachable given SubraceId only has the values above
}

int capForCoreClass(const ClassCaps& caps, ClassId id) {
    switch (id) {
        case ClassId::Fighter: return caps.fighter;
        case ClassId::Mage: return caps.mage;
        case ClassId::Cleric: return caps.cleric;
        case ClassId::Thief: return caps.thief;
        case ClassId::Tinker: break; // classLevelCap special-cases Tinker before this runs
    }
    return 20; // unreachable
}

} // namespace

bool meetsAbilityRange(RaceId race, const AbilityScores& scores) {
    // Human has no Table 7 entry at all (PHB p.27: "Any character can be a
    // human, if the player so desires") -- not merely a wide range, a real
    // absence of any check.
    if (race == RaceId::Human) return true;
    return withinRange(raceInfo(race).abilityRange, scores);
}

bool meetsSubraceAbilityRange(SubraceId subrace, const AbilityScores& scores) {
    const SubraceInfo* sub = subraceInfo(subrace);
    if (sub == nullptr) return true; // SubraceId::None -- nothing to check
    return withinRange(sub->abilityRange, scores);
}

int classLevelCap(RaceId race, SubraceId subrace, ClassId classId) {
    // Tinker isn't a real PHB/DMG/DLA class -- no sourced table entry
    // exists for it at all, and Gnome (the only race that plays one) is
    // unconditionally forced into it, so there's nothing to cap.
    if (classId == ClassId::Tinker) return 20;

    const SubraceInfo* sub = subraceInfo(subrace);
    ClassCaps caps = sub != nullptr ? subraceClassCaps(sub->id) : raceClassCaps(race);
    return capForCoreClass(caps, classId);
}

bool effectiveCanBeMage(RaceId race, SubraceId subrace) {
    return classLevelCap(race, subrace, ClassId::Mage) > 0;
}

} // namespace character
