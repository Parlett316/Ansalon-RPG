#include "character/CharacterCreator.h"
#include "character/Dice.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace character {

namespace {

// Throws on EOF/stream failure rather than returning an empty line: without
// this check, a closed/exhausted stdin (a piped script running out, or a
// real user hitting Ctrl+Z/Ctrl+D) would make promptChoice/promptYesNo's
// reprompt loops below spin forever printing "please enter..." since
// std::cin never recovers on its own. Throwing lets main.cpp's existing
// catch block end the program cleanly instead.
std::string promptLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw std::runtime_error("input ended unexpectedly during character creation");
    }
    return line;
}

// Prompts until the user enters an integer in [minValue, maxValue],
// re-prompting on anything else. std::cin is an external boundary (a human,
// or in tests a piped script), so this has to tolerate garbage input rather
// than assume it's well-formed.
int promptChoice(const std::string& prompt, int minValue, int maxValue) {
    for (;;) {
        std::istringstream iss(promptLine(prompt));
        int value;
        if (iss >> value && value >= minValue && value <= maxValue) {
            return value;
        }
        std::cout << "Please enter a number from " << minValue << " to " << maxValue << ".\n";
    }
}

bool promptYesNo(const std::string& prompt) {
    for (;;) {
        std::string line = promptLine(prompt);
        if (!line.empty() && (line[0] == 'y' || line[0] == 'Y')) return true;
        if (!line.empty() && (line[0] == 'n' || line[0] == 'N')) return false;
        std::cout << "Please answer y or n.\n";
    }
}

AbilityScores rollScores() {
    AbilityScores scores;
    scores.strength = roll(3, 6);
    scores.dexterity = roll(3, 6);
    scores.constitution = roll(3, 6);
    scores.intelligence = roll(3, 6);
    scores.wisdom = roll(3, 6);
    scores.charisma = roll(3, 6);
    return scores;
}

void printScores(const AbilityScores& s) {
    std::cout << "  STR " << s.strength << "  DEX " << s.dexterity << "  CON " << s.constitution
               << "  INT " << s.intelligence << "  WIS " << s.wisdom << "  CHA " << s.charisma << "\n";
}

} // namespace

Character CharacterCreator::run() {
    std::cout << "\n=== Character Creation (2nd Edition AD&D) ===\n\n";

    Character character;
    character.name = promptLine("What is your name, traveler? ");

    // Method I dice (3d6, straight down the line, fixed STR/DEX/CON/INT/
    // WIS/CHA order) with a house-rule reroll-the-whole-set option, per the
    // project owner's preference -- not Method II's drop-lowest/arrange.
    AbilityScores scores;
    for (;;) {
        std::cout << "\nRolling 3d6 down the line...\n";
        scores = rollScores();
        printScores(scores);
        if (promptYesNo("Keep these scores? (y/n) ")) break;
    }

    std::cout << "\nChoose a race:\n";
    for (size_t i = 0; i < kAllRaces.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << raceInfo(kAllRaces[i]).name << "\n";
    }
    int raceChoice = promptChoice("> ", 1, static_cast<int>(kAllRaces.size()));
    character.race = kAllRaces[static_cast<size_t>(raceChoice - 1)];
    applyRacialAdjustments(character.race, scores);

    std::cout << "\nYour ability scores after racial adjustments:\n";
    printScores(scores);
    character.scores = scores;

    bool raceCanBeMage = raceInfo(character.race).canBeMage;
    std::cout << "\nChoose a class:\n";
    for (size_t i = 0; i < kAllClasses.size(); ++i) {
        const ClassInfo& c = classInfo(kAllClasses[i]);
        bool qualifies = scores.get(c.primeRequisite) >= c.primeRequisiteMinimum;
        bool blockedByRace = c.id == ClassId::Mage && !raceCanBeMage;
        std::cout << "  " << (i + 1) << ". " << c.name;
        if (blockedByRace) {
            std::cout << "  (kender have innate magic resistance and cannot learn arcane magic)";
        } else if (!qualifies) {
            std::cout << "  (does not meet prime requisite)";
        }
        std::cout << "\n";
    }
    int classChoice = promptChoice("> ", 1, static_cast<int>(kAllClasses.size()));
    character.charClass = kAllClasses[static_cast<size_t>(classChoice - 1)];
    const ClassInfo& chosenClass = classInfo(character.charClass);

    std::cout << "\nChoose an alignment:\n";
    for (int i = 0; i < 9; ++i) {
        std::cout << "  " << (i + 1) << ". " << alignmentName(static_cast<Alignment>(i)) << "\n";
    }
    character.alignment = static_cast<Alignment>(promptChoice("> ", 1, 9) - 1);

    bool isWarrior = character.charClass == ClassId::Fighter;
    character.maxHp = std::max(1, chosenClass.hitDieSides + hpAdjustmentForConstitution(scores.constitution, isWarrior));
    character.currentHp = character.maxHp;
    character.armorClass = 10 - acAdjustmentForDexterity(scores.dexterity);
    character.thac0 = 20; // true to 2e: every class starts at THAC0 20, diverging only as levels are gained

    character.saves = chosenClass.level1Saves;
    applyRacialSavingThrowBonus(character.race, scores.constitution, character.saves);

    character.goldPieces =
        (roll(chosenClass.goldDiceCount, chosenClass.goldDiceSides) + chosenClass.goldFlatBonus) *
        chosenClass.goldMultiplier;

    std::cout << "\n=== " << character.name << " ===\n";
    std::cout << raceInfo(character.race).name << " " << chosenClass.name << ", "
               << alignmentName(character.alignment) << "\n";
    printScores(character.scores);
    std::cout << "HP " << character.maxHp << "   AC " << character.armorClass
               << "   THAC0 " << character.thac0 << "\n";
    std::cout << "Saves -- " << saveCategoryName(SaveCategory::ParalyzationPoisonDeath) << ": "
               << character.saves.at(SaveCategory::ParalyzationPoisonDeath)
               << "  " << saveCategoryName(SaveCategory::RodStaffWand) << ": "
               << character.saves.at(SaveCategory::RodStaffWand)
               << "  " << saveCategoryName(SaveCategory::PetrificationPolymorph) << ": "
               << character.saves.at(SaveCategory::PetrificationPolymorph)
               << "  " << saveCategoryName(SaveCategory::BreathWeapon) << ": "
               << character.saves.at(SaveCategory::BreathWeapon)
               << "  " << saveCategoryName(SaveCategory::Spell) << ": "
               << character.saves.at(SaveCategory::Spell) << "\n";
    std::cout << "Gold: " << character.goldPieces << " gp\n";

    if (!promptYesNo("\nBegin your journey as this character? (y/n) ")) {
        return run();
    }
    return character;
}

} // namespace character
