#include "world/ZoneLoader.h"
#include "world/Zone.h"
#include "world/ZoneTile.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
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
            } else if (keyword == "END") {
                state = State::Done;
            } else {
                fail(path, lineNumber, "unexpected '" + keyword + "' after GRID (expected POI or END)");
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

    return Zone(std::move(name), std::move(gridRows), entryX, entryY, std::move(pois));
}

} // namespace world
