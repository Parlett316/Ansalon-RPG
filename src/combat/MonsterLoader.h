#pragma once

#include <string>

namespace combat {

class MonsterCatalog;

// Loads data/monsters.txt into `outCatalog` -- see docs/COMBAT_NOTES.md
// for the file grammar. Fails fast (file:line: message) on malformed
// input, same philosophy as every other loader in this project.
class MonsterLoader {
public:
    static void loadFromFile(const std::string& path, MonsterCatalog& outCatalog);
};

} // namespace combat
