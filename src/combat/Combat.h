#pragma once

#include "character/Character.h"
#include "combat/Monster.h"

namespace combat {

struct AttackOutcome {
    bool hit = false;
    int damage = 0; // only meaningful if hit
};

// PHB p.119/p.121: roll 1d20 + attacker's to-hit adjustment; hits if the
// total >= (attacker's THAC0 - target's AC). A natural 20 always hits and
// a natural 1 always misses, checked against the UNMODIFIED die roll, "regardless
// of any modifiers." Damage is floored at 1 on a hit (a near-universal
// convention, not itself re-verified this pass -- see docs/COMBAT_NOTES.md).
AttackOutcome resolvePlayerAttack(const character::Character& character, const Monster& monster);
AttackOutcome resolveMonsterAttack(const Monster& monster, const character::Character& character);

// PHB p.124: one d10 per side, lower roll acts first. Ties are rare enough
// (both parties would need to act "simultaneously," which this project's
// strictly-turn-ordered round loop can't represent) that this just
// re-rolls until there's a winner -- see docs/COMBAT_NOTES.md.
bool playerActsFirst();

// Standard 2e save convention: roll 1d20, succeed if the roll is >= the
// character's saving throw number for that category (a lower number is
// better/easier, same "at or above" convention as character::SavingThrows
// itself -- see CharClass.h). See docs/COMBAT_NOTES.md for what currently
// calls this (the Giant Spider's poison bite).
bool rollSavingThrow(const character::Character& character, character::SaveCategory category);

} // namespace combat
