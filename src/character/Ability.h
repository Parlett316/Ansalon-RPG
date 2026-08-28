#pragma once

namespace character {

enum class Ability {
    Strength,
    Dexterity,
    Constitution,
    Intelligence,
    Wisdom,
    Charisma,
};

struct AbilityScores {
    int strength = 0;
    int dexterity = 0;
    int constitution = 0;
    int intelligence = 0;
    int wisdom = 0;
    int charisma = 0;

    int get(Ability a) const;
    void adjust(Ability a, int delta);
};

const char* abilityName(Ability a);

// The only two derived ability modifiers this milestone needs: Constitution
// affects hit points, Dexterity affects AC, and those are the only two
// numbers currently computed on a character sheet. STR (to-hit/damage),
// INT/WIS (spell bonuses), and CHA (reactions) all have their own 2e
// tables too, but nothing in the game reads them yet -- deferred until
// combat/spellcasting exist to use them (see docs/CHARACTER_NOTES.md).
// Values below are transcribed from the 2e Player's Handbook (revised),
// Table 2 (Dexterity, p.20) and Table 3 (Constitution, p.21).
//
// `isWarrior` matters only at Constitution 17-18: per Table 3's footnote,
// only warrior-group classes (here, just Fighter) get the full bonus there
// (+3/+4); every other class caps at +2 regardless of Constitution.
int hpAdjustmentForConstitution(int constitution, bool isWarrior);
int acAdjustmentForDexterity(int dexterity); // positive = better (lower) AC

// Table 9 (Constitution Saving Throw Bonuses, p.28) -- the "+1 per 3.5
// points of Constitution" bonus Dwarves, Gnomes, and (per Dragonlance
// Adventures, p.53) Kender get against magical wands, staves, rods, and
// spells -- and, for Dwarves/Kender specifically, also against poison (see
// character::Race for which categories each race applies this to). Returns
// 0 below the table's range (Constitution 3, reachable via 3d6, isn't
// covered by the printed table).
int constitutionMagicResistanceBonus(int constitution);

// Table 1 (Strength, PHB p.19), transcribed directly from the rendered
// page image. `exceptionalPercentile` is the 18/01-18/00 sub-bracket roll
// -- 0 if not applicable (Strength < 18, or a non-"warrior" class at
// Strength 18; only Fighter rolls this in CharacterCreator, see
// docs/CHARACTER_NOTES.md), otherwise 1-100.
int strengthToHitAdjustment(int strength, int exceptionalPercentile);
int strengthDamageAdjustment(int strength, int exceptionalPercentile);

} // namespace character
