#include "world/WorldLoader.h"
#include "world/World.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace world {

namespace {

std::string trim(const std::string& s) {
    size_t begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

// Splits "KEYWORD rest of line" into {keyword, rest}. The data format is
// authored by us (not third-party input), so keyword matching is
// case-sensitive by design -- no need for leniency we'd never exercise.
std::pair<std::string, std::string> splitKeyword(const std::string& line) {
    size_t sp = line.find_first_of(" \t");
    if (sp == std::string::npos) return {line, ""};
    return {line.substr(0, sp), trim(line.substr(sp + 1))};
}

[[noreturn]] void fail(int lineNumber, const std::string& message) {
    throw std::runtime_error("locations.txt:" + std::to_string(lineNumber) + ": " + message);
}

// Parses `targetId days "quoted road description"` as used by CONNECT lines.
Connection parseConnection(const std::string& rest, const std::string& locationId, int lineNumber) {
    std::istringstream iss(rest);
    std::string targetId;
    int days = 0;
    if (!(iss >> targetId >> days)) {
        fail(lineNumber, "malformed CONNECT in location '" + locationId +
                             "' (expected: CONNECT id days \"text\")");
    }
    std::string remainder;
    std::getline(iss, remainder);
    remainder = trim(remainder);
    if (remainder.size() < 2 || remainder.front() != '"' || remainder.back() != '"') {
        fail(lineNumber, "CONNECT road description must be wrapped in double quotes (location '" +
                              locationId + "')");
    }
    return Connection{targetId, days, remainder.substr(1, remainder.size() - 2)};
}

} // namespace

void WorldLoader::loadFromFile(const std::string& path, World& outWorld) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open world data file: " + path);
    }

    std::vector<Location> locations;
    Location current;
    bool inLocation = false;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        auto [keyword, rest] = splitKeyword(trimmed);

        if (keyword == "LOCATION") {
            if (inLocation) fail(lineNumber, "found LOCATION before the previous block's END");
            current = Location{};
            current.id = rest;
            inLocation = true;
        } else if (!inLocation) {
            fail(lineNumber, "'" + keyword + "' outside of a LOCATION/END block");
        } else if (keyword == "NAME") {
            current.name = rest;
        } else if (keyword == "REGION") {
            current.region = rest;
        } else if (keyword == "TERRAIN") {
            current.terrain = rest;
        } else if (keyword == "GLYPH") {
            current.glyph = rest.empty() ? '?' : rest[0];
        } else if (keyword == "POS") {
            std::istringstream iss(rest);
            if (!(iss >> current.row >> current.col)) {
                fail(lineNumber, "malformed POS (expected: POS row col)");
            }
            if (current.row < 0 || current.col < 0) {
                fail(lineNumber, "POS row/col must be non-negative");
            }
        } else if (keyword == "DESC") {
            current.description = rest;
        } else if (keyword == "CONNECT") {
            current.connections.push_back(parseConnection(rest, current.id, lineNumber));
        } else if (keyword == "END") {
            locations.push_back(current);
            inLocation = false;
        } else {
            fail(lineNumber, "unknown keyword '" + keyword + "'");
        }
    }

    if (inLocation) {
        throw std::runtime_error("locations.txt: reached end of file inside a LOCATION block missing END");
    }

    // Validate every CONNECT target refers to a location that actually
    // exists, once, here -- see the rationale in WorldLoader.h.
    std::unordered_set<std::string> ids;
    for (const auto& loc : locations) ids.insert(loc.id);
    for (const auto& loc : locations) {
        for (const auto& conn : loc.connections) {
            if (!ids.count(conn.targetId)) {
                throw std::runtime_error("locations.txt: location '" + loc.id +
                                          "' has a CONNECT to unknown location '" + conn.targetId + "'");
            }
        }
    }

    for (auto& loc : locations) {
        outWorld.addLocation(std::move(loc));
    }
}

} // namespace world
