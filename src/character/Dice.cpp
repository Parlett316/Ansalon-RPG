#include "character/Dice.h"

#include <random>

namespace character {

namespace {

// A single engine, seeded once from real entropy, shared by every roll()
// call for the life of the process. Not thread-safe -- the game is
// single-threaded, so this is fine; would need a mutex or thread_local if
// that ever changes.
std::mt19937& rng() {
    static std::mt19937 engine(std::random_device{}());
    return engine;
}

} // namespace

int roll(int count, int sides) {
    // "Roll zero dice" is a real, meaningful call (e.g. data/monsters.txt's
    // STEEL 0 0 0 for a monster that drops no steel -- Timber Wolf,
    // Skeleton, Zombie), not a caller error -- it must return 0 without
    // ever constructing uniform_int_distribution, whose constructor
    // requires min <= max. Constructing it unconditionally with sides
    // possibly 0 crashed the game (debug assertion in <random>) the
    // instant one of those monsters was killed. `sides <= 0` with a
    // nonzero count is still a genuine data bug elsewhere and is left to
    // assert -- only the "no dice at all" case is special-cased here.
    if (count <= 0) return 0;
    std::uniform_int_distribution<int> die(1, sides);
    int total = 0;
    for (int i = 0; i < count; ++i) {
        total += die(rng());
    }
    return total;
}

int roll4d6DropLowest() {
    std::uniform_int_distribution<int> die(1, 6);
    int lowest = die(rng());
    int total = lowest;
    for (int i = 0; i < 3; ++i) {
        int value = die(rng());
        total += value;
        if (value < lowest) lowest = value;
    }
    return total - lowest;
}

} // namespace character
