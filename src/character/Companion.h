#pragma once

#include "character/Character.h"

namespace character {

// A single, hand-authored recruitable party companion -- Milestone 116
// Phase 1's first, deliberately small step toward a real party (see
// docs/COMBAT_NOTES.md's "Extending this later" section for the full,
// much bigger gap this doesn't attempt to close yet: multiple companions,
// player-directed control in combat, deployment order, UIC). Returns a
// fully-formed level-1 Character using FIXED ability scores and starting
// steel (not character::roll) -- deliberately deterministic, so
// game::SaveGame can reconstruct an identical companion on load from a
// single "was one recruited" flag instead of serializing every field. See
// docs/ARCHITECTURE.md's "Party companions" section.
Character buildCompanion();

} // namespace character
