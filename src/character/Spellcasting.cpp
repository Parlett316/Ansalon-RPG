#include "character/Spellcasting.h"
#include "character/Dice.h"
#include "character/Race.h"

#include <algorithm>
#include <array>

namespace character {

bool canCastSpells(ClassId id) {
    return id == ClassId::Mage || id == ClassId::Cleric;
}

namespace {

// Table 21 (Wizard Spell Progression, PHB p.43), FULL table (all 9 spell-
// level columns), levels 1-20 -- visually confirmed against the rendered
// page. Column 0 (1st-level slots) matches the values this project already
// shipped in Milestone 40.
constexpr std::array<std::array<int, 9>, 20> kWizardSlots = {{
    {1, 0, 0, 0, 0, 0, 0, 0, 0},
    {2, 0, 0, 0, 0, 0, 0, 0, 0},
    {2, 1, 0, 0, 0, 0, 0, 0, 0},
    {3, 2, 0, 0, 0, 0, 0, 0, 0},
    {4, 2, 1, 0, 0, 0, 0, 0, 0},
    {4, 2, 2, 0, 0, 0, 0, 0, 0},
    {4, 3, 2, 1, 0, 0, 0, 0, 0},
    {4, 3, 3, 2, 0, 0, 0, 0, 0},
    {4, 3, 3, 2, 1, 0, 0, 0, 0},
    {4, 4, 3, 2, 2, 0, 0, 0, 0},
    {4, 4, 4, 3, 3, 0, 0, 0, 0},
    {4, 4, 4, 4, 4, 1, 0, 0, 0},
    {5, 5, 5, 4, 4, 2, 0, 0, 0},
    {5, 5, 5, 4, 4, 2, 1, 0, 0},
    {5, 5, 5, 5, 5, 2, 1, 0, 0},
    {5, 5, 5, 5, 5, 3, 2, 1, 0},
    {5, 5, 5, 5, 5, 3, 3, 2, 0},
    {5, 5, 5, 5, 5, 3, 3, 2, 1},
    {5, 5, 5, 5, 5, 3, 3, 3, 1},
    {5, 5, 5, 5, 5, 4, 3, 3, 2},
}};

// Table 24 (Priest Spell Progression, PHB p.47), FULL table (all 7
// spell-level columns), levels 1-20 -- visually confirmed. Column 0
// matches Milestone 40's shipped values. Columns 5/6 (6th/7th-level slots,
// 0-indexed) are gated separately by Wisdom below, per the table's own
// footnote ("usable only by priests with 17/18 or greater Wisdom") --
// the raw numbers here are transcribed as printed regardless.
constexpr std::array<std::array<int, 7>, 20> kPriestSlots = {{
    {1, 0, 0, 0, 0, 0, 0},
    {2, 0, 0, 0, 0, 0, 0},
    {2, 1, 0, 0, 0, 0, 0},
    {3, 2, 0, 0, 0, 0, 0},
    {3, 3, 1, 0, 0, 0, 0},
    {3, 3, 2, 0, 0, 0, 0},
    {3, 3, 2, 1, 0, 0, 0},
    {3, 3, 3, 2, 0, 0, 0},
    {4, 4, 3, 2, 1, 0, 0},
    {4, 4, 3, 3, 2, 0, 0},
    {5, 4, 4, 3, 2, 1, 0},
    {6, 5, 5, 3, 2, 2, 0},
    {6, 6, 6, 4, 2, 2, 0},
    {6, 6, 6, 5, 3, 2, 1},
    {6, 6, 6, 6, 4, 2, 1},
    {7, 7, 7, 6, 4, 3, 1},
    {7, 7, 7, 7, 5, 3, 2},
    {8, 8, 8, 8, 6, 4, 2},
    {9, 9, 8, 8, 6, 4, 2},
    {9, 9, 9, 8, 7, 5, 2},
}};

// Table 5 (Wisdom, PHB p.23), "Bonus Spells" column, 1st-level entries only
// -- same scope as Milestone 40's original implementation (see
// Spellcasting.h's spellSlotsPerDay comment for why this isn't extended to
// higher spell levels this pass).
int wisdomBonus1stLevelSpells(int wisdom) {
    if (wisdom >= 23) return 4;
    if (wisdom >= 19) return 3;
    if (wisdom >= 14) return 2;
    if (wisdom >= 13) return 1;
    return 0;
}

// The ~49 spells this engine mechanically implements, out of the 88
// sourced from DQoK.pdf/PHB -- see docs/CHARACTER_NOTES.md for the full
// census (including the ~39 documented-but-excluded spells and why).
// Ordered by level then name, matching how they're offered in the
// memorization picker.
const std::vector<SpellInfo>& wizardSpells() {
    static const std::vector<SpellInfo> spells = {
        {"burning_hands", "Burning Hands", 1},
        {"charm_person", "Charm Person", 1},
        {"enlarge", "Enlarge", 1},
        {"magic_missile", "Magic Missile", 1},
        {"protection_from_evil", "Protection from Evil", 1},
        {"shocking_grasp", "Shocking Grasp", 1},
        {"sleep", "Sleep", 1},
        {"mirror_image", "Mirror Image", 2},
        {"ray_of_enfeeblement", "Ray of Enfeeblement", 2},
        {"stinking_cloud", "Stinking Cloud", 2},
        {"strength", "Strength", 2},
        {"fireball", "Fireball", 3},
        {"hold_person_wiz", "Hold Person", 3},
        {"lightning_bolt", "Lightning Bolt", 3},
        {"protection_from_evil_10_wiz", "Protection from Evil, 10' Radius", 3},
        {"slow", "Slow", 3},
        {"bestow_curse", "Bestow Curse", 4},
        {"charm_monster", "Charm Monster", 4},
        {"confusion", "Confusion", 4},
        {"fear", "Fear", 4},
        {"fumble", "Fumble", 4},
        {"ice_storm", "Ice Storm", 4},
        {"cloud_kill", "Cloudkill", 5},
        {"cone_of_cold", "Cone of Cold", 5},
        {"hold_monster", "Hold Monster", 5},
        {"death_spell", "Death Spell", 6},
        {"disintegrate", "Disintegrate", 6},
        {"flesh_to_stone", "Flesh to Stone", 6},
        {"delayed_blast_fireball", "Delayed Blast Fireball", 7},
        {"power_word_stun", "Power Word, Stun", 7},
        {"mass_charm", "Mass Charm", 8},
        {"otto_irresistible_dance", "Otto's Irresistible Dance", 8},
        {"power_word_blind", "Power Word, Blind", 8},
        {"meteor_swarm", "Meteor Swarm", 9},
        {"power_word_kill", "Power Word, Kill", 9},
    };
    return spells;
}

const std::vector<SpellInfo>& clericSpells() {
    static const std::vector<SpellInfo> spells = {
        {"bless", "Bless", 1},
        {"cure_light_wounds", "Cure Light Wounds", 1},
        {"protection_from_evil_cleric", "Protection from Evil", 1},
        {"hold_person_cleric", "Hold Person", 2},
        {"spiritual_hammer", "Spiritual Hammer", 2},
        {"prayer", "Prayer", 3},
        {"cure_serious_wounds", "Cure Serious Wounds", 4},
        {"protection_from_evil_10_cleric", "Protection from Evil, 10' Radius", 4},
        {"sticks_to_snakes", "Sticks to Snakes", 4},
        {"cure_critical_wounds", "Cure Critical Wounds", 5},
        {"dispel_evil", "Dispel Evil", 5},
        {"flame_strike", "Flame Strike", 5},
        {"blade_barrier", "Blade Barrier", 6},
        {"heal", "Heal", 6},
    };
    return spells;
}

} // namespace

const std::vector<SpellInfo>& spellListFor(ClassId id) {
    static const std::vector<SpellInfo> empty;
    if (id == ClassId::Mage) return wizardSpells();
    if (id == ClassId::Cleric) return clericSpells();
    return empty;
}

const SpellInfo* findSpell(ClassId id, const std::string& spellId) {
    for (const SpellInfo& spell : spellListFor(id)) {
        if (spell.id == spellId) return &spell;
    }
    return nullptr;
}

int spellSlotsPerDay(const Character& character, int spellLevel) {
    if (!canCastSpells(character.charClass)) return 0;
    if (character.charClass == ClassId::Mage) {
        if (!effectiveCanBeMage(character.race, character.subrace)) return 0;
        if (spellLevel < 1 || spellLevel > 9) return 0;
        int level = std::max(1, std::min(20, character.level));
        return kWizardSlots[static_cast<size_t>(level - 1)][static_cast<size_t>(spellLevel - 1)];
    }
    // Cleric.
    if (spellLevel < 1 || spellLevel > 7) return 0;
    if (spellLevel == 6 && character.scores.wisdom < 17) return 0;
    if (spellLevel == 7 && character.scores.wisdom < 18) return 0;
    int level = std::max(1, std::min(20, character.level));
    int slots = kPriestSlots[static_cast<size_t>(level - 1)][static_cast<size_t>(spellLevel - 1)];
    if (spellLevel == 1) slots += wisdomBonus1stLevelSpells(character.scores.wisdom);
    return slots;
}

int maxAccessibleSpellLevel(const Character& character) {
    int highestLevel = canCastSpells(character.charClass)
                            ? (character.charClass == ClassId::Mage ? 9 : 7)
                            : 0;
    for (int spellLevel = highestLevel; spellLevel >= 1; --spellLevel) {
        if (spellSlotsPerDay(character, spellLevel) > 0) return spellLevel;
    }
    return 0;
}

bool hasMemorizedSpellsAvailable(const Character& character, long long currentDay) {
    return character.spellsCastDay == currentDay && !character.memorizedSpellIds.empty();
}

void memorizeSpells(Character& character, long long currentDay, std::vector<std::string> spellIds) {
    character.spellsCastDay = currentDay;
    character.memorizedSpellIds = std::move(spellIds);
}

namespace {

// 1d6/level, capped at 10d6 -- PHB p.192 (Fireball)/p.194 (Lightning
// Bolt): "a maximum of 10d6" for both, visually confirmed. Shared here
// since Fireball/Delayed Blast Fireball/Lightning Bolt all use it.
int fireballLikeDamage(int level) {
    int dice = std::min(10, std::max(1, level));
    return roll(dice, 6);
}

} // namespace

SpellCastResult castSpell(Character& character, const std::string& spellId) {
    auto it = std::find(character.memorizedSpellIds.begin(), character.memorizedSpellIds.end(), spellId);
    if (it == character.memorizedSpellIds.end()) return SpellCastResult{};

    const SpellInfo* spell = findSpell(character.charClass, spellId);
    if (spell == nullptr) return SpellCastResult{}; // defensive -- shouldn't happen if memorized legally

    character.memorizedSpellIds.erase(it);

    SpellCastResult result;
    result.success = true;
    result.spellName = spell->name;
    int level = std::max(1, character.level);

    // Mage spells.
    if (spellId == "magic_missile") {
        // PHB p.176: 1d4+1 per missile, no attack roll, no saving throw --
        // one missile at 1st level plus one more every two levels, capped
        // at five (reached at 9th level).
        int missileCount = std::min(5, 1 + (level - 1) / 2);
        int damage = 0;
        for (int i = 0; i < missileCount; ++i) damage += roll(1, 4) + 1;
        result.effect = SpellEffect::DamageMonster;
        result.amount = damage;
    } else if (spellId == "burning_hands") {
        // PHB p.170 / DQoK p.26: flat 1 hit point of fire damage per level
        // of the caster, no saving throw -- no die roll at all.
        result.effect = SpellEffect::DamageMonster;
        result.amount = level;
    } else if (spellId == "charm_person" || spellId == "charm_monster" || spellId == "sleep" ||
               spellId == "hold_person_wiz" || spellId == "hold_monster" || spellId == "fear" ||
               spellId == "power_word_stun" || spellId == "mass_charm" || spellId == "otto_irresistible_dance") {
        // PHB: each of these incapacitates or drives off its target with no
        // fixed short duration in the source text ("until dispelled"/"flee
        // in terror"/etc.) -- modeled as taking the monster out of the
        // fight entirely, same category as Brooch of Imog's "rest of this
        // fight" simplification (docs/CHARACTER_NOTES.md's "Magic items").
        result.effect = SpellEffect::BlockMonsterAttacks;
        result.amount = kBlockRestOfFight;
    } else if (spellId == "enlarge") {
        // PHB p.173/DQoK p.26: "makes the recipient larger and stronger" --
        // modeled as a modest this-fight to-hit bonus.
        result.effect = SpellEffect::BuffPlayerThac0;
        result.amount = 1;
    } else if (spellId == "protection_from_evil" || spellId == "protection_from_evil_10_wiz" ||
               spellId == "protection_from_evil_cleric" || spellId == "protection_from_evil_10_cleric") {
        // PHB: "+2 to AC and saving throws against attacks by evil
        // creatures" -- modeled as a flat +2 AC this fight (this engine has
        // no per-attacker alignment check to gate it further).
        result.effect = SpellEffect::BuffPlayerAc;
        result.amount = 2;
    } else if (spellId == "shocking_grasp") {
        // PHB p.178: 1d8, +1 per level of the caster.
        result.effect = SpellEffect::DamageMonster;
        result.amount = roll(1, 8) + level;
    } else if (spellId == "mirror_image") {
        // PHB p.186: 1-4 illusionary duplicates draw off attacks until one
        // is hit -- modeled as blocking a handful of the monster's attacks.
        result.effect = SpellEffect::BlockMonsterAttacks;
        result.amount = roll(1, 4);
    } else if (spellId == "ray_of_enfeeblement") {
        // PHB p.187: reduces the target's Strength (25% + 2%/level) --
        // modeled as a flat reduction to the monster's damage this fight.
        result.effect = SpellEffect::DebuffMonsterDamage;
        result.amount = 2;
    } else if (spellId == "stinking_cloud" || spellId == "confusion" || spellId == "fumble") {
        // PHB: paralyzes/confuses/immobilizes for a few rounds (a saving
        // throw lessens but doesn't erase the effect) -- modeled as
        // blocking a handful of the monster's attacks.
        result.effect = SpellEffect::BlockMonsterAttacks;
        result.amount = 3;
    } else if (spellId == "strength") {
        // PHB p.188: raises the target's Strength -- modeled as a this-
        // fight damage bonus (distinct from Enlarge's to-hit bonus above).
        result.effect = SpellEffect::BuffPlayerDamage;
        result.amount = 2;
    } else if (spellId == "fireball" || spellId == "delayed_blast_fireball") {
        result.effect = SpellEffect::DamageMonster;
        result.amount = fireballLikeDamage(level);
    } else if (spellId == "lightning_bolt") {
        result.effect = SpellEffect::DamageMonster;
        result.amount = fireballLikeDamage(level);
    } else if (spellId == "slow") {
        // PHB p.196: halves the target's attacks/movement -- modeled as a
        // this-fight to-hit penalty on the monster (this engine has no
        // per-round attack count to actually halve).
        result.effect = SpellEffect::DebuffMonsterThac0;
        result.amount = 2;
    } else if (spellId == "bestow_curse") {
        // PHB: reduces the target's THAC0 and saving throws by 4 (DQoK
        // p.28 gives the same number).
        result.effect = SpellEffect::DebuffMonsterThac0;
        result.amount = 4;
    } else if (spellId == "power_word_blind") {
        // PHB p.238: strikes the target instantly blind -- modeled as a
        // this-fight to-hit penalty on the monster.
        result.effect = SpellEffect::DebuffMonsterThac0;
        result.amount = 4;
    } else if (spellId == "ice_storm") {
        // DQoK p.29: flat 3d10, no saving throw.
        result.effect = SpellEffect::DamageMonster;
        result.amount = roll(3, 10);
    } else if (spellId == "cone_of_cold") {
        // PHB p.212: 1d4+1 per level of experience of the caster, no cap.
        result.effect = SpellEffect::DamageMonster;
        result.amount = level * (roll(1, 4) + 1);
    } else if (spellId == "cloud_kill" || spellId == "death_spell" || spellId == "disintegrate" ||
               spellId == "flesh_to_stone" || spellId == "power_word_kill") {
        // PHB: each of these kills or destroys its target outright (a
        // saving throw may apply in the book; this engine has no monster
        // saving-throw system -- see docs/CHARACTER_NOTES.md's "no monster
        // saving throws" simplification). Modeled as an outright defeat.
        result.effect = SpellEffect::InstantDefeat;
    } else if (spellId == "meteor_swarm") {
        // DQoK p.30: "10-40 hit points of damage" -- the four-sphere total
        // applied directly as a uniform range (the real PHB spell sums four
        // separate 2d6 spheres for up to 48; DQoK's own quoted number is
        // used here as the practical implemented value, flagged since it
        // isn't an ordinary NdM roll).
        result.effect = SpellEffect::DamageMonster;
        result.amount = 10 + roll(1, 31) - 1;
    }
    // Cleric spells.
    else if (spellId == "cure_light_wounds") {
        result.effect = SpellEffect::HealCaster;
        result.amount = roll(1, 8);
    } else if (spellId == "bless") {
        result.effect = SpellEffect::BuffPlayerThac0;
        result.amount = 1;
    } else if (spellId == "hold_person_cleric" || spellId == "sticks_to_snakes") {
        result.effect = SpellEffect::BlockMonsterAttacks;
        result.amount = 3;
    } else if (spellId == "spiritual_hammer") {
        // DQoK p.24: "does normal hammer damage" -- reuses the Mace's own
        // 1d6 (CharClass.cpp's Cleric starting weapon), not a fresh PHB
        // citation, since DQoK doesn't give an explicit die.
        result.effect = SpellEffect::DamageMonster;
        result.amount = roll(1, 6);
    } else if (spellId == "prayer") {
        // PHB: +1 to friendly THAC0/saves AND -1 to enemy THAC0/saves in
        // the same casting.
        result.effect = SpellEffect::BuffPlayerAndDebuffMonsterThac0;
        result.amount = 1;
    } else if (spellId == "cure_serious_wounds") {
        // PHB: 2d8+1.
        result.effect = SpellEffect::HealCaster;
        result.amount = roll(2, 8) + 1;
    } else if (spellId == "cure_critical_wounds") {
        // PHB: 3d8+3.
        result.effect = SpellEffect::HealCaster;
        result.amount = roll(3, 8) + 3;
    } else if (spellId == "dispel_evil") {
        // DQoK p.25: +7 AC vs. evil attackers.
        result.effect = SpellEffect::BuffPlayerAc;
        result.amount = 7;
    } else if (spellId == "flame_strike") {
        // DQoK p.25: 6-48, i.e. 6d8.
        result.effect = SpellEffect::DamageMonster;
        result.amount = roll(6, 8);
    } else if (spellId == "blade_barrier") {
        // DQoK p.25: 8-64, i.e. 8d8.
        result.effect = SpellEffect::DamageMonster;
        result.amount = roll(8, 8);
    } else if (spellId == "heal") {
        // PHB: cures all but 1d4 of the target's missing hit points.
        result.effect = SpellEffect::HealCaster;
        result.amount = std::max(0, (character.maxHp - character.currentHp) - roll(1, 4));
    }

    return result;
}

} // namespace character
