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

// The only two derived ability modifiers this milestone needs: Constitution
// affects hit points, Dexterity affects AC, and those are the only two
// numbers currently computed on a character sheet. STR (to-hit/damage),
// INT/WIS (spell bonuses), and CHA (reactions) all have their own 2e
// tables too, but nothing in the game reads them yet -- deferred until
// combat/spellcasting exist to use them (see docs/CHARACTER_NOTES.md).
// Values below are best-effort from memory of the 2e PHB tables; spot-check
// against the books.
int hpAdjustmentForConstitution(int constitution);
int acAdjustmentForDexterity(int dexterity); // positive = better (lower) AC

} // namespace character
