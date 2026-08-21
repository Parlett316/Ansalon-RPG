#include "world/ZoneLoader.h"
#include "world/Zone.h"
#include "world/ZoneTile.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace world {

namespace {

std::string trim(const std::string& s) {
    size_t begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

std::string stripCR(const std::string& s) {
    if (!s.empty() && s.back() == '\r') return s.substr(0, s.size() - 1);
    return s;
}

std::pair<std::string, std::string> splitKeyword(const std::string& line) {
    size_t sp = line.find_first_of(" \t");
    if (sp == std::string::npos) return {line, ""};
    return {line.substr(0, sp), trim(line.substr(sp + 1))};
}

[[noreturn]] void fail(const std::string& path, int lineNumber, const std::string& message) {
    throw std::runtime_error(path + ":" + std::to_string(lineNumber) + ": " + message);
}

// Parses `<char> "<name>" <description...>` as used by POI lines -- same
// quoted-field convention as CONNECT road descriptions in Milestone 1.
PointOfInterest parsePoi(const std::string& rest, const std::string& path, int lineNumber) {
    std::istringstream iss(rest);
    std::string codeToken;
    if (!(iss >> codeToken) || codeToken.size() != 1) {
        fail(path, lineNumber, "malformed POI (expected: POI <char> \"name\" description)");
    }
    std::string remainder;
    std::getline(iss, remainder);
    remainder = trim(remainder);
    if (remainder.empty() || remainder.front() != '"') {
        fail(path, lineNumber, "POI name must be quoted, e.g. POI I \"Inn of the Last Home\" ...");
    }
    size_t closeQuote = remainder.find('"', 1);
    if (closeQuote == std::string::npos) {
        fail(path, lineNumber, "POI name is missing its closing quote");
    }
    PointOfInterest poi;
    poi.code = codeToken[0];
    poi.name = remainder.substr(1, closeQuote - 1);
    poi.description = trim(remainder.substr(closeQuote + 1));
    return poi;
}

} // namespace

Zone ZoneLoader::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open zone file: " + path);
    }

    std::string name;
    int entryX = -1;
    int entryY = -1;
    std::vector<std::string> gridRows;
    std::unordered_map<char, PointOfInterest> pois;
    std::unordered_map<char, std::string> portals;
    // TALK lines are applied to `pois` after the whole file is parsed (a
    // TALK can't be required to appear after its POI line any more than
    // PORTAL currently is) -- keyed by code, value is {dialogue, the line
    // number TALK appeared on, for a clear error if its POI never shows up}.
    std::unordered_map<char, std::pair<std::string, int>> talkLines;
    // TALK_AGAIN gets the exact same "applied after the whole file is
    // parsed, must reference an already-declared POI" treatment as talkLines.
    std::unordered_map<char, std::pair<std::string, int>> talkAgainLines;
    // TALK_AFTER gets the same treatment as TALK_AGAIN, plus the extra
    // "must already have a TALK line" validation SAY_IF/TOPIC/BOAT require --
    // see docs/ZONE_NOTES.md's "Aftermath dialogue" section.
    std::unordered_map<char, std::pair<std::string, int>> talkAfterLines;
    // TALK_BEFORE gets the exact same treatment as TALK_AFTER -- see
    // docs/ZONE_NOTES.md's "Anticipation dialogue" section.
    std::unordered_map<char, std::pair<std::string, int>> talkBeforeLines;
    // Same "applied after the whole file is parsed" treatment as talkLines
    // above -- keyed by code, value is the line number SHOP appeared on,
    // for a clear error if its POI never shows up.
    std::unordered_map<char, int> shopLines;
    // Same "applied after the whole file is parsed" treatment as shopLines
    // above -- keyed by code, value is the line number BOAT appeared on.
    std::unordered_map<char, int> boatLines;
    // Same "applied after the whole file is parsed, must reference an
    // already-declared POI with a TALK line" treatment as boatLines above.
    // Value is {item-id, display-name, the line number GRANTS_ITEM appeared on}.
    std::unordered_map<char, std::tuple<std::string, std::string, int>> grantsItemLines;
    // Same "applied after the whole file is parsed" treatment as shopLines
    // above -- keyed by code, value is the line number BED appeared on.
    std::unordered_map<char, int> bedLines;
    // SAY_IF/TOPIC get the same "collected by POI char, applied after the
    // whole file is parsed" treatment as talkLines/shopLines above -- but
    // unlike those (one dialogue per POI), a POI can have zero or more of
    // each, so every entry keeps its own line number for a precise error if
    // its POI never shows up or has no TALK line to react against.
    std::unordered_map<char, std::vector<std::tuple<std::string, std::string, int>>> sayIfLines;
    std::unordered_map<char, std::vector<std::tuple<std::string, std::string, int>>> topicLines;
    // TIMELINE_ANCHOR -- see docs/TIMELINE_NOTES.md. At most one per zone;
    // validated against declared POIs after the whole file is parsed, same
    // as TALK/SHOP/PORTAL above.
    char timelineAnchorCode = '\0';
    int timelineAnchorLine = -1;
    std::string timelineLocationId; // TIMELINE_LOCATION -- optional, no positional dependency
    // QUEST -- same "applied after the whole file is parsed, must reference
    // an already-declared POI with a TALK line" treatment as BOAT above.
    // Value is {quest-id, the line number QUEST appeared on}.
    std::unordered_map<char, std::pair<std::string, int>> questLines;
    int gridWidth = -1;

    // GRID/ENDGRID is a literal raw-text block embedded inside an otherwise
    // keyword-per-line file: every line between them is taken as-is (no
    // trimming, no keyword parsing) until a line reads exactly "ENDGRID".
    enum class State { Header, InGrid, Footer, Done };
    State state = State::Header;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        line = stripCR(line);

        if (state == State::InGrid) {
            if (line == "ENDGRID") {
                state = State::Footer;
                continue;
            }
            if (gridWidth == -1) {
                gridWidth = static_cast<int>(line.size());
            } else if (static_cast<int>(line.size()) != gridWidth) {
                fail(path, lineNumber, "GRID row has inconsistent width (expected " +
                                            std::to_string(gridWidth) + ", got " +
                                            std::to_string(line.size()) + ")");
            }
            gridRows.push_back(line);
            continue;
        }

        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        auto [keyword, rest] = splitKeyword(trimmed);

        if (state == State::Header) {
            if (keyword == "NAME") {
                name = rest;
            } else if (keyword == "ENTRY") {
                std::istringstream iss(rest);
                if (!(iss >> entryX >> entryY)) {
                    fail(path, lineNumber, "malformed ENTRY (expected: ENTRY x y)");
                }
            } else if (keyword == "GRID") {
                state = State::InGrid;
            } else {
                fail(path, lineNumber, "unexpected '" + keyword + "' before GRID");
            }
        } else if (state == State::Footer) {
            if (keyword == "POI") {
                PointOfInterest poi = parsePoi(rest, path, lineNumber);
                pois[poi.code] = poi;
            } else if (keyword == "PORTAL") {
                std::istringstream iss(rest);
                std::string codeToken, targetId;
                if (!(iss >> codeToken >> targetId) || codeToken.size() != 1) {
                    fail(path, lineNumber, "malformed PORTAL (expected: PORTAL <char> <zone-id>)");
                }
                portals[codeToken[0]] = targetId;
            } else if (keyword == "TALK") {
                std::istringstream iss(rest);
                std::string codeToken;
                if (!(iss >> codeToken) || codeToken.size() != 1) {
                    fail(path, lineNumber, "malformed TALK (expected: TALK <char> dialogue...)");
                }
                std::string dialogue;
                std::getline(iss, dialogue);
                dialogue = trim(dialogue);
                if (dialogue.empty()) {
                    fail(path, lineNumber, "TALK is missing its dialogue text");
                }
                talkLines[codeToken[0]] = {dialogue, lineNumber};
            } else if (keyword == "TALK_AGAIN") {
                std::istringstream iss(rest);
                std::string codeToken;
                if (!(iss >> codeToken) || codeToken.size() != 1) {
                    fail(path, lineNumber, "malformed TALK_AGAIN (expected: TALK_AGAIN <char> dialogue...)");
                }
                std::string dialogue;
                std::getline(iss, dialogue);
                dialogue = trim(dialogue);
                if (dialogue.empty()) {
                    fail(path, lineNumber, "TALK_AGAIN is missing its dialogue text");
                }
                talkAgainLines[codeToken[0]] = {dialogue, lineNumber};
            } else if (keyword == "TALK_AFTER") {
                std::istringstream iss(rest);
                std::string codeToken;
                if (!(iss >> codeToken) || codeToken.size() != 1) {
                    fail(path, lineNumber, "malformed TALK_AFTER (expected: TALK_AFTER <char> dialogue...)");
                }
                std::string dialogue;
                std::getline(iss, dialogue);
                dialogue = trim(dialogue);
                if (dialogue.empty()) {
                    fail(path, lineNumber, "TALK_AFTER is missing its dialogue text");
                }
                talkAfterLines[codeToken[0]] = {dialogue, lineNumber};
            } else if (keyword == "TALK_BEFORE") {
                std::istringstream iss(rest);
                std::string codeToken;
                if (!(iss >> codeToken) || codeToken.size() != 1) {
                    fail(path, lineNumber, "malformed TALK_BEFORE (expected: TALK_BEFORE <char> dialogue...)");
                }
                std::string dialogue;
                std::getline(iss, dialogue);
                dialogue = trim(dialogue);
                if (dialogue.empty()) {
                    fail(path, lineNumber, "TALK_BEFORE is missing its dialogue text");
                }
                talkBeforeLines[codeToken[0]] = {dialogue, lineNumber};
            } else if (keyword == "SHOP") {
                std::istringstream iss(rest);
                std::string codeToken;
                if (!(iss >> codeToken) || codeToken.size() != 1) {
                    fail(path, lineNumber, "malformed SHOP (expected: SHOP <char>)");
                }
                shopLines[codeToken[0]] = lineNumber;
            } else if (keyword == "BOAT") {
                std::istringstream iss(rest);
                std::string codeToken;
                if (!(iss >> codeToken) || codeToken.size() != 1) {
                    fail(path, lineNumber, "malformed BOAT (expected: BOAT <char>)");
                }
                boatLines[codeToken[0]] = lineNumber;
            } else if (keyword == "GRANTS_ITEM") {
                std::istringstream iss(rest);
                std::string codeToken, itemId;
                if (!(iss >> codeToken >> itemId) || codeToken.size() != 1) {
                    fail(path, lineNumber,
                         "malformed GRANTS_ITEM (expected: GRANTS_ITEM <char> <item-id> <display-name...>)");
                }
                std::string itemName;
                std::getline(iss, itemName);
                itemName = trim(itemName);
                if (itemName.empty()) {
                    fail(path, lineNumber, "GRANTS_ITEM is missing its display name");
                }
                grantsItemLines[codeToken[0]] = {itemId, itemName, lineNumber};
            } else if (keyword == "BED") {
                std::istringstream iss(rest);
                std::string codeToken;
                if (!(iss >> codeToken) || codeToken.size() != 1) {
                    fail(path, lineNumber, "malformed BED (expected: BED <char>)");
                }
                bedLines[codeToken[0]] = lineNumber;
            } else if (keyword == "SAY_IF") {
                std::istringstream iss(rest);
                std::string codeToken, condition;
                if (!(iss >> codeToken) || codeToken.size() != 1 || !(iss >> condition)) {
                    fail(path, lineNumber, "malformed SAY_IF (expected: SAY_IF <char> <condition> dialogue...)");
                }
                std::string dialogue;
                std::getline(iss, dialogue);
                dialogue = trim(dialogue);
                if (dialogue.empty()) {
                    fail(path, lineNumber, "SAY_IF is missing its dialogue text");
                }
                sayIfLines[codeToken[0]].emplace_back(condition, dialogue, lineNumber);
            } else if (keyword == "TOPIC") {
                std::istringstream iss(rest);
                std::string codeToken;
                if (!(iss >> codeToken) || codeToken.size() != 1) {
                    fail(path, lineNumber, "malformed TOPIC (expected: TOPIC <char> \"label\" text)");
                }
                std::string remainder;
                std::getline(iss, remainder);
                remainder = trim(remainder);
                if (remainder.empty() || remainder.front() != '"') {
                    fail(path, lineNumber, "TOPIC label must be quoted, e.g. TOPIC K \"The Staff\" text");
                }
                size_t closeQuote = remainder.find('"', 1);
                if (closeQuote == std::string::npos) {
                    fail(path, lineNumber, "TOPIC label is missing its closing quote");
                }
                std::string label = remainder.substr(1, closeQuote - 1);
                std::string text = trim(remainder.substr(closeQuote + 1));
                if (text.empty()) {
                    fail(path, lineNumber, "TOPIC is missing its text");
                }
                topicLines[codeToken[0]].emplace_back(label, text, lineNumber);
            } else if (keyword == "TIMELINE_ANCHOR") {
                std::istringstream iss(rest);
                std::string codeToken;
                if (!(iss >> codeToken) || codeToken.size() != 1) {
                    fail(path, lineNumber, "malformed TIMELINE_ANCHOR (expected: TIMELINE_ANCHOR <char>)");
                }
                if (timelineAnchorCode != '\0') {
                    fail(path, lineNumber, "a zone can only have one TIMELINE_ANCHOR");
                }
                timelineAnchorCode = codeToken[0];
                timelineAnchorLine = lineNumber;
            } else if (keyword == "TIMELINE_LOCATION") {
                if (rest.empty()) {
                    fail(path, lineNumber, "malformed TIMELINE_LOCATION (expected: TIMELINE_LOCATION <location-id>)");
                }
                timelineLocationId = rest;
            } else if (keyword == "QUEST") {
                std::istringstream iss(rest);
                std::string codeToken, questId;
                if (!(iss >> codeToken >> questId) || codeToken.size() != 1) {
                    fail(path, lineNumber, "malformed QUEST (expected: QUEST <char> <quest-id>)");
                }
                questLines[codeToken[0]] = {questId, lineNumber};
            } else if (keyword == "END") {
                state = State::Done;
            } else {
                fail(path, lineNumber,
                     "unexpected '" + keyword +
                         "' after GRID (expected POI, PORTAL, TALK, TALK_AGAIN, TALK_AFTER, TALK_BEFORE, SHOP, "
                         "BOAT, GRANTS_ITEM, BED, SAY_IF, TOPIC, TIMELINE_ANCHOR, TIMELINE_LOCATION, QUEST, or END)");
            }
        } else {
            fail(path, lineNumber, "content found after END");
        }
    }

    if (state != State::Done) {
        fail(path, lineNumber + 1, "reached end of file without a closing END");
    }
    if (name.empty()) {
        throw std::runtime_error(path + ": missing NAME");
    }
    if (entryX < 0 || entryY < 0) {
        throw std::runtime_error(path + ": missing or invalid ENTRY");
    }
    if (gridRows.empty()) {
        throw std::runtime_error(path + ": GRID block is empty");
    }

    // Every character in the grid must be either a known ZoneTile code or a
    // declared POI -- fail fast rather than let a typo silently render as
    // "unknown"/impassable during play.
    for (size_t y = 0; y < gridRows.size(); ++y) {
        for (char c : gridRows[y]) {
            bool isKnownTile = zoneTileFor(c).code == c;
            bool isPoi = pois.count(c) > 0;
            if (!isKnownTile && !isPoi) {
                throw std::runtime_error(path + ": GRID contains unrecognized character '" +
                                          std::string(1, c) + "' (row " + std::to_string(y) +
                                          ") -- not a known tile and no matching POI declared");
            }
        }
    }
    if (entryX >= gridWidth || entryY >= static_cast<int>(gridRows.size())) {
        throw std::runtime_error(path + ": ENTRY is outside the GRID bounds");
    }
    // A portal tile needs a description just as much as any other POI, so
    // every PORTAL char is required to also have a POI declaration -- this
    // is a deliberate grammar rule (see docs/ZONE_NOTES.md), not implied by
    // the grid-character check above.
    for (const auto& [code, targetId] : portals) {
        if (pois.count(code) == 0) {
            fail(path, lineNumber, "PORTAL '" + std::string(1, code) + "' has no matching POI declaration");
        }
    }
    // Same rule for TALK: a talkable NPC still needs a name/description via
    // POI, TALK only adds the spoken line on top of it.
    for (const auto& [code, dialogueAndLine] : talkLines) {
        auto it = pois.find(code);
        if (it == pois.end()) {
            fail(path, dialogueAndLine.second,
                 "TALK '" + std::string(1, code) + "' has no matching POI declaration");
        }
        it->second.dialogue = dialogueAndLine.first;
    }
    // Same rule for TALK_AGAIN.
    for (const auto& [code, dialogueAndLine] : talkAgainLines) {
        auto it = pois.find(code);
        if (it == pois.end()) {
            fail(path, dialogueAndLine.second,
                 "TALK_AGAIN '" + std::string(1, code) + "' has no matching POI declaration");
        }
        it->second.dialogueAgain = dialogueAndLine.first;
    }
    // Same rule for TALK_AFTER, plus the same "must already have a TALK
    // line" requirement as BOAT/SAY_IF/TOPIC: aftermath dialogue is a
    // reactive variant of TALK, so a POI with no TALK line could parse
    // cleanly but never actually be reachable in play (see
    // docs/ZONE_NOTES.md's "Aftermath dialogue" section).
    for (const auto& [code, dialogueAndLine] : talkAfterLines) {
        auto it = pois.find(code);
        if (it == pois.end()) {
            fail(path, dialogueAndLine.second,
                 "TALK_AFTER '" + std::string(1, code) + "' has no matching POI declaration");
        }
        if (it->second.dialogue.empty()) {
            fail(path, dialogueAndLine.second,
                 "TALK_AFTER '" + std::string(1, code) + "' has no TALK line to react against");
        }
        it->second.dialogueAfter = dialogueAndLine.first;
    }
    // Same rule for TALK_BEFORE, plus the same "must already have a TALK
    // line" requirement as TALK_AFTER: anticipation dialogue is a reactive
    // variant of TALK too (see docs/ZONE_NOTES.md's "Anticipation dialogue"
    // section).
    for (const auto& [code, dialogueAndLine] : talkBeforeLines) {
        auto it = pois.find(code);
        if (it == pois.end()) {
            fail(path, dialogueAndLine.second,
                 "TALK_BEFORE '" + std::string(1, code) + "' has no matching POI declaration");
        }
        if (it->second.dialogue.empty()) {
            fail(path, dialogueAndLine.second,
                 "TALK_BEFORE '" + std::string(1, code) + "' has no TALK line to react against");
        }
        it->second.dialogueBefore = dialogueAndLine.first;
    }
    // Same rule for SHOP: a browsable POI still needs a name/description
    // via POI, SHOP only marks it as also being able to open the shop screen.
    for (const auto& [code, lineNum] : shopLines) {
        auto it = pois.find(code);
        if (it == pois.end()) {
            fail(path, lineNum, "SHOP '" + std::string(1, code) + "' has no matching POI declaration");
        }
        it->second.isShop = true;
    }
    // Same rule for BOAT, but -- like SAY_IF/TOPIC -- it must also already
    // have a TALK line: BOAT is granted as a side effect of talking to the
    // POI (see GameLoop::talkTo), so a POI with no TALK line could never
    // actually trigger it, and this would silently author dead content.
    for (const auto& [code, lineNum] : boatLines) {
        auto it = pois.find(code);
        if (it == pois.end()) {
            fail(path, lineNum, "BOAT '" + std::string(1, code) + "' has no matching POI declaration");
        }
        if (it->second.dialogue.empty()) {
            fail(path, lineNum, "BOAT '" + std::string(1, code) + "' has no TALK line to grant it through");
        }
        it->second.isBoat = true;
    }
    // Same rule for GRANTS_ITEM as BOAT: granted as a side effect of
    // talking to the POI (see GameLoop::talkTo), so it needs the same
    // "already-declared POI with a TALK line" validation.
    for (const auto& [code, idNameLine] : grantsItemLines) {
        const auto& [itemId, itemName, lineNum] = idNameLine;
        auto it = pois.find(code);
        if (it == pois.end()) {
            fail(path, lineNum, "GRANTS_ITEM '" + std::string(1, code) + "' has no matching POI declaration");
        }
        if (it->second.dialogue.empty()) {
            fail(path, lineNum, "GRANTS_ITEM '" + std::string(1, code) + "' has no TALK line to grant it through");
        }
        it->second.grantsItemId = itemId;
        it->second.grantsItemName = itemName;
    }
    // Same rule for QUEST as BOAT: it must reference an already-declared
    // POI that also has a TALK line, since a quest is offered through
    // talking to that POI (see game::GameLoop::handleTalk) -- without TALK
    // it could parse cleanly but never actually be reachable in play.
    // Whether the quest-id itself is real is validated later, in main.cpp,
    // once quest::QuestCatalog has been loaded (ZoneLoader can't see it).
    std::unordered_map<char, std::string> quests;
    for (const auto& [code, idAndLine] : questLines) {
        auto it = pois.find(code);
        if (it == pois.end()) {
            fail(path, idAndLine.second, "QUEST '" + std::string(1, code) + "' has no matching POI declaration");
        }
        if (it->second.dialogue.empty()) {
            fail(path, idAndLine.second, "QUEST '" + std::string(1, code) + "' has no TALK line to offer it through");
        }
        quests[code] = idAndLine.first;
    }
    // Same rule for BED as SHOP: a bed POI still needs a name/description
    // via POI, BED only marks it as also being able to fully heal there --
    // no TALK prerequisite, since it's not talk-gated like BOAT.
    for (const auto& [code, lineNum] : bedLines) {
        auto it = pois.find(code);
        if (it == pois.end()) {
            fail(path, lineNum, "BED '" + std::string(1, code) + "' has no matching POI declaration");
        }
        it->second.isBed = true;
    }
    // Same rule for SAY_IF: it must reference an already-declared POI --
    // but SAY_IF is reactive dialogue, so its POI must also already have a
    // TALK line for it to react against. Without that, a SAY_IF/TOPIC pair
    // could parse cleanly but never actually be reachable in play (a POI
    // with no TALK line is never a talk candidate at all -- see
    // GameLoop::handleTalk), so this fails fast at load time instead of
    // silently authoring dead content.
    for (const auto& [code, entries] : sayIfLines) {
        auto it = pois.find(code);
        if (it == pois.end()) {
            fail(path, std::get<2>(entries.front()),
                 "SAY_IF '" + std::string(1, code) + "' has no matching POI declaration");
        }
        if (it->second.dialogue.empty()) {
            fail(path, std::get<2>(entries.front()),
                 "SAY_IF '" + std::string(1, code) + "' has no TALK line to react against");
        }
        for (const auto& entry : entries) {
            it->second.conditionalDialogue.emplace_back(std::get<0>(entry), std::get<1>(entry));
        }
    }
    // Same rule for TOPIC.
    for (const auto& [code, entries] : topicLines) {
        auto it = pois.find(code);
        if (it == pois.end()) {
            fail(path, std::get<2>(entries.front()),
                 "TOPIC '" + std::string(1, code) + "' has no matching POI declaration");
        }
        if (it->second.dialogue.empty()) {
            fail(path, std::get<2>(entries.front()),
                 "TOPIC '" + std::string(1, code) + "' has no TALK line to react against");
        }
        for (const auto& entry : entries) {
            it->second.topics.emplace_back(std::get<0>(entry), std::get<1>(entry));
        }
    }
    // Same rule for TIMELINE_ANCHOR: it must point at a real, already-
    // declared POI (the anchor's own name/description come from that POI;
    // TIMELINE_ANCHOR only marks it as also checking canon-character
    // presence).
    if (timelineAnchorCode != '\0' && pois.count(timelineAnchorCode) == 0) {
        fail(path, timelineAnchorLine,
             "TIMELINE_ANCHOR '" + std::string(1, timelineAnchorCode) + "' has no matching POI declaration");
    }

    return Zone(std::move(name), std::move(gridRows), entryX, entryY, std::move(pois), std::move(portals),
                timelineAnchorCode, std::move(timelineLocationId), std::move(quests));
}

} // namespace world
