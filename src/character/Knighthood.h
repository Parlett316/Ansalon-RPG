#pragma once

#include "character/Ability.h"
#include "character/Alignment.h"
#include "character/Race.h"

namespace character {

// Knights of Solamnia (Dragonlance Adventures, TSR 2021, p.14-20). Every
// Knight starts in the Order of the Crown; the Order of the Rose still
// requires a witnessed quest and XP thresholds this project doesn't model
// -- see docs/CHARACTER_NOTES.md. Sword is append-only after Crown, same
// precedent as quest::QuestStatus::ReadyToTurnIn.
enum class KnightOrder {
    None,
    Crown,
    Sword,
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

// True if these ability scores meet the Order of the Sword's minimums:
// STR 12, INT 9, WIS 13, DEX 9, CON 10 (no CHA minimum) -- per the book's
// Sword "Game Data" minimums box (p.18). That box is printed with a
// "Rose Knight Minimum Scores" header, a genuine erratum confirmed by
// cross-checking p.19's separate, correctly-labeled (and differently
// valued) Rose box -- see docs/CHARACTER_NOTES.md and docs/QUEST_NOTES.md.
// No race/class param: unlike Crown, this is only ever checked on a
// character who already has knightOrder == Crown, which already passed
// the Crown race gate.
bool meetsKnightOfSwordRequirements(const AbilityScores& scores);

} // namespace character
