#include "world/BattleMapLoader.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

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

[[noreturn]] void fail(const std::string& path, int lineNumber, const std::string& message) {
    throw std::runtime_error(path + ":" + std::to_string(lineNumber) + ": " + message);
}

} // namespace

BattleMap BattleMapLoader::loadFromFile(const std::string& path, int expectedWidth, int expectedHeight) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open battlemap file: " + path);
    }

    // Same "literal raw-text block embedded in an otherwise keyword-per-line
    // file" idiom docs/ZONE_NOTES.md documents for zones -- just with no
    // other keywords: a battlemap file is nothing but this one block.
    enum class State { Header, InGrid, Done };
    State state = State::Header;
    std::vector<std::string> gridRows;
    std::string line;
    int lineNumber = 0;
    int gridStartLine = -1;

    while (std::getline(file, line)) {
        ++lineNumber;
        line = stripCR(line);

        if (state == State::InGrid) {
            if (line == "ENDGRID") {
                state = State::Done;
                break;
            }
            if (static_cast<int>(line.size()) != expectedWidth) {
                fail(path, lineNumber,
                     "GRID row has width " + std::to_string(line.size()) + ", expected " +
                         std::to_string(expectedWidth));
            }
            for (size_t x = 0; x < line.size(); ++x) {
                if (line[x] != '.' && line[x] != '#') {
                    fail(path, lineNumber,
                         "invalid battlemap tile '" + std::string(1, line[x]) + "' at column " +
                             std::to_string(x) + " (must be '.' or '#')");
                }
            }
            gridRows.push_back(line);
            continue;
        }

        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        if (trimmed == "GRID") {
            state = State::InGrid;
            gridStartLine = lineNumber;
        } else {
            fail(path, lineNumber, "unexpected '" + trimmed + "' (a battlemap file is just a GRID/ENDGRID block)");
        }
    }

    if (gridStartLine == -1) {
        throw std::runtime_error(path + ": missing GRID block");
    }
    if (state != State::Done) {
        throw std::runtime_error(path + ": GRID block is missing its ENDGRID");
    }
    if (static_cast<int>(gridRows.size()) != expectedHeight) {
        throw std::runtime_error(path + ": GRID has " + std::to_string(gridRows.size()) + " rows, expected " +
                                  std::to_string(expectedHeight));
    }
    // Row 0 and the last row are combat's fixed spawn rows (monster
    // instances on row 0, player/companions on the last row -- see
    // sfml_phase1/main.cpp's combatStartEncounter) -- guaranteed clear here
    // so no hand-authored wall can ever trap a spawn.
    for (char c : gridRows.front()) {
        if (c == '#') throw std::runtime_error(path + ": row 0 must be entirely open ('.') -- it's the monster spawn row");
    }
    for (char c : gridRows.back()) {
        if (c == '#') {
            throw std::runtime_error(path + ": the last row must be entirely open ('.') -- it's the player/companion spawn row");
        }
    }

    return BattleMap(std::move(gridRows), expectedWidth, expectedHeight);
}

} // namespace world
