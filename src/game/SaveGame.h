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

    // Deletes the save file at path, if any. Non-throwing (unlike load()) --
    // returns whether the file is now gone with no error, so a caller can
    // report failure to the player instead of crashing over e.g. a locked
    // file. See main.cpp's save-slot menu, the only caller.
    static bool remove(const std::string& path);

    static void save(const GameState& state, const std::string& path);

    // Throws a runtime_error (with a clear message) on a missing, malformed,
    // or out-of-range save file -- fails fast rather than silently
    // producing a corrupt GameState, same philosophy as WorldLoader/ZoneLoader.
    static GameState load(const std::string& path);
};

} // namespace game
