#pragma once

namespace character {

// Rolls `count` dice with `sides` faces each and returns the sum (e.g.
// roll(3, 6) is "3d6"). Backed by a single process-lifetime random engine
// -- see Dice.cpp.
int roll(int count, int sides);

// PHB p.19 Method V: roll four d6, discard the lowest, sum the remaining
// three. Named for this specific mechanic rather than a generic "roll N
// drop M" helper -- nothing else in the project needs the general case.
// See character::CharacterCreator::run and docs/CHARACTER_NOTES.md.
int roll4d6DropLowest();

} // namespace character
