#pragma once

#include "character/Character.h"

#include <string>
#include <vector>

namespace character {

bool canCastSpells(ClassId id);

// One entry in a class's spell roster -- id is stable/snake_case ("magic_missile"),
// used in save files and for lookup; name is the display string.
struct SpellInfo {
    std::string id;
    std::string name;
    int level; // 1-9 (Mage) / 1-7 (Cleric)
};

// The spells this engine mechanically implements for a class, ordered by
// level then name -- NOT the complete real PHB class list. This project
// cross-referenced the full DQoK.pdf ("Dark Queen of Krynn," an official
// TSR/SSI Dragonlance computer game) spell census -- 59 Mage spells across
// 9 levels, 29 Cleric spells across 7 -- against the Player's Handbook
// (revised)'s own spell index and Tables 21/24, and found 85 of 88 matched
// PHB exactly (three didn't -- see docs/CHARACTER_NOTES.md's
// "Spellcasting" section for the corrections). Of those 88, roughly 49 are
// implemented here; the other ~39 (utility/exploration spells, and spells
// whose real effect needs a subsystem this engine doesn't have --
// poison/disease/blindness/curse status, monster saving throws,
// damage-type resistance, multi-attack rounds, ally summoning) are sourced
// and documented but not selectable in-game, same "restraint over
// completeness" precedent Milestone 56 used for DLA's magic item chapter.
// See docs/CHARACTER_NOTES.md for the full census and per-spell citations.
//
// Every caster automatically "knows" every spell on this list once their
// level unlocks it -- a documented simplification of the Wizard's real
// spell-research/spellbook-copying rules (2e Priests already work this way
// for real; extending it to Mage avoids building a second, smaller "known
// spells" subsystem just for one class).
const std::vector<SpellInfo>& spellListFor(ClassId id);

const SpellInfo* findSpell(ClassId id, const std::string& spellId);

// Real per-day slot count for one spell level -- PHB Table 21 (Wizard
// Spell Progression, p.43) / Table 24 (Priest Spell Progression, p.47),
// FULL tables now (every spell-level column, not just 1st), visually
// confirmed against rendered page images. Cleric's 6th/7th-level columns
// additionally require Wisdom 17+/18+ (the table's own footnote) -- a
// character below that Wisdom gets 0 in that column regardless of level.
// Wisdom bonus spells (Table 5, p.23) still only cover 1st-level slots, the
// same scope the original 1st-level-only implementation had -- Table 5's
// bonus-spell breakdown at higher spell levels wasn't re-verified this
// pass, so it isn't claimed. Mage gets no Intelligence-based bonus slots at
// any level (real 2e asymmetry, INT governs chance-to-learn/max spell
// level for Wizards, not slot count). A Mage who can't actually cast
// arcane magic (Kender, or a subrace with effectiveCanBeMage false) always
// gets 0, regardless of level or spellLevel.
int spellSlotsPerDay(const Character& character, int spellLevel);

// The highest spell level spellSlotsPerDay(character, level) is nonzero
// for, i.e. which levels of spellListFor(character.charClass) are actually
// offered for memorization right now. 0 if the character can't prepare any
// spell yet (including a Mage blocked from arcane magic entirely).
int maxAccessibleSpellLevel(const Character& character);

// True if the character has memorized at least one spell today
// (character.spellsCastDay == currentDay) that hasn't been cast yet.
bool hasMemorizedSpellsAvailable(const Character& character, long long currentDay);

// PHB p.107 (Wizard)/p.111 (Priest): a caster needs a restful night's sleep
// before they can (re-)memorize their spells for the day. Replaces
// character.memorizedSpellIds with `spellIds` (one entry per prepared slot;
// repeats are allowed -- 2e lets a caster memorize the same spell into more
// than one slot) and sets spellsCastDay = currentDay. Called only from
// game::GameLoop::handleRest/handleBedRest ('r'/'z'). Does not itself
// validate that spellIds is a legal loadout (right ids, right count per
// level, level actually accessible) -- the caller builds it from
// spellListFor/spellSlotsPerDay/maxAccessibleSpellLevel already, so
// there's nothing left to check here.
void memorizeSpells(Character& character, long long currentDay, std::vector<std::string> spellIds);

// How castSpell resolved a spell -- game::GameLoop::runCombat dispatches on
// this rather than the spell's name/id, so adding a new spell that reuses
// an existing category needs no GameLoop change.
enum class SpellEffect {
    DamageMonster,
    HealCaster,
    // amount is the number of the monster's attacks blocked, EXCEPT the
    // sentinel kBlockRestOfFight (below), which blocks all of its remaining
    // attacks this fight -- same local-variable-in-runCombat shape as the
    // existing Webnet (a counted block)/Brooch of Imog (rest-of-fight)
    // precedent, just generalized to one mechanism for both.
    BlockMonsterAttacks,
    BuffPlayerThac0,      // amount: bonus to the player's own to-hit, this fight
    BuffPlayerDamage,      // amount: bonus to the player's own damage, this fight
    BuffPlayerAc,           // amount: reduction to the player's AC, this fight
    DebuffMonsterThac0,     // amount: penalty to the monster's to-hit, this fight
    DebuffMonsterDamage,    // amount: flat reduction to the monster's damage, this fight
    // Prayer (PHB p.271): the one spell in this roster that buffs the
    // player's THAC0 AND debuffs the monster's THAC0 by the same amount in
    // one cast -- a dedicated case rather than two SpellCastResults, since
    // castSpell only consumes one memorized slot per call.
    BuffPlayerAndDebuffMonsterThac0,
    // Haste (PHB p.192): "functions at double its normal... attack rate."
    // amount: multiplies the player's attacks-per-round this fight (see
    // character::meleeAttacksThisRound) -- player-only, same as every other
    // buff effect above; companions never read the player's spell buffs.
    HastePlayer,
    // Slow (PHB p.196): "an Armor Class penalty of +4 AC, an attack penalty
    // of -4" -- one dedicated case sharing a single amount across both
    // penalties, same shape as BuffPlayerAndDebuffMonsterThac0 above.
    DebuffMonsterThac0AndAc,
    InstantDefeat, // the monster is simply defeated -- sets monsterHp to 0
};

// amount == this sentinel (for BlockMonsterAttacks only) means "for the
// rest of this fight," not a counted number of attacks.
constexpr int kBlockRestOfFight = -1;

struct SpellCastResult {
    bool success = false; // false if spellId wasn't actually memorized -- nothing consumed
    std::string spellName;
    SpellEffect effect = SpellEffect::DamageMonster;
    int amount = 0;
};

// Removes one occurrence of spellId from character.memorizedSpellIds and
// resolves its effect. Returns {success = false} (no state change) if
// spellId isn't currently memorized -- callers should only offer ids
// actually present in the character's memorized list (see
// hasMemorizedSpellsAvailable), same "caller already checked" contract
// castSpell's single-known-spell predecessor had.
SpellCastResult castSpell(Character& character, const std::string& spellId);

} // namespace character
