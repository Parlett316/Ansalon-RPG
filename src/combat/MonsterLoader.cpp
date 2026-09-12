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

void parseTerrainCodes(const std::string& path, int lineNumber, const std::string& rest,
                        const std::string& keyword, std::vector<char>& out) {
    std::istringstream iss(rest);
    std::string token;
    bool any = false;
    while (iss >> token) {
        if (token.size() != 1) {
            fail(path, lineNumber,
                 "malformed " + keyword + " (expected space-separated single-character terrain codes, got '" +
                     token + "')");
        }
        out.push_back(token[0]);
        any = true;
    }
    if (!any) {
        fail(path, lineNumber, "malformed " + keyword + " (expected at least one terrain code)");
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
        } else if (keyword == "CASTS_MAGIC_MISSILE") {
            current.castsMagicMissile = true;
            std::istringstream iss(rest);
            if (!(iss >> current.magicMissileChancePercent)) fail(path, lineNumber, "malformed CASTS_MAGIC_MISSILE");
        } else if (keyword == "BREATH_WEAPON") {
            current.hasBreathWeapon = true;
            std::istringstream iss(rest);
            if (!(iss >> current.breathWeaponChancePercent)) fail(path, lineNumber, "malformed BREATH_WEAPON");
        } else if (keyword == "BREATH_DAMAGE") {
            std::istringstream iss(rest);
            int blinds = 0;
            if (!(iss >> current.breathDamageDiceCount >> current.breathDamageDiceSides >>
                  current.breathDamageFlatBonus >> blinds) ||
                (blinds != 0 && blinds != 1)) {
                fail(path, lineNumber,
                     "malformed BREATH_DAMAGE (expected: BREATH_DAMAGE <count> <sides> <flat> <blinds 0|1> <name...>)");
            }
            current.breathWeaponBlindsOnFail = (blinds == 1);
            std::string name;
            std::getline(iss, name);
            current.breathWeaponName = trim(name);
            if (current.breathWeaponName.empty()) {
                fail(path, lineNumber, "malformed BREATH_DAMAGE (missing trailing breath weapon name)");
            }
        } else if (keyword == "SIZE") {
            std::istringstream iss(rest);
            if (!(iss >> current.footprintWidth >> current.footprintHeight) || current.footprintWidth <= 0 ||
                current.footprintHeight <= 0) {
                fail(path, lineNumber, "malformed SIZE (expected: SIZE <width> <height>, both positive)");
            }
        } else if (keyword == "BURSTS_INTO_FLAME") {
            current.burstsIntoFlameOnDeath = true;
        } else if (keyword == "EXCLUDE_TERRAIN") {
            parseTerrainCodes(path, lineNumber, rest, "EXCLUDE_TERRAIN", current.excludedTerrain);
        } else if (keyword == "TERRAIN_BIAS") {
            parseTerrainCodes(path, lineNumber, rest, "TERRAIN_BIAS", current.terrainBias);
        } else if (keyword == "ONLY_TERRAIN") {
            parseTerrainCodes(path, lineNumber, rest, "ONLY_TERRAIN", current.onlyTerrain);
        } else if (keyword == "MIN_TOWN_DISTANCE") {
            std::istringstream iss(rest);
            if (!(iss >> current.minTownDistance)) fail(path, lineNumber, "malformed MIN_TOWN_DISTANCE");
        } else if (keyword == "GROUP") {
            std::istringstream iss(rest);
            int groupMin = 0;
            int groupMax = 0;
            if (!(iss >> groupMin >> groupMax)) {
                fail(path, lineNumber, "malformed GROUP (expected: GROUP <min> <max>)");
            }
            if (groupMin <= 0 || groupMax < groupMin) {
                fail(path, lineNumber, "malformed GROUP (min must be >= 1 and max >= min)");
            }
            current.groupMin = groupMin;
            current.groupMax = groupMax;
        } else if (keyword == "MOVE") {
            std::istringstream iss(rest);
            int moveSquares = 0;
            if (!(iss >> moveSquares) || moveSquares <= 0) {
                fail(path, lineNumber, "malformed MOVE (expected a positive integer number of squares)");
            }
            current.moveSquares = moveSquares;
        } else if (keyword == "DESC") {
            current.description = rest;
        } else if (keyword == "END") {
            if (current.footprintWidth * current.footprintHeight > 1 && current.groupMax > 1) {
                fail(path, lineNumber,
                     "a multi-cell SIZE monster must stay solo (GROUP max 1) -- no multi-cell BFS pathing or "
                     "overlap resolution exists for more than one big creature at once");
            }
            if (current.hasBreathWeapon && current.breathDamageDiceCount == 0 && current.breathDamageDiceSides == 0 &&
                current.breathDamageFlatBonus == 0 && current.breathWeaponName.empty()) {
                fail(path, lineNumber, "BREATH_WEAPON requires a BREATH_DAMAGE line in the same block");
            }
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
