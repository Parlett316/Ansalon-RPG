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
    std::uniform_int_distribution<int> die(1, sides);
    int total = 0;
    for (int i = 0; i < count; ++i) {
        total += die(rng());
    }
    return total;
}

} // namespace character
