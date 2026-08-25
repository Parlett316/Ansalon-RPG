#pragma once

#include <string>

namespace timeline {

class Timeline;

// Loads data/timeline.txt into `outTimeline` -- see docs/TIMELINE_NOTES.md
// for the file grammar. Fails fast (file:line: message) on malformed
// input, same philosophy as world::WorldLoader/ZoneLoader.
//
// `reportCollisionWarnings` (default off) enables the advisory
// SUBJECT/SUBJECT_WHEN keyword-collision warnings described in
// docs/TIMELINE_NOTES.md's "Keyword-collision warning" -- content-author
// diagnostics, not something a player should see on every launch. Pass
// `true` (main.cpp's `--check-timeline` flag) when deliberately checking
// timeline.txt after an edit.
class TimelineLoader {
public:
    static void loadFromFile(const std::string& path, Timeline& outTimeline,
                              bool reportCollisionWarnings = false);
};

} // namespace timeline
