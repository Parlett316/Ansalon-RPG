#pragma once

#include <string>
#include <utility>
#include <vector>

namespace timeline {

// One stretch of in-game days during which a canon character is at a
// given overworld Location -- see docs/TIMELINE_NOTES.md.
struct PresenceWindow {
    std::string locationId;
    int dayStart = 0;
    int dayEnd = 0; // inclusive
    std::string flavorText;
    // Spoken line shown when the player presses 't' (talk) while standing
    // at locationId during this window -- empty means nothing to say yet.
    // Set via a SAY line immediately following this window's PRESENCE line
    // in data/timeline.txt -- see docs/TIMELINE_NOTES.md.
    std::string dialogue;
    // Reactive variants of `dialogue`, checked in order against the
    // player's character (race/class/alignment) the first time this
    // window is talked to -- the first condition that matches wins over
    // the plain `dialogue`. Set via SAY_IF lines. world::Timeline stays
    // decoupled from character::Character -- these are raw condition
    // strings, evaluated by game::GameLoop, not here.
    std::vector<std::pair<std::string, std::string>> conditionalDialogue;
    // Shown instead of the generic recognition fallback on every talk
    // after the first -- empty means fall back to that generic line. Set
    // via an optional SAY_AGAIN line.
    std::string dialogueAgain;
    // Topics offered after the greeting, in authored order -- empty means
    // no topic menu at all. Set via zero or more TOPIC lines.
    std::vector<std::pair<std::string, std::string>> topics;
    // Free-text "ask about..." subjects, in authored order -- each pairs a
    // comma-separated keyword-alias list with its dialogue text. Empty means
    // no free-text asking is offered for this window. Set via zero or more
    // SUBJECT lines; game::GameLoop does the actual keyword matching (see
    // game::matchSubject) so this stays a plain data holder, same
    // decoupling reasoning as conditionalDialogue above. See
    // docs/TIMELINE_NOTES.md's "Ask about anything".
    std::vector<std::pair<std::vector<std::string>, std::string>> subjects;
    // Shown when a free-typed subject matches none of the above -- empty
    // means fall back to game::GameLoop::talkTo's generic line. Set via an
    // optional SUBJECT_UNKNOWN line.
    std::string subjectUnknown;
};

// A character-level free-text-askable subject, scoped to an in-game day
// range -- see docs/TIMELINE_NOTES.md's "Ask about anything". Set via
// SUBJECT (dayStart=0, dayEnd=-1, i.e. always available) or SUBJECT_WHEN
// (explicit range) before the first PRESENCE line in a CHARACTER block.
// dayEnd of -1 means open-ended, same sentinel latestDayEnd/earliestDayStart
// already use for "no such day".
struct CharacterSubject {
    std::vector<std::string> keywords;
    std::string text;
    int dayStart = 0;
    int dayEnd = -1;
};

struct CanonCharacter {
    std::string id;
    std::string name;
    std::vector<PresenceWindow> schedule;
    // Character-level subjects (SUBJECT/SUBJECT_WHEN before the first
    // PRESENCE line) -- available at every window whose current day falls
    // in range, layered underneath that window's own subjects. See
    // Timeline::subjectsFor and docs/TIMELINE_NOTES.md.
    std::vector<CharacterSubject> subjects;
    // Character-level SUBJECT_UNKNOWN -- the fallback used when a window
    // has no SUBJECT_UNKNOWN of its own. Empty means no character-level
    // fallback either (falls through to game::GameLoop::talkTo's generic
    // line). See Timeline::subjectUnknownFor.
    std::string subjectUnknown;
};

// One free-text subject as resolved for a specific window/day by
// Timeline::subjectsFor -- window-level and character-level subjects share
// this shape once resolved, since day-range filtering has already happened
// by the time a caller sees one. See docs/TIMELINE_NOTES.md's "Ask about
// anything".
struct Subject {
    std::vector<std::string> keywords;
    std::string text;
};

// One canon character's presence, as returned by Timeline::presentAt --
// bundles the character with the specific PresenceWindow that matched, so
// callers don't have to re-scan the schedule to find the flavor text.
struct Presence {
    const CanonCharacter* character = nullptr;
    const PresenceWindow* window = nullptr;
};

// A hand-authored schedule of where the canon Heroes of the Lance (and
// others, later) are on which in-game day -- the "chance encounter" engine
// the whole project was originally pitched around (see README.md). Static
// content, loaded once at startup by TimelineLoader and never mutated
// during play, same treatment as world::World/OverworldGrid -- only
// game::GameState's position/hoursElapsed determine what's currently
// showing, so nothing here needs to be saved.
class Timeline {
public:
    void addCharacter(CanonCharacter character);

    // Every canon character whose schedule places them at `locationId` on
    // `day` -- normally 0 or 1 results, but not assumed to be.
    std::vector<Presence> presentAt(const std::string& locationId, int day) const;

    // The latest dayEnd across every PresenceWindow scheduled at
    // `locationId`, or -1 if no character's schedule ever visits it at all --
    // lets a caller ask "have the Heroes fully moved on from here?" (day >
    // this value) without re-scanning every character's schedule itself. See
    // docs/ZONE_NOTES.md's "Aftermath dialogue" section for the feature this
    // supports.
    int latestDayEnd(const std::string& locationId) const;

    // The earliest dayStart across every PresenceWindow scheduled at
    // `locationId`, or -1 if no character's schedule ever visits it at all --
    // the mirror-image query to latestDayEnd, letting a caller ask "have the
    // Heroes not arrived here yet?" (day < this value). See
    // docs/ZONE_NOTES.md's "Anticipation dialogue" section for the feature
    // this supports.
    int earliestDayStart(const std::string& locationId) const;

    // The free-text subjects available while talking to `character` during
    // `window`, on in-game day `day`: that window's own SUBJECT entries
    // (authored order), then the character's SUBJECT/SUBJECT_WHEN entries
    // whose day range contains `day` (authored order) -- a window SUBJECT
    // sharing a keyword with a pool entry wins, since game::matchSubject
    // checks this list in order and stops at the first match. See
    // docs/TIMELINE_NOTES.md's "Ask about anything".
    std::vector<Subject> subjectsFor(const CanonCharacter& character, const PresenceWindow& window, int day) const;

    // `window`'s own SUBJECT_UNKNOWN if it has one, else `character`'s
    // character-level SUBJECT_UNKNOWN, else empty (falls back to
    // game::GameLoop::talkTo's generic line).
    std::string subjectUnknownFor(const CanonCharacter& character, const PresenceWindow& window) const;

private:
    std::vector<CanonCharacter> characters_;
};

} // namespace timeline
