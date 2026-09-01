#include "character/CharacterCreator.h"
#include "character/Dice.h"
#include "character/Equipment.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace character {

namespace {

// ANSI palette reused verbatim from render/MapRenderer.cpp's existing
// convention (bright yellow for headers/standout names, bright cyan for
// status labels, bright white for "must always read clearly") plus one
// new code, bright green, for confirmed/assigned values -- matching the
// two reference screenshots (References/abilityscore.png,
// References/Bobs_Game.png) this screen redesign is modeled on.
constexpr const char* kColorHeader = "\x1b[93m";
constexpr const char* kColorLabel = "\x1b[96m";
constexpr const char* kColorSelect = "\x1b[92m";
constexpr const char* kColorName = "\x1b[97m";

std::string color(const std::string& text, const char* code) {
    return std::string(code) + text + "\x1b[0m";
}

// Same VT100 sequence as render::Console::clearScreen()/MapRenderer.cpp's
// own inline uses -- emitted directly rather than calling into
// render::Console so character/ keeps its zero-dependency-on-render/
// status (see docs/ARCHITECTURE.md's module map). Safe here because
// render::Console's constructor (which enables
// ENABLE_VIRTUAL_TERMINAL_PROCESSING on Windows) already ran at the top
// of main() before CharacterCreator::run() is ever called.
void clearScreen() {
    std::cout << "\x1b[2J\x1b[H";
}

void printStepHeader(int step, const std::string& title) {
    clearScreen();
    std::cout << color("STEP " + std::to_string(step) + ": " + title, kColorHeader) << "\n\n";
}

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

// Reprompts until the name fits, same "tolerate garbage input, reprompt on
// anything else" shape as promptChoice above.
std::string promptName() {
    for (;;) {
        std::string name = promptLine("What is your name, traveler? ");
        if (name.size() <= 20) return name;
        std::cout << "Names can be at most 20 characters -- please try again.\n";
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

// Used only where a screen would otherwise be overwritten by the next
// step's clearScreen before the player has had a chance to read it (the
// race-adjustments recap below is the one such spot -- every other
// transition already ends on a promptChoice/promptYesNo that serves as
// its own "read this, then respond" gate).
void pauseForEnter() {
    promptLine("\n(Press Enter to continue) ");
}

// PHB p.27 (base races) / Dragonlance Adventures pp.59-61,66-67 (Elf/Dwarf
// subraces) -- true if `scores` (the character's base, pre-adjustment
// rolled scores) let this race be chosen at all. Elf/Dwarf are checked via
// "does at least one subrace qualify," since there's no generic Elf/Dwarf
// character on Krynn (see Race.h); Human always qualifies (no Table 7
// entry at all).
bool raceIsSelectable(RaceId race, const AbilityScores& scores) {
    if (race == RaceId::Elf) {
        for (SubraceId sub : kElfSubraces) {
            if (meetsSubraceAbilityRange(sub, scores)) return true;
        }
        return false;
    }
    if (race == RaceId::Dwarf) {
        for (SubraceId sub : kDwarfSubraces) {
            if (meetsSubraceAbilityRange(sub, scores)) return true;
        }
        return false;
    }
    return meetsAbilityRange(race, scores);
}

constexpr std::array<Ability, 6> kAbilityOrder = {
    Ability::Strength, Ability::Dexterity, Ability::Constitution,
    Ability::Intelligence, Ability::Wisdom, Ability::Charisma,
};

void printScores(const AbilityScores& s) {
    std::cout << "  STR " << s.strength << "  DEX " << s.dexterity << "  CON " << s.constitution
               << "  INT " << s.intelligence << "  WIS " << s.wisdom << "  CHA " << s.charisma << "\n";
}

// Recap block shown at the top of the race/class/alignment screens so
// the player's scores stay visible across steps rather than being
// something they have to remember or scroll back for.
void printScoresRecap(const AbilityScores& s) {
    std::cout << color("Ability Scores:", kColorLabel) << "\n";
    printScores(s);
    std::cout << "\n";
}

void printAbilityMethodHeader() {
    printStepHeader(1, "ABILITY SCORES");
    std::cout << "Method V: 4d6, drop lowest die, six times.\n";
    std::cout << "Assign each rolled score to an ability.\n\n";
}

// Method V (PHB p.19, confirmed against the rendered page image -- see
// docs/CHARACTER_NOTES.md): roll 4d6-drop-lowest six times, then let the
// player assign the six results to abilities however they want. The
// house-rule whole-set reroll from the project's prior Method I era is
// kept, applied to these six rolls before assignment.
std::vector<int> rollAndKeepPool() {
    std::vector<int> pool;
    for (;;) {
        printAbilityMethodHeader();
        pool.clear();
        for (int i = 0; i < 6; ++i) pool.push_back(roll4d6DropLowest());

        std::cout << "Rolled: ";
        for (size_t i = 0; i < pool.size(); ++i) {
            std::cout << (i == 0 ? "" : ", ") << pool[i];
        }
        std::cout << "\n\n";

        if (promptYesNo("Keep these rolls? (y/n) ")) break;
    }
    return pool;
}

// Interactive "Assigning: <Ability>" loop -- the actual interaction the
// reference screenshot depicts, adapted from a live arrow-key cursor to a
// numbered-list choice. See docs/ARCHITECTURE.md's "Character creation"
// section for why: CharacterCreator deliberately stays on plain
// std::cin/std::cout (not render::Console::readKey()) so it remains the
// one part of the game a piped/redirected test script can drive
// end-to-end, and a live-highlighted cursor would need real single-key
// input to work.
AbilityScores assignPoolToAbilities(std::vector<int> pool) {
    AbilityScores scores;
    std::array<bool, kAbilityOrder.size()> filled{};

    for (Ability ability : kAbilityOrder) {
        printAbilityMethodHeader();

        for (Ability shown : kAbilityOrder) {
            size_t idx = static_cast<size_t>(shown);
            std::cout << "  " << std::left << std::setw(14) << abilityName(shown);
            if (filled[idx]) {
                std::cout << color(std::to_string(scores.get(shown)), kColorSelect) << "\n";
            } else {
                std::cout << "--\n";
            }
        }

        std::cout << "\n" << color(std::string("Assigning: ") + abilityName(ability), kColorLabel) << "\n\n";
        for (size_t i = 0; i < pool.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << pool[i] << "\n";
        }

        int choice = promptChoice("\n> ", 1, static_cast<int>(pool.size()));
        int picked = pool[static_cast<size_t>(choice - 1)];
        pool.erase(pool.begin() + (choice - 1));

        scores.adjust(ability, picked);
        filled[static_cast<size_t>(ability)] = true;
    }

    return scores;
}

} // namespace

Character CharacterCreator::run() {
    clearScreen();
    std::cout << color("=== Character Creation (2nd Edition AD&D) ===", kColorHeader) << "\n\n";

    Character character;
    character.name = promptName();

    AbilityScores scores = assignPoolToAbilities(rollAndKeepPool());

    printStepHeader(2, "RACE");
    printScoresRecap(scores);

    // Hard-blocks a pick whose base (pre-adjustment) rolled scores fall
    // outside that race's PHB/DLA ability range -- PHB p.27: "If the basic
    // scores that you rolled up meet the requirements for a particular
    // race, your character can be of that race" implies the converse too.
    // Human is always selectable (no Table 7 entry), so this can never
    // dead-end the player with no legal choice.
    for (;;) {
        std::cout << "Choose a race:\n";
        for (size_t i = 0; i < kAllRaces.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << raceInfo(kAllRaces[i]).name;
            if (!raceIsSelectable(kAllRaces[i], scores)) {
                std::cout << "  (your ability scores don't qualify)";
            }
            std::cout << "\n";
        }
        int raceChoice = promptChoice("> ", 1, static_cast<int>(kAllRaces.size()));
        RaceId candidate = kAllRaces[static_cast<size_t>(raceChoice - 1)];
        if (!raceIsSelectable(candidate, scores)) {
            std::cout << "\nYour rolled ability scores don't meet " << raceInfo(candidate).name
                       << "'s requirements. Choose a different race.\n\n";
            continue;
        }
        character.race = candidate;
        break;
    }

    // Elf and Dwarf always resolve to one of Dragonlance's real subraces --
    // there's no generic "Elf" or "Dwarf" character on Krynn. See Race.h.
    // raceIsSelectable already confirmed at least one subrace qualifies, so
    // this loop can't dead-end either.
    if (character.race == RaceId::Elf) {
        for (;;) {
            std::cout << "\nChoose an elven heritage:\n";
            for (size_t i = 0; i < kElfSubraces.size(); ++i) {
                std::cout << "  " << (i + 1) << ". " << subraceInfo(kElfSubraces[i])->name;
                if (!meetsSubraceAbilityRange(kElfSubraces[i], scores)) {
                    std::cout << "  (your ability scores don't qualify)";
                }
                std::cout << "\n";
            }
            int choice = promptChoice("> ", 1, static_cast<int>(kElfSubraces.size()));
            SubraceId candidate = kElfSubraces[static_cast<size_t>(choice - 1)];
            if (!meetsSubraceAbilityRange(candidate, scores)) {
                std::cout << "\nYour rolled ability scores don't meet " << subraceInfo(candidate)->name
                           << "'s requirements. Choose a different heritage.\n";
                continue;
            }
            character.subrace = candidate;
            break;
        }
    } else if (character.race == RaceId::Dwarf) {
        for (;;) {
            std::cout << "\nChoose a dwarven clan:\n";
            for (size_t i = 0; i < kDwarfSubraces.size(); ++i) {
                std::cout << "  " << (i + 1) << ". " << subraceInfo(kDwarfSubraces[i])->name;
                if (!meetsSubraceAbilityRange(kDwarfSubraces[i], scores)) {
                    std::cout << "  (your ability scores don't qualify)";
                }
                std::cout << "\n";
            }
            int choice = promptChoice("> ", 1, static_cast<int>(kDwarfSubraces.size()));
            SubraceId candidate = kDwarfSubraces[static_cast<size_t>(choice - 1)];
            if (!meetsSubraceAbilityRange(candidate, scores)) {
                std::cout << "\nYour rolled ability scores don't meet " << subraceInfo(candidate)->name
                           << "'s requirements. Choose a different clan.\n";
                continue;
            }
            character.subrace = candidate;
            break;
        }
    }

    AbilityScores beforeRaceAdjustments = scores;
    applyRacialOrSubracialAdjustments(character.race, character.subrace, scores);
    character.scores = scores;

    std::cout << "\n" << color("Race Adjustments:", kColorLabel) << "\n";
    bool anyAdjustment = false;
    for (Ability a : kAbilityOrder) {
        int before = beforeRaceAdjustments.get(a);
        int after = scores.get(a);
        if (before == after) continue;
        anyAdjustment = true;
        std::string delta = (after > before ? "+" : "") + std::to_string(after - before);
        std::cout << "  " << std::left << std::setw(14) << abilityName(a) << before << " -> "
                   << color(std::to_string(after), kColorSelect) << " (" << delta << ")\n";
    }
    if (!anyAdjustment) {
        std::cout << "  No adjustments for this race.\n";
    }
    pauseForEnter();

    // Gnomes are forced into the Tinker class, not offered a choice --
    // Dragonlance Adventures (TSR 2021, p.57) states "Gnomes in Krynn can
    // only be of the tinker class," and Tinker exists in this project
    // specifically to make that true rather than as a menu option other
    // races could also pick (see docs/CHARACTER_NOTES.md).
    if (character.race == RaceId::Gnome) {
        character.charClass = ClassId::Tinker;
        printStepHeader(3, "CLASS");
        printScoresRecap(scores);
        std::cout << "As a Gnome, you are a Tinker -- Krynn's gnomes know no other calling.\n";
        pauseForEnter();
    } else {
        printStepHeader(3, "CLASS");
        printScoresRecap(scores);

        // Krynn-specific Mage prerequisite on top of the base INT 9+
        // (Dragonlance Adventures p.35, "Student Wizard Minimum Scores"):
        // Dexterity 6+. Deliberately left as a soft (annotated, not
        // blocking) check, same as the prime-requisite annotation below --
        // neither is part of this milestone's race-based eligibility work.
        bool mageDexOk = scores.dexterity >= 6;

        // Hard-blocks a class DMG Table 7 / Dragonlance Adventures marks
        // "N/E" (not eligible) for this race/subrace -- classLevelCap
        // returns 0 for those. Every race can be at least Fighter, Cleric,
        // or Thief (see Race.cpp's raceClassCaps/subraceClassCaps), so this
        // can't dead-end the player either.
        for (;;) {
            std::cout << "Choose a class:\n";
            for (size_t i = 0; i < kAllClasses.size(); ++i) {
                const ClassInfo& c = classInfo(kAllClasses[i]);
                bool qualifies = scores.get(c.primeRequisite) >= c.primeRequisiteMinimum;
                bool blockedByRace = classLevelCap(character.race, character.subrace, c.id) == 0;
                std::cout << "  " << (i + 1) << ". " << c.name;
                if (blockedByRace) {
                    std::cout << (c.id == ClassId::Mage ? "  (cannot learn arcane magic)"
                                                          : "  (not eligible for this race)");
                } else if (c.id == ClassId::Mage && !mageDexOk) {
                    std::cout << "  (wizardry on Krynn also requires Dexterity 6+)";
                } else if (!qualifies) {
                    std::cout << "  (does not meet prime requisite)";
                }
                std::cout << "\n";
            }
            int classChoice = promptChoice("> ", 1, static_cast<int>(kAllClasses.size()));
            ClassId candidate = kAllClasses[static_cast<size_t>(classChoice - 1)];
            if (classLevelCap(character.race, character.subrace, candidate) == 0) {
                std::cout << "\n" << classInfo(candidate).name << " is not open to a "
                           << (subraceInfo(character.subrace) != nullptr ? subraceInfo(character.subrace)->name
                                                                          : raceInfo(character.race).name)
                           << " character. Choose a different class.\n\n";
                continue;
            }
            character.charClass = candidate;
            break;
        }
    }
    const ClassInfo& chosenClass = classInfo(character.charClass);

    // Exceptional (percentile) Strength: PHB Table 1 (p.19) only lets
    // "warrior" group characters roll this at 18 Strength -- Fighter is
    // the only warrior-group class this project has (Knight of the Crown
    // is built on top of it, so it's covered too).
    if (character.charClass == ClassId::Fighter && scores.strength == 18) {
        character.exceptionalStrengthPercentile = roll(1, 100);
    }

    printStepHeader(4, "ALIGNMENT");
    printScoresRecap(scores);

    // Hard-blocks Evil for Kender (Dragonlance Adventures p.53: "No evil
    // kender are known to exist") -- same reject-and-reprompt pattern as
    // the race/class prompts above. Every other race/class combination
    // still picks freely from all 9, so this loop can't dead-end anyone.
    for (;;) {
        std::cout << "Choose an alignment:\n";
        for (int i = 0; i < 9; ++i) {
            Alignment a = static_cast<Alignment>(i);
            std::cout << "  " << (i + 1) << ". " << alignmentName(a);
            if (!meetsAlignmentRestriction(character.race, a)) {
                std::cout << "  (kender cannot be evil)";
            }
            std::cout << "\n";
        }
        Alignment candidate = static_cast<Alignment>(promptChoice("> ", 1, 9) - 1);
        if (!meetsAlignmentRestriction(character.race, candidate)) {
            std::cout << "\nKender cannot be of evil alignment. Choose a different alignment.\n\n";
            continue;
        }
        character.alignment = candidate;
        break;
    }

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

    // Weapon Specialization (PHB p.71-73): an optional rule limited to
    // single-class fighters. Every Fighter here already qualifies -- this
    // project has no multi-classing -- so it's offered unconditionally to
    // the class, race-agnostic (a Kender wielding a Hoopak may specialize
    // too; the book allows "any weapon"). See character::Character::
    // specializedWeapon and docs/CHARACTER_NOTES.md's "Weapon
    // Specialization" section for why this is a flat yes/no rather than
    // tracked proficiency slots.
    if (character.charClass == ClassId::Fighter) {
        std::cout << "\nYou may specialize in your weapon, gaining +1 to hit and +2 damage "
                      "with it, plus faster extra attacks as you level (PHB, Weapon "
                      "Specialization).\n";
        if (promptYesNo("Specialize? (y/n) ")) {
            character.specializedWeapon = true;
        }
    }

    bool isWarrior = character.charClass == ClassId::Fighter;
    character.maxHp = std::max(1, chosenClass.hitDieSides + hpAdjustmentForConstitution(scores.constitution, isWarrior));
    character.currentHp = character.maxHp;
    // Kender start with a Hoopak instead of their class's normal starting
    // weapon -- a real, race-appropriate weapon (see Equipment.h's
    // kHoopakName doc comment for sourcing), not a fabricated flavor
    // choice, and consistent with DQoK's own "Only usable by kender
    // characters" restriction. This doesn't block a Kender from later
    // buying their class's own WeaponUpgrade instead -- both are just
    // named weapons, purchaseItem doesn't care what's currently equipped.
    if (character.race == RaceId::Kender) {
        character.weaponName = kHoopakName;
        character.weaponDamageSides = kHoopakDamageSides;
        character.weaponDamageBonus = kHoopakDamageBonus;
    } else {
        character.weaponName = chosenClass.weaponName;
        character.weaponDamageSides = chosenClass.weaponDamageSides;
        character.weaponDamageBonus = 0;
    }
    recomputeArmorClass(character); // equippedArmor is still None here, so this is just the Dex adjustment
    character.thac0 = 20; // true to 2e: every class starts at THAC0 20, diverging only as levels are gained

    character.saves = chosenClass.level1Saves;
    applyRacialSavingThrowBonus(character.race, scores.constitution, character.saves);

    character.steelPieces =
        (roll(chosenClass.steelDiceCount, chosenClass.steelDiceSides) + chosenClass.steelFlatBonus) *
        chosenClass.steelMultiplier;

    const SubraceInfo* sub = subraceInfo(character.subrace);
    clearScreen();
    std::cout << color("=== " + character.name + " ===", kColorName) << "\n";
    std::cout << (sub != nullptr ? sub->name : raceInfo(character.race).name) << " " << chosenClass.name
               << ", " << alignmentName(character.alignment) << "\n";
    if (character.knightOrder != KnightOrder::None) {
        std::cout << knightOrderName(character.knightOrder) << "\n";
    }
    if (character.charClass == ClassId::Mage) {
        std::cout << "An unaffiliated student of the arcane -- a Robe and Order of High Sorcery\n"
                      "await at higher levels.\n";
    }
    std::cout << "\n" << color("Ability Scores:", kColorLabel) << "\n";
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
    std::cout << "\nHP " << character.maxHp << "   AC " << character.armorClass
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
