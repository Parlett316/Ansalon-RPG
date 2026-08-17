#pragma once

#include "character/Character.h"

namespace character {

// Runs the interactive, text-prompt character creation flow on
// std::cin/std::cout -- deliberately NOT render::Console::readKey(), since
// this is a one-time deliberate wizard before the real-time game loop
// starts, not part of it (see docs/ARCHITECTURE.md). Returns the finished
// Character once the player confirms it.
class CharacterCreator {
public:
    static Character run();
};

} // namespace character
