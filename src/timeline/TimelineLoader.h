#pragma once

#include <string>

namespace timeline {

class Timeline;

// Loads data/timeline.txt into `outTimeline` -- see docs/TIMELINE_NOTES.md
// for the file grammar. Fails fast (file:line: message) on malformed
// input, same philosophy as world::WorldLoader/ZoneLoader.
class TimelineLoader {
public:
    static void loadFromFile(const std::string& path, Timeline& outTimeline);
};

} // namespace timeline
