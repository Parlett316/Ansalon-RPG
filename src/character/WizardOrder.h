#pragma once

#include "character/Alignment.h"

namespace character {

// The Robe a Wizard of High Sorcery is assigned at the Test of High
// Sorcery (Dragonlance Adventures, TSR 2021, p.35-37; see
// docs/CHARACTER_NOTES.md), which this project models as happening at
// 3rd level -- see character::applyPendingLevelUps (Leveling.h). Assigned
// by alignment, not chosen by the player, per the source material.
enum class RobeColor {
    None,
    White,
    Red,
    Black,
};

const char* robeColorName(RobeColor color);

// Solinari (White), Lunitari (Red), Nuitari (Black) -- the three gods/moons
// of magic each Robe draws power from (Dragonlance Adventures p.28).
// Returns "" for RobeColor::None.
const char* robeMoonName(RobeColor color);

// Good -> White, Neutral -> Red, Evil -> Black (Dragonlance Adventures
// pp.36-37, "Minimum Requirements" for each Robe).
RobeColor robeForAlignment(Alignment alignment);

} // namespace character
