#include "character/Spellcasting.h"
#include "character/Dice.h"
#include "character/Race.h"

#include <algorithm>
#include <array>

namespace character {

bool canCastSpells(ClassId id) {
    return id == ClassId::Mage || id == ClassId::Cleric;
}

const char* knownSpellName(ClassId id) {
    switch (id) {
        case ClassId::Mage: return "Magic Missile";
        case ClassId::Cleric: return "Cure Light Wounds";
        default: return "";
    }
}

namespace {

// Table 21 (Wizard Spell Progression, PHB p.43), 1st-level-spell column
// only, levels 1-20 -- visually confirmed against the rendered page.
constexpr std::array<int, 20> kWizardSlots = {1, 2, 2, 3, 4, 4, 4, 4, 4, 4,
                                               4, 4, 5, 5, 5, 5, 5, 5, 5, 5};

// Table 24 (Priest Spell Progression, PHB p.47), 1st-level-spell column
// only, levels 1-20 -- visually confirmed; independently cross-checked
// against the Cleric column of Table 23 (already sourced in Leveling.cpp)
// on the same rendered page.
constexpr std::array<int, 20> kPriestSlots = {1, 2, 2, 3, 3, 3, 3, 3, 4, 4,
                                               5, 6, 6, 6, 6, 7, 7, 8, 9, 9};

// Table 5 (Wisdom, PHB p.23), "Bonus Spells" column, cumulative per the
// book's own worked example ("a priest with a Wisdom of 15 is entitled to
// two 1st-level bonus spells and one 2nd-level bonus spell"). Only the
// 1st-level-spell entries matter here (Wisdom 13 and 14 each grant one;
// 19 and 23 grant one more apiece) -- 2nd-level-and-up bonus entries on
// other rows are irrelevant since this project only models 1st-level
// spells.
int wisdomBonus1stLevelSpells(int wisdom) {
    if (wisdom >= 23) return 4;
    if (wisdom >= 19) return 3;
    if (wisdom >= 14) return 2;
    if (wisdom >= 13) return 1;
    return 0;
}

} // namespace

int maxSpellSlotsPerDay(const Character& character) {
    if (!canCastSpells(character.charClass)) return 0;

    int level = std::max(1, std::min(20, character.level));
    if (character.charClass == ClassId::Mage) {
        // Real enforcement of the racial/subracial arcane-magic block --
        // flavor-only before spellcasting existed (see
        // docs/CHARACTER_NOTES.md). Kender's raceInfo.canBeMage is false;
        // Kagonesti Elf/Hill Dwarf/Mountain Dwarf set it via their
        // subrace's canBeMage instead.
        if (!effectiveCanBeMage(character.race, character.subrace)) return 0;
        return kWizardSlots[static_cast<size_t>(level - 1)];
    }
    // Cleric: level-based slots plus Wisdom bonus spells -- Mages get no
    // Intelligence-based bonus slots in 2e (INT governs chance-to-learn
    // and max spell level, not slot count), so this asymmetry is
    // deliberate, not a missing feature on the Mage side.
    return kPriestSlots[static_cast<size_t>(level - 1)] +
           wisdomBonus1stLevelSpells(character.scores.wisdom);
}

bool hasSpellSlotAvailable(const Character& character, long long currentDay) {
    return character.spellsCastDay == currentDay &&
           character.spellsCastToday < maxSpellSlotsPerDay(character);
}

void memorizeSpells(Character& character, long long currentDay) {
    character.spellsCastDay = currentDay;
    character.spellsCastToday = 0;
}

SpellCastResult castSpell(Character& character) {
    ++character.spellsCastToday;

    SpellCastResult result;
    if (character.charClass == ClassId::Mage) {
        // PHB p.176: "up to five missiles... he has two at 3rd level,
        // three at 5th level, four at 7th level, etc., up to a total of
        // five missiles at 9th level" -- one at 1st, plus one per two
        // levels, capped at five. Each missile: 1d4+1, no attack roll
        // (spell never misses), no saving throw.
        int level = std::max(1, character.level);
        int missileCount = std::min(5, 1 + (level - 1) / 2);
        int damage = 0;
        for (int i = 0; i < missileCount; ++i) {
            damage += roll(1, 4) + 1;
        }
        result.targetsMonster = true;
        result.amount = damage;
    } else {
        // Cure Light Wounds, PHB p.253: flat 1d8, no level scaling.
        result.targetsMonster = false;
        result.amount = roll(1, 8);
    }
    return result;
}

} // namespace character
