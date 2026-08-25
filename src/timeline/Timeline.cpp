#include "timeline/Timeline.h"

namespace timeline {

void Timeline::addCharacter(CanonCharacter character) {
    characters_.push_back(std::move(character));
}

std::vector<Presence> Timeline::presentAt(const std::string& locationId, int day) const {
    std::vector<Presence> result;
    for (const auto& character : characters_) {
        for (const auto& window : character.schedule) {
            if (window.locationId == locationId && day >= window.dayStart && day <= window.dayEnd) {
                result.push_back(Presence{&character, &window});
                break; // don't list the same character twice for overlapping windows
            }
        }
    }
    return result;
}

int Timeline::latestDayEnd(const std::string& locationId) const {
    int latest = -1;
    for (const auto& character : characters_) {
        for (const auto& window : character.schedule) {
            if (window.locationId == locationId && window.dayEnd > latest) {
                latest = window.dayEnd;
            }
        }
    }
    return latest;
}

int Timeline::earliestDayStart(const std::string& locationId) const {
    int earliest = -1;
    for (const auto& character : characters_) {
        for (const auto& window : character.schedule) {
            if (window.locationId == locationId && (earliest < 0 || window.dayStart < earliest)) {
                earliest = window.dayStart;
            }
        }
    }
    return earliest;
}

std::vector<Subject> Timeline::subjectsFor(const CanonCharacter& character, const PresenceWindow& window,
                                            int day) const {
    std::vector<Subject> result;
    for (const auto& [keywords, text] : window.subjects) {
        result.push_back(Subject{keywords, text});
    }
    for (const auto& subject : character.subjects) {
        if (day >= subject.dayStart && (subject.dayEnd == -1 || day <= subject.dayEnd)) {
            result.push_back(Subject{subject.keywords, subject.text});
        }
    }
    return result;
}

std::string Timeline::subjectUnknownFor(const CanonCharacter& character, const PresenceWindow& window) const {
    if (!window.subjectUnknown.empty()) return window.subjectUnknown;
    return character.subjectUnknown;
}

} // namespace timeline
