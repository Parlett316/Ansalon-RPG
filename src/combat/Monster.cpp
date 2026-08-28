#include "combat/Monster.h"
#include "character/Dice.h"

#include <algorithm>
#include <stdexcept>

namespace combat {

namespace {

bool contains(const std::vector<char>& codes, char code) {
    return std::find(codes.begin(), codes.end(), code) != codes.end();
}

// Invented flavor weighting: a monster biased for this terrain is this many
// times more likely to be picked than an unbiased one. Tuned for feel, not
// sourced -- see docs/COMBAT_NOTES.md.
constexpr int kBiasWeight = 3;

} // namespace

bool isSweepEligible(const Monster& monster) {
    return monster.hpDiceCount <= 1;
}

int rollGroupSize(const Monster& monster) {
    if (monster.groupMax <= monster.groupMin) return monster.groupMin;
    return monster.groupMin + character::roll(1, monster.groupMax - monster.groupMin + 1) - 1;
}

void MonsterCatalog::addMonster(Monster monster) {
    monsters_.push_back(std::move(monster));
}

const Monster& MonsterCatalog::randomMonster(char terrainCode, int distanceToNearestTown) const {
    if (monsters_.empty()) {
        throw std::runtime_error("MonsterCatalog::randomMonster called on an empty catalog");
    }

    std::vector<const Monster*> eligible;
    for (const auto& monster : monsters_) {
        bool terrainOk = !contains(monster.excludedTerrain, terrainCode) &&
                          (monster.onlyTerrain.empty() || contains(monster.onlyTerrain, terrainCode));
        bool distanceOk = monster.minTownDistance <= 0 || distanceToNearestTown >= monster.minTownDistance;
        if (terrainOk && distanceOk) {
            eligible.push_back(&monster);
        }
    }
    // Defensive fallback: can't happen with today's data, but stay safe if a
    // future terrain/town-distance combination excludes every monster in the
    // roster rather than throwing mid-encounter.
    if (eligible.empty()) {
        for (const auto& monster : monsters_) {
            eligible.push_back(&monster);
        }
    }

    int totalWeight = 0;
    for (const Monster* monster : eligible) {
        totalWeight += contains(monster->terrainBias, terrainCode) ? kBiasWeight : 1;
    }

    int roll = character::roll(1, totalWeight);
    int cumulative = 0;
    for (const Monster* monster : eligible) {
        cumulative += contains(monster->terrainBias, terrainCode) ? kBiasWeight : 1;
        if (roll <= cumulative) return *monster;
    }
    return *eligible.back(); // unreachable: cumulative always reaches totalWeight
}

} // namespace combat
