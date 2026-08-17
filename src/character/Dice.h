#pragma once

namespace character {

// Rolls `count` dice with `sides` faces each and returns the sum (e.g.
// roll(3, 6) is "3d6"). Backed by a single process-lifetime random engine
// -- see Dice.cpp.
int roll(int count, int sides);

} // namespace character
