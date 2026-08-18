#pragma once

#include "character/Ability.h"
#include "character/Alignment.h"
#include "character/Race.h"

namespace character {

// Knights of Solamnia (Dragonlance Adventures, TSR 2021, p.14-20). Every
// Knight starts in the Order of the Crown; advancing into the Order of the
// Sword and then the Order of the Rose requires XP thresholds and a
// witnessed quest this project doesn't model, since there's no leveling
// system yet -- see docs/CHARACTER_NOTES.md.
enum class KnightOrder {
    None,
    Crown,
};

const char* knightOrderName(KnightOrder order);

// True if this combination of race/subrace, ability scores, and alignment
// qualifies for sponsorship into the Order of the Crown, per the book's
// "Knights of the Crown / Game Data" minimums (p.18): STR 10, INT 7, WIS
// 10, DEX 8, CON 10 (no CHA minimum), and a Good alignment (the source ties
// Knighthood to the good/evil axis, not a law/chaos one). Also excludes
// every researched Elf/Dwarf subrace, which cannot join in any class-limit
// table checked -- see docs/CHARACTER_NOTES.md for what's confirmed vs.
// left as an unconfirmed-but-allowed default for other races. Does NOT
// check class -- CharacterCreator only offers this to Fighters (the book's
// Knights are built on the Cavalier class, which this project simplifies
// to "a Fighter who qualifies" -- see docs/CHARACTER_NOTES.md).
bool meetsKnightOfCrownRequirements(RaceId race, SubraceId subrace, const AbilityScores& scores,
                                     Alignment alignment);

} // namespace character
