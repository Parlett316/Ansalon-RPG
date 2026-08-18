#include "character/CharacterCreator.h"
#include "character/Dice.h"
#include "character/Equipment.h"

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

    // Elf and Dwarf always resolve to one of Dragonlance's real subraces --
    // there's no generic "Elf" or "Dwarf" character on Krynn. See Race.h.
    if (character.race == RaceId::Elf) {
        std::cout << "\nChoose an elven heritage:\n";
        for (size_t i = 0; i < kElfSubraces.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << subraceInfo(kElfSubraces[i])->name << "\n";
        }
        int choice = promptChoice("> ", 1, static_cast<int>(kElfSubraces.size()));
        character.subrace = kElfSubraces[static_cast<size_t>(choice - 1)];
    } else if (character.race == RaceId::Dwarf) {
        std::cout << "\nChoose a dwarven clan:\n";
        for (size_t i = 0; i < kDwarfSubraces.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << subraceInfo(kDwarfSubraces[i])->name << "\n";
        }
        int choice = promptChoice("> ", 1, static_cast<int>(kDwarfSubraces.size()));
        character.subrace = kDwarfSubraces[static_cast<size_t>(choice - 1)];
    }
    applyRacialOrSubracialAdjustments(character.race, character.subrace, scores);

    std::cout << "\nYour ability scores after racial adjustments:\n";
    printScores(scores);
    character.scores = scores;

    // Gnomes are forced into the Tinker class, not offered a choice --
    // Dragonlance Adventures (TSR 2021, p.57) states "Gnomes in Krynn can
    // only be of the tinker class," and Tinker exists in this project
    // specifically to make that true rather than as a menu option other
    // races could also pick (see docs/CHARACTER_NOTES.md).
    if (character.race == RaceId::Gnome) {
        character.charClass = ClassId::Tinker;
        std::cout << "\nAs a Gnome, you are a Tinker -- Krynn's gnomes know no other calling.\n";
    } else {
        bool raceCanBeMage = effectiveCanBeMage(character.race, character.subrace);
        // Krynn-specific Mage prerequisite on top of the base INT 9+
        // (Dragonlance Adventures p.35, "Student Wizard Minimum Scores"):
        // Dexterity 6+.
        bool mageDexOk = scores.dexterity >= 6;
        std::cout << "\nChoose a class:\n";
        for (size_t i = 0; i < kAllClasses.size(); ++i) {
            const ClassInfo& c = classInfo(kAllClasses[i]);
            bool qualifies = scores.get(c.primeRequisite) >= c.primeRequisiteMinimum;
            bool blockedByRace = c.id == ClassId::Mage && !raceCanBeMage;
            std::cout << "  " << (i + 1) << ". " << c.name;
            if (blockedByRace) {
                std::cout << "  (cannot learn arcane magic)";
            } else if (c.id == ClassId::Mage && !mageDexOk) {
                std::cout << "  (wizardry on Krynn also requires Dexterity 6+)";
            } else if (!qualifies) {
                std::cout << "  (does not meet prime requisite)";
            }
            std::cout << "\n";
        }
        int classChoice = promptChoice("> ", 1, static_cast<int>(kAllClasses.size()));
        character.charClass = kAllClasses[static_cast<size_t>(classChoice - 1)];
    }
    const ClassInfo& chosenClass = classInfo(character.charClass);

    // Exceptional (percentile) Strength: PHB Table 1 (p.19) only lets
    // "warrior" group characters roll this at 18 Strength -- Fighter is
    // the only warrior-group class this project has (Knight of the Crown
    // is built on top of it, so it's covered too).
    if (character.charClass == ClassId::Fighter && scores.strength == 18) {
        character.exceptionalStrengthPercentile = roll(1, 100);
    }

    std::cout << "\nChoose an alignment:\n";
    for (int i = 0; i < 9; ++i) {
        std::cout << "  " << (i + 1) << ". " << alignmentName(static_cast<Alignment>(i)) << "\n";
    }
    character.alignment = static_cast<Alignment>(promptChoice("> ", 1, 9) - 1);

    // Knights of Solamnia: every Knight starts in the Order of the Crown
    // (Sword/Rose require leveling this project doesn't have yet -- see
    // Knighthood.h). Only offered to Fighters, the closest chassis this
    // project has to the book's Cavalier-based Knight classes.
    if (character.charClass == ClassId::Fighter &&
        meetsKnightOfCrownRequirements(character.race, character.subrace, scores, character.alignment)) {
        std::cout << "\nYou meet the qualifications to be sponsored into the Knights of Solamnia "
                      "as a Knight of the Crown.\n";
        if (promptYesNo("Swear the oath and join? (y/n) ")) {
            character.knightOrder = KnightOrder::Crown;
        }
    }

    bool isWarrior = character.charClass == ClassId::Fighter;
    character.maxHp = std::max(1, chosenClass.hitDieSides + hpAdjustmentForConstitution(scores.constitution, isWarrior));
    character.currentHp = character.maxHp;
    character.weaponName = chosenClass.weaponName;
    character.weaponDamageSides = chosenClass.weaponDamageSides;
    character.weaponDamageBonus = 0;
    recomputeArmorClass(character); // equippedArmor is still None here, so this is just the Dex adjustment
    character.thac0 = 20; // true to 2e: every class starts at THAC0 20, diverging only as levels are gained

    character.saves = chosenClass.level1Saves;
    applyRacialSavingThrowBonus(character.race, scores.constitution, character.saves);

    character.steelPieces =
        (roll(chosenClass.steelDiceCount, chosenClass.steelDiceSides) + chosenClass.steelFlatBonus) *
        chosenClass.steelMultiplier;

    const SubraceInfo* sub = subraceInfo(character.subrace);
    std::cout << "\n=== " << character.name << " ===\n";
    std::cout << (sub != nullptr ? sub->name : raceInfo(character.race).name) << " " << chosenClass.name
               << ", " << alignmentName(character.alignment) << "\n";
    if (character.knightOrder != KnightOrder::None) {
        std::cout << knightOrderName(character.knightOrder) << "\n";
    }
    if (character.charClass == ClassId::Mage) {
        std::cout << "An unaffiliated student of the arcane -- a Robe and Order of High Sorcery\n"
                      "await at higher levels.\n";
    }
    printScores(character.scores);
    if (character.exceptionalStrengthPercentile > 0) {
        std::cout << "  (exceptional Strength: 18/";
        if (character.exceptionalStrengthPercentile == 100) {
            std::cout << "00";
        } else if (character.exceptionalStrengthPercentile < 10) {
            std::cout << "0" << character.exceptionalStrengthPercentile;
        } else {
            std::cout << character.exceptionalStrengthPercentile;
        }
        std::cout << ")\n";
    }
    std::cout << "HP " << character.maxHp << "   AC " << character.armorClass
               << "   THAC0 " << character.thac0 << "\n";
    std::cout << "Weapon: " << character.weaponName << "\n";
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
    std::cout << "Steel: " << character.steelPieces << " stl\n";

    if (!promptYesNo("\nBegin your journey as this character? (y/n) ")) {
        return run();
    }
    return character;
}

} // namespace character
