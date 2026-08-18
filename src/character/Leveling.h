#pragma once

#include "character/CharClass.h"
#include "character/Character.h"

#include <string>
#include <vector>

namespace character {

// The four PHB "class groups" leveling tables are organized by (PHB
// p.121/p.134) -- Fighter is Warrior, Mage is Wizard, Cleric is Priest,
// Thief is Rogue. Tinker maps to Wizard: a documented analogy (the book
// has no leveling tables for it either, same reasoning already used for
// Tinker's saves/steel at level 1 -- see docs/CHARACTER_NOTES.md).
enum class ClassGroup { Warrior, Wizard, Priest, Rogue };
ClassGroup classGroupFor(ClassId id);

// XP needed to REACH this level (level 1 = 0). Levels 1-20 are modeled
// (PHB Tables 14/20/23/25, the full printed tables); anything past 20
// returns a value experience can never reach, since going further isn't
// modeled yet -- see docs/CHARACTER_NOTES.md.
int xpThresholdForLevel(ClassId id, int level);

// Table 53 (PHB p.121), levels 1-20 (clamped).
int thac0ForLevel(ClassId id, int level);

// Table 60 (PHB p.134), a level-bracket lookup, levels 1-21+ (clamped).
SavingThrows savesForLevel(ClassId id, int level);

// Applies every level-up `character.experience` has earned, one level at
// a time (so a single big XP award still visits every intermediate level
// -- needed so the level-3 flavor moments below are never skipped).
// Updates level/HP/THAC0/saves in place and appends one message per level
// gained (plus Knight-of-Crown/Wizard-Robe flavor lines at level 3, where
// applicable) to `messages`. Returns the number of levels gained.
int applyPendingLevelUps(Character& character, std::vector<std::string>& messages);

} // namespace character
