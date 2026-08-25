#include "timeline/TimelineLoader.h"
#include "timeline/Timeline.h"

#include <cctype>
#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace timeline {

namespace {

// Same trim/splitKeyword/fail idioms as world::WorldLoader/ZoneLoader --
// each loader in this project owns its own tiny copy rather than sharing a
// utility file (see docs/ARCHITECTURE.md).
std::string trim(const std::string& s) {
    size_t begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

std::pair<std::string, std::string> splitKeyword(const std::string& line) {
    size_t sp = line.find_first_of(" \t");
    if (sp == std::string::npos) return {line, ""};
    return {line.substr(0, sp), trim(line.substr(sp + 1))};
}

[[noreturn]] void fail(const std::string& path, int lineNumber, const std::string& message) {
    throw std::runtime_error(path + ":" + std::to_string(lineNumber) + ": " + message);
}

// Non-fatal counterpart to fail() -- see "Keyword-collision reporting"
// below. Same file:line prefix as fail()'s message so it reads like the
// rest of the loader's output, just routed to stderr instead of thrown.
void warn(const std::string& path, int lineNumber, const std::string& message) {
    std::cerr << path << ":" << lineNumber << ": warning: " << message << "\n";
}

// Shared by SUBJECT and SUBJECT_WHEN: splits a comma-separated keyword-alias
// token into its individual keywords, failing fast on an empty list. `label`
// names the calling keyword for the error message.
std::vector<std::string> parseKeywordList(const std::string& path, int lineNumber, const std::string& label,
                                           const std::string& keywordList) {
    std::vector<std::string> keywords;
    std::istringstream kiss(keywordList);
    std::string token;
    while (std::getline(kiss, token, ',')) {
        token = trim(token);
        if (!token.empty()) keywords.push_back(token);
    }
    if (keywords.empty()) {
        fail(path, lineNumber, label + " has an empty keyword list");
    }
    return keywords;
}

// One SUBJECT/SUBJECT_WHEN entry (window- or character-level) tracked while
// parsing a single CHARACTER block, purely so the block's END handler can
// run the keyword-collision check below -- not kept once that check runs.
struct SubjectRecord {
    std::vector<std::string> keywords; // lowercased, for case-insensitive comparison
    int dayStart;
    int dayEnd; // -1 = open-ended
    int lineNumber;
};

std::string lowercase(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

bool dayRangesOverlap(int aStart, int aEnd, int bStart, int bEnd) {
    int aEndEff = aEnd == -1 ? INT_MAX : aEnd;
    int bEndEff = bEnd == -1 ? INT_MAX : bEnd;
    return aStart <= bEndEff && bStart <= aEndEff;
}

// Section 4.5 of docs/MILESTONE_72_SPEC.md: at three subjects, keyword
// ordering is invisible; at twenty-five it's load-bearing and fragile (a
// player typing "dragon orb" tokenizes to "dragon" and "orb", and "dragon"
// alone is reachable from dragon orbs, Khisanth, Cyan Bloodbane, and
// draconians alike). Reports -- doesn't fail on -- any keyword reachable
// from two entries whose day ranges overlap; a window subject overlapping
// its own window's days is exactly the deliberate override mechanism
// (docs/TIMELINE_NOTES.md), not a bug, so this stays advisory. Authoring
// rule: order specific before general (e.g. "orb"/"orbs" before any entry
// claiming "dragon").
void reportKeywordCollisions(const std::string& path, const std::vector<SubjectRecord>& records) {
    for (size_t i = 0; i < records.size(); ++i) {
        for (size_t j = i + 1; j < records.size(); ++j) {
            if (!dayRangesOverlap(records[i].dayStart, records[i].dayEnd, records[j].dayStart, records[j].dayEnd)) {
                continue;
            }
            for (const std::string& kw : records[i].keywords) {
                bool shared = false;
                for (const std::string& other : records[j].keywords) {
                    if (kw == other) {
                        shared = true;
                        break;
                    }
                }
                if (shared) {
                    warn(path, records[j].lineNumber,
                         "keyword '" + kw + "' is also reachable from the SUBJECT at line " +
                             std::to_string(records[i].lineNumber) +
                             " (overlapping day ranges) -- order specific keywords before general ones");
                    break; // one warning per colliding pair is enough
                }
            }
        }
    }
}

} // namespace

void TimelineLoader::loadFromFile(const std::string& path, Timeline& outTimeline,
                                   bool reportCollisionWarnings) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open timeline data file: " + path);
    }

