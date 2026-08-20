#pragma once

#include <string>

namespace quest {

class QuestCatalog;

// Loads data/quests.txt into `outCatalog` -- see docs/QUEST_NOTES.md for the
// file grammar. Fails fast (file:line: message) on malformed input, same
// philosophy as world::WorldLoader/ZoneLoader/timeline::TimelineLoader.
class QuestLoader {
public:
    static void loadFromFile(const std::string& path, QuestCatalog& outCatalog);
};

} // namespace quest
