#include "timeline/TimelineLoader.h"
#include "timeline/Timeline.h"

#include <fstream>
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

} // namespace

void TimelineLoader::loadFromFile(const std::string& path, Timeline& outTimeline) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open timeline data file: " + path);
    }

    CanonCharacter current;
    bool inCharacter = false;
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
            if (current.schedule.empty()) {
                fail(path, lineNumber, "SUBJECT must follow a PRESENCE line in the same CHARACTER block");
            }
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
            std::vector<std::string> keywords;
            std::istringstream kiss(keywordList);
            std::string token;
            while (std::getline(kiss, token, ',')) {
                token = trim(token);
                if (!token.empty()) keywords.push_back(token);
            }
            if (keywords.empty()) {
                fail(path, lineNumber, "SUBJECT has an empty keyword list");
            }
            current.schedule.back().subjects.emplace_back(std::move(keywords), text);
        } else if (keyword == "SUBJECT_UNKNOWN") {
            if (current.schedule.empty()) {
                fail(path, lineNumber, "SUBJECT_UNKNOWN must follow a PRESENCE line in the same CHARACTER block");
            }
            std::string text = trim(rest);
            if (text.empty()) {
                fail(path, lineNumber, "SUBJECT_UNKNOWN is missing its dialogue text");
            }
            current.schedule.back().subjectUnknown = text;
        } else if (keyword == "END") {
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
