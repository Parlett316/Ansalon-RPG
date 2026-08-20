#pragma once

#include "character/Ability.h"
#include "character/Alignment.h"
#include "character/Race.h"

namespace character {

// Knights of Solamnia (Dragonlance Adventures, TSR 2021, p.14-20). Every
// Knight starts in the Order of the Crown, advances into the Order of the
// Sword (see meetsKnightOfSwordRequirements below), and finally the Order
// of the Rose (see meetsKnightOfRoseRequirements below) -- see
// docs/CHARACTER_NOTES.md. Sword and Rose are append-only after Crown, same
// precedent as quest::QuestStatus::ReadyToTurnIn.
enum class KnightOrder {
    None,
    Crown,
    Sword,
    Rose,
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

// True if these ability scores meet the Order of the Rose's minimums:
// STR 15, INT 10, WIS 13, DEX 12, CON 15 (no CHA minimum) -- per the book's
// Rose "Game Data" minimums box, printed p.19 (PDF p.20), correctly labeled
// this time (the p.18/PDF p.19 box under this same heading is actually the
// Sword minimums -- see meetsKnightOfSwordRequirements above and
// docs/CHARACTER_NOTES.md). The book's own prose for the level threshold is
// internally inconsistent -- "two levels as Crown, then Sword and two
// additional levels" (arithmetically level 5) versus "sufficient hit points
// to become 4th level" in the very next sentence -- resolved via the Rose
// Knight Advancement Table itself, which starts at level 4 ("Novice of
// Roses"); see game::conditionMatches's "rose_eligible" token and
// docs/CHARACTER_NOTES.md for the full sourcing note. No race/class param,
// same reasoning as meetsKnightOfSwordRequirements: only ever checked on a
// character who already has knightOrder == Sword, which already passed the
// Crown race gate.
bool meetsKnightOfRoseRequirements(const AbilityScores& scores);

} // namespace character
