#include "combat/MonsterLoader.h"
#include "combat/Monster.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace combat {

namespace {

// Same trim/splitKeyword/fail idioms as every other loader in this
// project (see docs/ARCHITECTURE.md).
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

void parseDiceTriple(const std::string& path, int lineNumber, const std::string& rest,
                      const std::string& keyword, int& count, int& sides, int& flat) {
    std::istringstream iss(rest);
    if (!(iss >> count >> sides >> flat)) {
        fail(path, lineNumber, "malformed " + keyword + " (expected: " + keyword + " <count> <sides> <flat>)");
    }
}

} // namespace

void MonsterLoader::loadFromFile(const std::string& path, MonsterCatalog& outCatalog) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open monster data file: " + path);
    }

    Monster current;
    bool inMonster = false;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        auto [keyword, rest] = splitKeyword(trimmed);

        if (keyword == "MONSTER") {
            if (inMonster) fail(path, lineNumber, "found MONSTER before the previous block's END");
            current = Monster{};
            current.id = rest;
            inMonster = true;
        } else if (!inMonster) {
            fail(path, lineNumber, "'" + keyword + "' outside of a MONSTER/END block");
        } else if (keyword == "NAME") {
            current.name = rest;
        } else if (keyword == "HP") {
            parseDiceTriple(path, lineNumber, rest, "HP", current.hpDiceCount, current.hpDiceSides,
                             current.hpFlatBonus);
        } else if (keyword == "AC") {
            std::istringstream iss(rest);
            if (!(iss >> current.armorClass)) fail(path, lineNumber, "malformed AC");
        } else if (keyword == "THAC0") {
            std::istringstream iss(rest);
            if (!(iss >> current.thac0)) fail(path, lineNumber, "malformed THAC0");
        } else if (keyword == "DAMAGE") {
            parseDiceTriple(path, lineNumber, rest, "DAMAGE", current.damageDiceCount,
                             current.damageDiceSides, current.damageFlatBonus);
        } else if (keyword == "STEEL") {
            parseDiceTriple(path, lineNumber, rest, "STEEL", current.steelDiceCount, current.steelDiceSides,
                             current.steelFlatBonus);
        } else if (keyword == "XP") {
            std::istringstream iss(rest);
            if (!(iss >> current.xpValue)) fail(path, lineNumber, "malformed XP");
        } else if (keyword == "POISON") {
            current.poisonOnHit = true;
        } else if (keyword == "DESC") {
            current.description = rest;
        } else if (keyword == "END") {
            outCatalog.addMonster(std::move(current));
            inMonster = false;
        } else {
            fail(path, lineNumber, "unknown keyword '" + keyword + "'");
        }
    }

    if (inMonster) {
        fail(path, lineNumber + 1, "reached end of file inside a MONSTER block missing END");
    }
}

} // namespace combat
