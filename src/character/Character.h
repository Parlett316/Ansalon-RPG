#pragma once

#include "character/Ability.h"
#include "character/Alignment.h"
#include "character/CharClass.h"
#include "character/Race.h"

#include <string>

namespace character {

// A level-1 character sheet -- plain data, produced once by
// CharacterCreator and then carried in game::GameState for the rest of the
// session. See docs/CHARACTER_NOTES.md for exactly which 2e mechanics this
// does and does not model yet (no percentile Strength, no equipment/armor,
// no spellbook, THAC0 fixed at 20 -- true to 2e at level 1 regardless of
// class).
struct Character {
    std::string name;
    RaceId race = RaceId::Human;
    ClassId charClass = ClassId::Fighter;
    Alignment alignment = Alignment::TrueNeutral;

    AbilityScores scores;
    int level = 1;
    int maxHp = 1;
    int currentHp = 1;
    int armorClass = 10;
    int thac0 = 20;
    SavingThrows saves;
    int goldPieces = 0;
};

} // namespace character
