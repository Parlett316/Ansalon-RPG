#pragma once

#include "game/GameState.h"

#include <string>

namespace game {

// Serializes/deserializes GameState (including the embedded Character) to a
// single hand-rolled plain-text save file -- see docs/ARCHITECTURE.md for
// why this is the whole save system (GameState was deliberately kept flat
// and serializable from the start) and docs/GOTCHAS.md for format
// limitations (raw enum ints, no cross-checking against World/ZoneCatalog).
class SaveGame {
public:
    static bool exists(const std::string& path);

    static void save(const GameState& state, const std::string& path);

    // Throws a runtime_error (with a clear message) on a missing, malformed,
    // or out-of-range save file -- fails fast rather than silently
    // producing a corrupt GameState, same philosophy as WorldLoader/ZoneLoader.
    static GameState load(const std::string& path);
};

} // namespace game
