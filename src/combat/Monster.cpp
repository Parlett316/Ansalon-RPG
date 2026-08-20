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

void MonsterCatalog::addMonster(Monster monster) {
    monsters_.push_back(std::move(monster));
}

const Monster& MonsterCatalog::randomMonster(char terrainCode) const {
    if (monsters_.empty()) {
        throw std::runtime_error("MonsterCatalog::randomMonster called on an empty catalog");
    }

    std::vector<const Monster*> eligible;
    for (const auto& monster : monsters_) {
        if (!contains(monster.excludedTerrain, terrainCode)) {
            eligible.push_back(&monster);
        }
    }
    // Defensive fallback: can't happen with today's data (only Gnoll
    // excludes anything), but stay safe if a future terrain excludes every
    // monster in the roster rather than throwing mid-encounter.
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
