#pragma once

#include "character/Ability.h"
#include "character/Alignment.h"
#include "character/CharClass.h"

#include <array>

namespace character {

// Five core 2e PHB races, plus Kender in place of Half-Orc AND Halfling --
// see docs/CHARACTER_NOTES.md for why: Half-Orc is not actually a standard
// race in the revised 2e Player's Handbook (it doesn't appear in Table 7
// or Table 8 at all -- it was moved to an optional sourcebook), and Krynn
// has no separate Halfling race in Dragonlance canon -- Kender fill that
// niche instead. Kender is the signature Dragonlance race, verified
// against Dragonlance Adventures (TSR 2021), p.53.
// Kender's raw value is pinned to its pre-removal ordinal (6) rather than
// renumbered down to 5 now that Halfling is gone -- SaveGame.cpp stores
// RACE as a raw int (see its own "raw-enum-int fragility" note), and a
// real save written before this removal has RACE 6 meaning Kender; letting
// it silently renumber to 5 would make that save mean something else
// instead of failing loudly. 5 is intentionally unused/unassigned.
enum class RaceId {
    Human,
    Dwarf,
    Elf,
    Gnome,
    HalfElf,
    Kender = 6,
};

// Highest RaceId ordinal + 1 -- NOT kAllRaces.size() (6): Kender's raw
// value is 6, one past what a plain count-based bound would allow, since
// value 5 is deliberately skipped above. Same shape as CharClass.h's
// kClassIdCount vs. kAllClasses. Used only by SaveGame.cpp's RACE bounds
// check.
constexpr int kRaceIdCount = 7;

struct AbilityAdjustment {
    Ability ability;
    int delta;
};

// Minimum/maximum a demihuman's ability score must fall within to be of
// that race/subrace, checked against the character's BASE (pre-racial-
// adjustment) rolled scores -- PHB p.27: "Consult Table 7 before making
// any racial adjustments... Once you satisfy the requirements at the
// start, you never have to worry about them again." Indexed by Ability,
// same convention as SubraceInfo::abilityAdjustments below.
struct AbilityRange {
    int min;
    int max;
};

struct RaceInfo {
    RaceId id;
    const char* name;
    std::array<AbilityAdjustment, 3> adjustments; // unused slots have delta == 0

    // Table 9 (Constitution Saving Throw Bonuses) applies to Dwarf, Gnome,
    // and Kender -- a bonus of +1 per ~3.5 points of Constitution against
    // magical wands/staves/rods/spells. Dwarf/Kender (not Gnome) also
    // apply it to poison specifically, which in our 5-category model means
    // the whole ParalyzationPoisonDeath category. Elf and Half-Elf have a
    // DIFFERENT ability (percentage magic resistance vs. sleep/charm
    // specifically) that doesn't map onto our save-category model at all
    // -- deliberately not represented as a save-table bonus; see
    // docs/CHARACTER_NOTES.md.
    bool hasConScaledMagicResistance;
    bool magicResistanceIncludesPoison;

    // PHB Table 7 (p.27) for Gnome/Half-Elf; Dragonlance Adventures p.53
    // for Kender. Human has no Table 7 entry at all (p.27: "Any character
    // can be a human, if the player so desires") and Elf/Dwarf's entries
    // here are unused in practice -- see meetsAbilityRange() below, both
    // races always resolve to a subrace before this matters.
    std::array<AbilityRange, 6> abilityRange;
};

const RaceInfo& raceInfo(RaceId id);

constexpr std::array<RaceId, 6> kAllRaces = {
    RaceId::Human, RaceId::Dwarf, RaceId::Elf, RaceId::Gnome, RaceId::HalfElf, RaceId::Kender,
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

    // Dragonlance Adventures' own per-subrace ability-score range (pp.59-61
    // Elves, pp.66-67 Dwarves) -- REPLACES RaceInfo::abilityRange the same
    // way abilityAdjustments replaces RaceInfo::adjustments. Indexed by
    // Ability, checked against the character's base (pre-adjustment) rolled
    // scores -- see meetsSubraceAbilityRange().
    std::array<AbilityRange, 6> abilityRange;
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

// True if `scores` (the character's BASE rolled scores, before any racial
// adjustment) fall within `race`'s Table 7 range on every ability. Always
// true for Human (no Table 7 entry) and for Elf/Dwarf (their entries are
// unused -- see meetsSubraceAbilityRange, which is what actually gates
// those two once a subrace is chosen).
bool meetsAbilityRange(RaceId race, const AbilityScores& scores);

// True if `scores` (base, pre-adjustment) fall within `subrace`'s own
// Dragonlance Adventures ability range on every ability.
bool meetsSubraceAbilityRange(SubraceId subrace, const AbilityScores& scores);

// DMG Table 7 ("Racial Class and Level Limits," p.15 -- a DM-facing table,
// not in the PHB, which explicitly defers this to "ask your DM") plus
// Dragonlance Adventures' own per-subrace/Kender class-limit tables.
// Returns the highest level `classId` can reach for this race/subrace: 0
// means not eligible for the class at all ("N/E" in the sourcebooks), 20
// (this project's own real level ceiling -- see Leveling.h) stands in for
// the sourcebooks' "Unlimited"/"U". Subrace overrides race the same way
// applyRacialOrSubracialAdjustments does; Elf/Dwarf's own base-race values
// are unused in practice for the same reason as meetsAbilityRange above.
// Gnome's own DMG Table 7 entry is sourced but unused in practice (Gnome
// is unconditionally forced into Tinker -- see CharacterCreator::run);
// Tinker itself has no sourced entry at all (it isn't a real PHB/DMG
// class) and this function always returns 20 for it, regardless of race.
//
// Two DM-optional/DL-specific "exceed the cap via an exceptional prime
// requisite" bonus-level mechanics (DMG Table 8, and Kender's own STR
// 17/18 Fighter-cap footnote, DLA p.53) are deliberately not modeled here
// -- see docs/CHARACTER_NOTES.md.
//
// Only Fighter/Mage/Cleric/Thief are sourced; this project's Mage/Cleric
// map onto the Dragonlance-specific "Wizard of High Sorcery"/"Holy Orders
// of the Stars" rows (the licensed/sanctioned tracks), not "Magic-User
// (Renegade)"/"Cleric (Heathen)" -- a project interpretation, not a purely
// mechanical fact, chosen because this project's Mage/Cleric already play
// as the licensed track (the Test of High Sorcery, real Cleric spells from
// the start) -- see docs/CHARACTER_NOTES.md.
int classLevelCap(RaceId race, SubraceId subrace, ClassId classId);

// subrace's Mage eligibility if a subrace is selected, else race's own --
// a thin wrapper over classLevelCap(race, subrace, ClassId::Mage) > 0.
bool effectiveCanBeMage(RaceId race, SubraceId subrace);

// Dragonlance Adventures p.53 ("Kender Game Statistics" box, the same page
// already sourcing Kender's ability ranges/class limits above): "they...
// cannot become monks because, regardless of alignment, they lack
// self-discipline. No evil kender are known to exist." The only race-based
// alignment restriction this project enforces -- every other race/class
// combination still picks freely from all 9 alignments (see
// docs/CHARACTER_NOTES.md's "Scope"). Hard-enforced in CharacterCreator
// the same reject-and-reprompt way as meetsAbilityRange/classLevelCap
// above, not the older "flagged, not blocked" style.
bool meetsAlignmentRestriction(RaceId race, Alignment alignment);

} // namespace character
