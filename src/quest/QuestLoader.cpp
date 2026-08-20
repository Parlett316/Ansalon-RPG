#include "quest/QuestLoader.h"
#include "quest/Quest.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace quest {

namespace {

// Same trim/splitKeyword/fail idioms as every other loader in this project
// -- each one owns its own tiny copy rather than sharing a utility file
// (see docs/ARCHITECTURE.md).
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

// Parses "<targetId> <label...>" for VISIT/TALK -- the label is everything
// after the first token, unquoted (no ambiguity, unlike TOPIC's label,
// since there's no keyword that could follow it on the same line).
void parseIdAndLabel(const std::string& path, int lineNumber, const std::string& keyword,
                      const std::string& rest, std::string& outId, std::string& outLabel) {
    std::istringstream iss(rest);
    if (!(iss >> outId)) {
        fail(path, lineNumber, "malformed " + keyword + " (expected: " + keyword + " <id> <label>)");
    }
    std::getline(iss, outLabel);
    outLabel = trim(outLabel);
    if (outLabel.empty()) {
        fail(path, lineNumber, keyword + " is missing its journal label");
    }
}

} // namespace

void QuestLoader::loadFromFile(const std::string& path, QuestCatalog& outCatalog) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open quest data file: " + path);
    }

    Quest current;
    bool inQuest = false;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        auto [keyword, rest] = splitKeyword(trimmed);

        if (keyword == "QUEST") {
            if (inQuest) fail(path, lineNumber, "found QUEST before the previous block's END");
            if (rest.empty()) fail(path, lineNumber, "QUEST is missing its id");
            if (outCatalog.find(rest) != nullptr) {
                fail(path, lineNumber, "duplicate quest id '" + rest + "'");
            }
            current = Quest{};
            current.id = rest;
            inQuest = true;
        } else if (!inQuest) {
            fail(path, lineNumber, "'" + keyword + "' outside of a QUEST/END block");
        } else if (keyword == "NAME") {
            current.name = rest;
        } else if (keyword == "OFFER") {
            current.offerText = rest;
        } else if (keyword == "ACCEPT") {
            current.acceptText = rest;
        } else if (keyword == "PROGRESS") {
            current.progressText = rest;
        } else if (keyword == "COMPLETE") {
            current.completeText = rest;
        } else if (keyword == "GIVER") {
            if (rest.empty()) fail(path, lineNumber, "GIVER is missing its text");
            current.giver = rest;
        } else if (keyword == "REQUIRE") {
            if (rest.empty()) fail(path, lineNumber, "REQUIRE is missing its condition");
            current.requirement = rest;
        } else if (keyword == "VISIT") {
            Objective obj;
            obj.kind = ObjectiveKind::Visit;
            parseIdAndLabel(path, lineNumber, keyword, rest, obj.targetId, obj.label);
            current.objectives.push_back(std::move(obj));
        } else if (keyword == "TALK") {
            Objective obj;
            obj.kind = ObjectiveKind::Talk;
            parseIdAndLabel(path, lineNumber, keyword, rest, obj.targetId, obj.label);
            current.objectives.push_back(std::move(obj));
        } else if (keyword == "SLAY") {
            std::istringstream iss(rest);
            Objective obj;
            obj.kind = ObjectiveKind::Slay;
            if (!(iss >> obj.targetId >> obj.count)) {
                fail(path, lineNumber, "malformed SLAY (expected: SLAY <monster-id> <count> <label>)");
            }
            if (obj.count < 1) {
                fail(path, lineNumber, "SLAY count must be at least 1");
            }
            std::getline(iss, obj.label);
            obj.label = trim(obj.label);
            if (obj.label.empty()) {
                fail(path, lineNumber, "SLAY is missing its journal label");
            }
            current.objectives.push_back(std::move(obj));
        } else if (keyword == "REWARD_STEEL") {
            std::istringstream iss(rest);
            if (!(iss >> current.rewardSteel)) {
                fail(path, lineNumber, "malformed REWARD_STEEL (expected: REWARD_STEEL <n>)");
            }
        } else if (keyword == "REWARD_XP") {
            std::istringstream iss(rest);
            if (!(iss >> current.rewardXp)) {
                fail(path, lineNumber, "malformed REWARD_XP (expected: REWARD_XP <n>)");
            }
        } else if (keyword == "REWARD_KNIGHT_SWORD") {
            if (!rest.empty()) fail(path, lineNumber, "REWARD_KNIGHT_SWORD takes no argument");
            current.rewardKnightSword = true;
        } else if (keyword == "REWARD_SOLAMNIC_ARMOR") {
            if (!rest.empty()) fail(path, lineNumber, "REWARD_SOLAMNIC_ARMOR takes no argument");
            current.rewardSolamnicArmor = true;
        } else if (keyword == "REWARD_KNIGHT_ROSE") {
            if (!rest.empty()) fail(path, lineNumber, "REWARD_KNIGHT_ROSE takes no argument");
            current.rewardKnightRose = true;
        } else if (keyword == "END") {
            if (current.name.empty()) fail(path, lineNumber, "quest is missing its NAME");
            if (current.offerText.empty()) fail(path, lineNumber, "quest is missing its OFFER text");
            if (current.acceptText.empty()) fail(path, lineNumber, "quest is missing its ACCEPT text");
            if (current.progressText.empty()) fail(path, lineNumber, "quest is missing its PROGRESS text");
            if (current.completeText.empty()) fail(path, lineNumber, "quest is missing its COMPLETE text");
            if (current.giver.empty()) fail(path, lineNumber, "quest is missing its GIVER text");
            if (current.objectives.empty()) {
                fail(path, lineNumber, "quest has no objectives (need at least one VISIT/TALK/SLAY)");
            }
            outCatalog.addQuest(std::move(current));
            inQuest = false;
        } else {
            fail(path, lineNumber, "unknown keyword '" + keyword + "'");
        }
    }

    if (inQuest) {
        fail(path, lineNumber + 1, "reached end of file inside a QUEST block missing END");
    }
}

} // namespace quest
