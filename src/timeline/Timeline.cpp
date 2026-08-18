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

} // namespace timeline
