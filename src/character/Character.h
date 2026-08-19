#pragma once

#include "character/Ability.h"
#include "character/Alignment.h"
#include "character/CharClass.h"
#include "character/Equipment.h"
#include "character/Knighthood.h"
#include "character/Race.h"
#include "character/WizardOrder.h"

#include <string>

namespace character {

// A character sheet -- plain data, produced at level 1 by CharacterCreator
// and then carried in game::GameState for the rest of the session, mutated
// in place by character::applyPendingLevelUps (Leveling.h) as experience
// is earned. See docs/CHARACTER_NOTES.md for exactly which 2e mechanics
// this does and does not model yet (no spellbook, no carried-item
// inventory beyond the equipped armor/weapon below).
struct Character {
    std::string name;
    RaceId race = RaceId::Human;
    SubraceId subrace = SubraceId::None; // Elf/Dwarf always set one -- see Race.h
    ClassId charClass = ClassId::Fighter;
    Alignment alignment = Alignment::TrueNeutral;
    KnightOrder knightOrder = KnightOrder::None; // Fighters only -- see Knighthood.h
    // Set at level 3 for a Mage (the Test of High Sorcery) -- see
    // Leveling.h/WizardOrder.h and docs/CHARACTER_NOTES.md.
    RobeColor robeColor = RobeColor::None;

    AbilityScores scores;
    // 0 unless charClass == Fighter and scores.strength == 18 -- see
    // character::strengthToHitAdjustment/strengthDamageAdjustment and
    // docs/CHARACTER_NOTES.md for why only Fighter rolls this.
    int exceptionalStrengthPercentile = 0;
    int level = 1;
    int experience = 0;
    int maxHp = 1;
    int currentHp = 1;
    int armorClass = 10;
    int thac0 = 20;
    SavingThrows saves;
    int steelPieces = 0; // Krynn's post-Cataclysm currency -- see docs/CHARACTER_NOTES.md

    // Equipment (see character/Equipment.h) -- seeded at creation from the
    // class's starting weapon (CharacterCreator.cpp), changed only via
    // Equipment::purchaseItem at the General Store. armorClass above is
    // always kept in sync via Equipment::recomputeArmorClass, not derived
    // on the fly -- same "plain data, mutated in place" shape as the rest
    // of this struct.
    ArmorId equippedArmor = ArmorId::None;
    bool hasShield = false;
    std::string weaponName;
    int weaponDamageSides = 4;
    int weaponDamageBonus = 0;
    // Carried, not-currently-equipped items -- bought via the General
    // Store, worn via GameLoop::handleInventory ('i'). See Equipment.h's
    // equipInventoryItem for how items move between here and the equipped
    // fields above.
    std::vector<InventoryItem> inventory;

    // Mage/Cleric only -- see Spellcasting.h. spellsCastDay is the day
    // (hoursElapsed/24, -1 meaning "never memorized") the character last
    // memorized spells via game::GameLoop::handleRest -- no slots are
    // available at all until spellsCastDay == the current day, regardless
    // of level. spellsCastToday counts casts against that memorization,
    // reset to 0 by character::memorizeSpells.
    int spellsCastToday = 0;
    long long spellsCastDay = -1;
    // The day (same hoursElapsed/24 convention) the character last used
    // the Rest action ('r') -- see game::GameLoop::handleRest. Gates Rest
    // to once per in-game day so it can't be spammed for infinite healing.
    long long lastRestDay = -1;
};

} // namespace character
