#pragma once

#include "character/Ability.h"
#include "character/CharClass.h"

#include <array>

namespace character {

// Six core 2e PHB races, plus Kender in place of Half-Orc -- see
// docs/CHARACTER_NOTES.md for why: Half-Orc is not actually a standard
// race in the revised 2e Player's Handbook (it doesn't appear in Table 7
// or Table 8 at all -- it was moved to an optional sourcebook), and Kender
// is the signature Dragonlance race, verified against Dragonlance
// Adventures (TSR 2021), p.53.
enum class RaceId {
    Human,
    Dwarf,
    Elf,
    Gnome,
    HalfElf,
    Halfling,
    Kender,
};

struct AbilityAdjustment {
    Ability ability;
    int delta;
};

struct RaceInfo {
    RaceId id;
    const char* name;
    std::array<AbilityAdjustment, 3> adjustments; // unused slots have delta == 0

    // Table 9 (Constitution Saving Throw Bonuses) applies to Dwarf, Gnome,
    // Halfling, and Kender -- a bonus of +1 per ~3.5 points of Constitution
    // against magical wands/staves/rods/spells. Dwarf/Halfling/Kender
    // (not Gnome) also apply it to poison specifically, which in our
    // 5-category model means the whole ParalyzationPoisonDeath category.
    // Elf and Half-Elf have a DIFFERENT ability (percentage magic
    // resistance vs. sleep/charm specifically) that doesn't map onto our
    // save-category model at all -- deliberately not represented as a
    // save-table bonus; see docs/CHARACTER_NOTES.md.
    bool hasConScaledMagicResistance;
    bool magicResistanceIncludesPoison;

    // False only for Kender: per Dragonlance Adventures p.53, kender
    // cannot learn magic-user (Mage) or illusionist spells due to their
    // innate magic resistance.
    bool canBeMage;
};

const RaceInfo& raceInfo(RaceId id);

constexpr std::array<RaceId, 7> kAllRaces = {
    RaceId::Human, RaceId::Dwarf, RaceId::Elf,      RaceId::Gnome,
    RaceId::HalfElf, RaceId::Halfling, RaceId::Kender,
};

// Applies this race's ability adjustments to `scores` in place.
void applyRacialAdjustments(RaceId id, AbilityScores& scores);

// Adds this race's Table-9-style magic resistance bonus (if any) to
// `saves`, using `constitution` to look up the bonus via
// constitutionMagicResistanceBonus(). No-op for races without the ability.
void applyRacialSavingThrowBonus(RaceId id, int constitution, SavingThrows& saves);

// Dragonlance-specific elf/dwarf subraces (Dragonlance Adventures, TSR
// 2021), verified against scanned book pages -- see docs/CHARACTER_NOTES.md.
// Elf and Dwarf PCs always pick one of these (there's no generic "Elf" or
// "Dwarf" character on Krynn); every other race has no subrace, hence
// SubraceId::None. Gully Dwarf is deliberately not included -- see
// docs/CHARACTER_NOTES.md for why.
enum class SubraceId {
    None,
    SilvanestiElf,
    QualinestiElf,
    KagonestiElf,
    HillDwarf,
    MountainDwarf,
};

struct SubraceInfo {
    SubraceId id;
    RaceId parentRace;
    const char* name;

    // Indexed directly by Ability (static_cast<size_t>(Ability::X)), unlike
    // RaceInfo::adjustments' 3-slot array -- Kagonesti needs four
    // simultaneous non-zero adjustments (STR/CON/DEX/INT), more than that
    // array can hold. REPLACES the parent race's adjustments when a subrace
    // is selected; the two are never combined -- see
    // applyRacialOrSubracialAdjustments.
    std::array<int, 6> abilityAdjustments;

    // Overrides RaceInfo::canBeMage when a subrace is selected: Hill/
    // Mountain Dwarf and Kagonesti Elf cannot be Mages at all (a
    // Dragonlance-specific restriction the base Elf/Dwarf entries don't
    // have), while Silvanesti/Qualinesti Elf can.
    bool canBeMage;
};

// Returns nullptr for SubraceId::None.
const SubraceInfo* subraceInfo(SubraceId id);

constexpr std::array<SubraceId, 3> kElfSubraces = {
    SubraceId::SilvanestiElf, SubraceId::QualinestiElf, SubraceId::KagonestiElf,
};
constexpr std::array<SubraceId, 2> kDwarfSubraces = {
    SubraceId::HillDwarf, SubraceId::MountainDwarf,
};

// Applies `subrace`'s adjustments if it's not SubraceId::None, otherwise
// falls back to `race`'s own adjustments (applyRacialAdjustments) --
// subrace adjustments replace the base race's, they don't stack with it.
void applyRacialOrSubracialAdjustments(RaceId race, SubraceId subrace, AbilityScores& scores);

// subrace's canBeMage if a subrace is selected, else race's own.
bool effectiveCanBeMage(RaceId race, SubraceId subrace);

} // namespace character
