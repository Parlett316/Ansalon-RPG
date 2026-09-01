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
    // +1 to both the attack roll and damage -- a real magical enchantment,
    // distinct from weaponDamageBonus above (a mundane weapon's own base
    // damage die bonus, e.g. the Cleric's Footman's Flail, which carries no
    // to-hit bonus). See character::magicWeaponFor and
    // combat::resolvePlayerAttack.
    int weaponMagicBonus = 0;
    // Carried, not-currently-equipped items -- bought via the General
    // Store, worn via GameLoop::handleInventory ('i'). See Equipment.h's
    // equipInventoryItem for how items move between here and the equipped
    // fields above.
    std::vector<InventoryItem> inventory;

    // Mage/Cleric only -- see Spellcasting.h. spellsCastDay is the day
    // (hoursElapsed/24, -1 meaning "never memorized") the character last
    // memorized spells via game::GameLoop::handleRest -- no slots are
    // available at all until spellsCastDay == the current day, regardless
    // of level. memorizedSpellIds holds one entry per prepared slot
    // remaining today (repeats allowed -- the same spell can fill more than
    // one slot); character::castSpell removes one matching entry per cast.
    // preferredSpellIds is the standing loadout as last deliberately chosen
    // by the player (game::GameLoop::chooseSpellLoadout) -- unlike
    // memorizedSpellIds, casting a spell does NOT remove it from here.
    // Resting re-copies preferredSpellIds into memorizedSpellIds by default
    // (DQoK's own quoted design: "Selecting REST without choosing new
    // spells has the spellcasters rememorize the spells they have cast
    // since last resting"), only re-prompting the picker when the player
    // asks to change loadouts. See docs/CHARACTER_NOTES.md.
    std::vector<std::string> memorizedSpellIds;
    std::vector<std::string> preferredSpellIds;
    long long spellsCastDay = -1;
    // The day (same hoursElapsed/24 convention) the character last used
    // the Rest action ('r') -- see game::GameLoop::handleRest. Gates Rest
    // to once per in-game day so it can't be spammed for infinite healing.
    long long lastRestDay = -1;
    // The day the character last activated a Brooch of Imog (see
    // character::activateBrooch/broochAvailableToday and
    // GameLoop::runCombat) -- same once-per-day gate shape as lastRestDay.
    long long lastBroochUseDay = -1;
    // The day the character last used the Staff of Striking/Curing's cure
    // function (see character::useStaffCure/staffCureAvailableToday and
    // GameLoop::runCombat) -- same once-per-day gate shape as
    // lastBroochUseDay. The staff's own DLA text also caps it at 50
    // charges/5-per-day recharge, but that pool can never actually bind
    // once the "no more than once per day on a given individual" cap
    // already limits this engine's one player character to once/day --
    // see docs/CHARACTER_NOTES.md's "Magic items" for the full reasoning,
    // so no separate charge count is tracked.
    long long lastStaffCureDay = -1;
    // The day (same hoursElapsed/24 convention) the character last asked
    // Astinus of Palanthas a free-text question, and how many he's
    // answered so far that day -- see GameLoop::talkTo's ASK_LIMIT handling
    // and docs/ZONE_NOTES.md's "Ask about anything". Not a generic
    // once-per-day gate like lastRestDay above (which just blocks a second
    // use); this counts up to a per-POI limit before Astinus ends the
    // conversation, then resets once lastAstinusAskDay no longer matches
    // the current day.
    long long lastAstinusAskDay = -1;
    int astinusQuestionsToday = 0;
    // Today's effective question cap -- starts at the POI's ASK_LIMIT
    // (world::PointOfInterest::askLimit) and is raised to its
    // askLimitHardCap the moment the character passes the Intelligence+
    // Wisdom check GameLoop::talkTo rolls once astinusQuestionsToday first
    // reaches it. 0 (or stale from a previous day) means "not yet set for
    // today, use the POI's plain askLimit" -- same backward-compatibility
    // shape as the fields above.
    int astinusDailyLimit = 0;
};

} // namespace character