    CanonCharacter current;
    bool inCharacter = false;
    // SUBJECT/SUBJECT_WHEN entries seen so far in the current CHARACTER
    // block (window- and character-level alike), reset per block -- feeds
    // the keyword-collision check run at that block's END. See
    // reportKeywordCollisions above.
    std::vector<SubjectRecord> subjectRecords;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        auto [keyword, rest] = splitKeyword(trimmed);

        if (keyword == "CHARACTER") {
            if (inCharacter) fail(path, lineNumber, "found CHARACTER before the previous block's END");
            current = CanonCharacter{};
            current.id = rest;
            inCharacter = true;
            subjectRecords.clear();
        } else if (!inCharacter) {
            fail(path, lineNumber, "'" + keyword + "' outside of a CHARACTER/END block");
        } else if (keyword == "NAME") {
            current.name = rest;
        } else if (keyword == "PRESENCE") {
            std::istringstream iss(rest);
            std::string locationId;
            int dayStart = 0;
            int dayEnd = 0;
            if (!(iss >> locationId >> dayStart >> dayEnd)) {
                fail(path, lineNumber,
                     "malformed PRESENCE (expected: PRESENCE <location-id> <day-start> <day-end> <text>)");
            }
            if (dayStart > dayEnd) {
                fail(path, lineNumber, "PRESENCE day-start must be <= day-end");
            }
            std::string flavor;
            std::getline(iss, flavor);
            flavor = trim(flavor);
            if (flavor.empty()) {
                fail(path, lineNumber, "PRESENCE is missing its flavor text");
            }
            current.schedule.push_back(PresenceWindow{locationId, dayStart, dayEnd, flavor, ""});
        } else if (keyword == "SAY") {
            if (current.schedule.empty()) {
                fail(path, lineNumber, "SAY must follow a PRESENCE line in the same CHARACTER block");
            }
            std::string dialogue = trim(rest);
            if (dialogue.empty()) {
                fail(path, lineNumber, "SAY is missing its dialogue text");
            }
            current.schedule.back().dialogue = dialogue;
        } else if (keyword == "SAY_IF") {
            if (current.schedule.empty()) {
                fail(path, lineNumber, "SAY_IF must follow a PRESENCE line in the same CHARACTER block");
            }
            std::istringstream iss(rest);
            std::string condition;
            if (!(iss >> condition)) {
                fail(path, lineNumber, "malformed SAY_IF (expected: SAY_IF <condition> <text>)");
            }
            std::string text;
            std::getline(iss, text);
            text = trim(text);
            if (text.empty()) {
                fail(path, lineNumber, "SAY_IF is missing its dialogue text");
            }
            current.schedule.back().conditionalDialogue.emplace_back(condition, text);
        } else if (keyword == "SAY_AGAIN") {
            if (current.schedule.empty()) {
                fail(path, lineNumber, "SAY_AGAIN must follow a PRESENCE line in the same CHARACTER block");
            }
            std::string text = trim(rest);
            if (text.empty()) {
                fail(path, lineNumber, "SAY_AGAIN is missing its dialogue text");
            }
            current.schedule.back().dialogueAgain = text;
        } else if (keyword == "TOPIC") {
            if (current.schedule.empty()) {
                fail(path, lineNumber, "TOPIC must follow a PRESENCE line in the same CHARACTER block");
            }
            // Quoted label (may contain spaces, e.g. "The Blue Crystal Staff"),
            // same convention as ZoneLoader's POI name -- everything after
            // the closing quote is the topic's text.
            std::string quoted = trim(rest);
            if (quoted.empty() || quoted.front() != '"') {
                fail(path, lineNumber, "TOPIC label must be quoted, e.g. TOPIC \"The Staff\" text");
            }
            size_t closeQuote = quoted.find('"', 1);
            if (closeQuote == std::string::npos) {
                fail(path, lineNumber, "TOPIC label is missing its closing quote");
            }
            std::string label = quoted.substr(1, closeQuote - 1);
            std::string text = trim(quoted.substr(closeQuote + 1));
            if (text.empty()) {
                fail(path, lineNumber, "TOPIC is missing its text");
            }
            current.schedule.back().topics.emplace_back(label, text);
        } else if (keyword == "SUBJECT") {
            // Before the first PRESENCE: character-level, equivalent to
            // SUBJECT_WHEN 0 -1 (see docs/TIMELINE_NOTES.md). After one:
            // scoped to that window, unchanged from Milestone 71.
            std::istringstream iss(rest);
            std::string keywordList;
            if (!(iss >> keywordList)) {
                fail(path, lineNumber, "malformed SUBJECT (expected: SUBJECT <keyword1,keyword2,...> <text>)");
            }
            std::string text;
            std::getline(iss, text);
            text = trim(text);
            if (text.empty()) {
                fail(path, lineNumber, "SUBJECT is missing its dialogue text");
            }
            std::vector<std::string> keywords = parseKeywordList(path, lineNumber, "SUBJECT", keywordList);
            std::vector<std::string> lowered;
            for (const std::string& k : keywords) lowered.push_back(lowercase(k));
            int dayStart = 0;
            int dayEnd = -1;
            if (current.schedule.empty()) {
                current.subjects.push_back(CharacterSubject{std::move(keywords), text, dayStart, dayEnd});
            } else {
                dayStart = current.schedule.back().dayStart;
                dayEnd = current.schedule.back().dayEnd;
                current.schedule.back().subjects.emplace_back(std::move(keywords), text);
            }
            subjectRecords.push_back(SubjectRecord{std::move(lowered), dayStart, dayEnd, lineNumber});
        } else if (keyword == "SUBJECT_WHEN") {
            // Character-level only -- an error after the first PRESENCE,
            // since a window is already day-scoped (see
            // docs/TIMELINE_NOTES.md).
            if (!current.schedule.empty()) {
                fail(path, lineNumber, "SUBJECT_WHEN must appear before the first PRESENCE line (character-level only)");
            }
            std::istringstream iss(rest);
            int dayStart = 0;
            int dayEnd = 0;
            std::string keywordList;
            if (!(iss >> dayStart >> dayEnd >> keywordList)) {
                fail(path, lineNumber,
                     "malformed SUBJECT_WHEN (expected: SUBJECT_WHEN <day-start> <day-end> "
                     "<keyword1,keyword2,...> <text>)");
            }
            if (dayStart < 0) {
                fail(path, lineNumber, "SUBJECT_WHEN day-start must be >= 0");
            }
            if (dayEnd != -1 && dayEnd < dayStart) {
                fail(path, lineNumber, "SUBJECT_WHEN day-end must be >= day-start, or -1 for open-ended");
            }
            std::string text;
            std::getline(iss, text);
            text = trim(text);
            if (text.empty()) {
                fail(path, lineNumber, "SUBJECT_WHEN is missing its dialogue text");
            }
            std::vector<std::string> keywords = parseKeywordList(path, lineNumber, "SUBJECT_WHEN", keywordList);
            std::vector<std::string> lowered;
            for (const std::string& k : keywords) lowered.push_back(lowercase(k));
            subjectRecords.push_back(SubjectRecord{std::move(lowered), dayStart, dayEnd, lineNumber});
            current.subjects.push_back(CharacterSubject{std::move(keywords), text, dayStart, dayEnd});
        } else if (keyword == "SUBJECT_UNKNOWN") {
            std::string text = trim(rest);
            if (text.empty()) {
                fail(path, lineNumber, "SUBJECT_UNKNOWN is missing its dialogue text");
            }
            if (current.schedule.empty()) {
                if (!current.subjectUnknown.empty()) {
                    fail(path, lineNumber, "duplicate character-level SUBJECT_UNKNOWN in this CHARACTER block");
                }
                current.subjectUnknown = text;
            } else {
                current.schedule.back().subjectUnknown = text;
            }
        } else if (keyword == "END") {
            if (reportCollisionWarnings) {
                reportKeywordCollisions(path, subjectRecords);
            }
            outTimeline.addCharacter(std::move(current));
            inCharacter = false;
        } else {
            fail(path, lineNumber, "unknown keyword '" + keyword + "'");
        }
    }

    if (inCharacter) {
        fail(path, lineNumber + 1, "reached end of file inside a CHARACTER block missing END");
    }
}

} // namespace timeline
