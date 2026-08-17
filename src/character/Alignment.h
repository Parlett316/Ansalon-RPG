#pragma once

namespace character {

// No alignment restrictions per race/class are enforced this milestone
// (the four core classes are unrestricted in 2e anyway) -- see
// docs/CHARACTER_NOTES.md.
enum class Alignment {
    LawfulGood,
    NeutralGood,
    ChaoticGood,
    LawfulNeutral,
    TrueNeutral,
    ChaoticNeutral,
    LawfulEvil,
    NeutralEvil,
    ChaoticEvil,
};

const char* alignmentName(Alignment a);

} // namespace character
