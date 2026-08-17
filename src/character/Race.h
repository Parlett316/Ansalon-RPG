#pragma once

#include "character/Ability.h"
#include "character/CharClass.h"

#include <array>

namespace character {

enum class RaceId {
    Human,
    Dwarf,
    Elf,
    Gnome,
    HalfElf,
    Halfling,
    HalfOrc,
};

struct AbilityAdjustment {
    Ability ability;
    int delta;
};

struct RacialSaveBonus {
    SaveCategory category;
    int bonus; // 0 means "unused slot"; positive makes the target number easier (lower)
};

struct RaceInfo {
    RaceId id;
    const char* name;
    std::array<AbilityAdjustment, 3> adjustments; // unused slots have delta == 0
    std::array<RacialSaveBonus, 2> saveBonuses;   // unused slots have bonus == 0
};

const RaceInfo& raceInfo(RaceId id);

constexpr std::array<RaceId, 7> kAllRaces = {
    RaceId::Human, RaceId::Dwarf,  RaceId::Elf,     RaceId::Gnome,
    RaceId::HalfElf, RaceId::Halfling, RaceId::HalfOrc,
};

// Applies this race's ability adjustments to `scores` in place.
void applyRacialAdjustments(RaceId id, AbilityScores& scores);

} // namespace character
