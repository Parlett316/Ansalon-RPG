#include "character/CharacterCreator.h"
#include "character/CharClass.h"
#include "character/Companion.h"
#include "character/Race.h"
#include "combat/Monster.h"
#include "combat/MonsterLoader.h"
#include "game/GameLoop.h"
#include "game/GameState.h"
#include "game/SaveGame.h"
#include "quest/Quest.h"
#include "quest/QuestLoader.h"
#include "render/Console.h"
#include "render/MapRenderer.h"
#include "timeline/Timeline.h"
#include "timeline/TimelineLoader.h"
#include "world/OverworldGrid.h"
#include "world/World.h"
#include "world/WorldLoader.h"
#include "world/ZoneCatalog.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>

#ifndef ANSALON_DATA_DIR
// CMakeLists.txt always supplies this via target_compile_definitions. The
// fallback here only exists so editor/IDE tooling that parses this file
// without running through CMake configuration still sees something valid.
#define ANSALON_DATA_DIR "data"
#endif

#ifndef ANSALON_SAVE_FILE_BASE
// Same reasoning/fallback as ANSALON_DATA_DIR above.
#define ANSALON_SAVE_FILE_BASE "save"
#endif

namespace {
constexpr const char* kStartingLocationId = "solace";

// Milestone 89: 3 independent save slots (save1.txt/save2.txt/save3.txt)
// replacing the old single save.txt -- see docs/ARCHITECTURE.md's
// "Save/load". Fixed, not configurable: "at least 3" was the ask, and a
// generic N-slot system isn't needed for it (CLAUDE.md's "no premature
// abstraction").
constexpr int kSaveSlotCount = 3;

// Same reprompt-until-valid, throw-on-EOF idiom as
// character::CharacterCreator.cpp's file-local promptLine/promptChoice --
// this file uses the same plain std::cin/std::cout interaction mode,
// before GameLoop's raw-keypress world starts (see docs/ARCHITECTURE.md).
// Kept as its own small copy here rather than shared across translation
// units, same "each loader/prompt owns its own tiny copy" precedent as
// trim/splitKeyword in the various *Loader.cpp files.
std::string promptLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw std::runtime_error("input ended unexpectedly at the save-slot menu");
    }
    return line;
}

// A menu pick, plus whether it was prefixed 'd'/'D' (e.g. "d1") requesting
// immediate deletion of that slot rather than selecting it.
struct SlotChoice {
    int slot;
    bool deleteRequested;
};

SlotChoice promptSlotChoice() {
    for (;;) {
        std::string line = promptLine("Choose a slot (1-" + std::to_string(kSaveSlotCount) +
                                       "), or 'd' plus a slot number (e.g. 'd1') to delete a save: ");
        size_t begin = line.find_first_not_of(" \t\r\n");
        if (begin != std::string::npos) {
            size_t end = line.find_last_not_of(" \t\r\n");
            std::string trimmed = line.substr(begin, end - begin + 1);
            bool deleteRequested = trimmed[0] == 'd' || trimmed[0] == 'D';
            std::istringstream iss(deleteRequested ? trimmed.substr(1) : trimmed);
            int value;
            if (iss >> value && value >= 1 && value <= kSaveSlotCount && iss.eof()) {
                return {value, deleteRequested};
            }
        }
        std::cout << "Please enter a number from 1 to " << kSaveSlotCount
                   << ", or 'd' plus a slot number.\n";
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

// One save slot's resolved state at startup: whether a file exists there,
// and if so, whether it loaded cleanly (a display summary + the real
// game::GameState, ready to hand to GameLoop) or not (the error that,
// before Milestone 89, would have aborted the entire program -- now it
// just marks this one slot unreadable and leaves the other two playable).
struct SlotInfo {
    std::string path;
    bool exists = false;
    bool valid = false;
    game::GameState state;
    std::string summary; // set when valid
    std::string error;   // set when exists && !valid
};

// Mirrors the single-save cross-check main() used to do inline (a saved
// ZONE that no longer exists in the current ZoneCatalog) -- SaveGame::load
// itself can't do this check (it doesn't know about world::ZoneCatalog,
// same one-way dependency direction as WorldLoader not knowing about
// OverworldGrid). Any exception here (a malformed file, or the stale-zone
// case thrown below) is caught and turned into SlotInfo::error rather than
// propagating, so one bad slot can't take down the whole menu.
SlotInfo describeSlot(std::string path, const world::ZoneCatalog& zones) {
    SlotInfo info;
    info.exists = game::SaveGame::exists(path);
    info.path = std::move(path);
    if (!info.exists) return info;
    try {
        game::GameState loaded = game::SaveGame::load(info.path);
        if (loaded.mode == game::Mode::Zone && !zones.hasZone(loaded.currentZoneId)) {
            throw std::runtime_error("references a zone ('" + loaded.currentZoneId +
                                      "') that no longer exists");
        }
        const character::Character& c = loaded.character;
        info.summary = c.name + ", level " + std::to_string(c.level) + " " +
                       std::string(character::raceInfo(c.race).name) + " " +
                       std::string(character::classInfo(c.charClass).name) + " (Day " +
                       std::to_string(loaded.hoursElapsed / 24) + ")";
        info.state = std::move(loaded);
        info.valid = true;
    } catch (const std::exception& ex) {
        info.error = ex.what();
    }
    return info;
}

// Shared by the menu printout and the delete-confirmation prompt below, so
// both describe an occupied/unreadable/empty slot identically.
std::string slotLabel(const SlotInfo& slot) {
    if (!slot.exists) return "(empty)";
    if (slot.valid) return slot.summary;
    return "(unreadable save: " + slot.error + ")";
}

} // namespace

int main(int argc, char** argv) {
    // Opt-in diagnostics for a content author checking data/timeline.txt
    // after an edit -- see TimelineLoader::loadFromFile and
    // docs/TIMELINE_NOTES.md's "Keyword-collision warning". Off by
    // default so a normal player launch never prints loader-internal
    // advisories to the console.
    bool checkTimeline = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--check-timeline") checkTimeline = true;
    }


    // Console's constructor enables ANSI/VT100 escape processing for the
    // lifetime of this scope (Windows only -- see render/Console.cpp) and
    // restores the original console mode when it goes out of scope at the
    // end of main().
    render::Console console;

    // Sizes MapRenderer's layout to the real, currently visible console
    // window (not the scrollback buffer -- see Console::currentWindowSize
    // and docs/GOTCHAS.md) before anything renders. Must happen before any
    // GameLoop/MapRenderer usage below. Same "fail fast with a clear
    // message" convention as the starting-location checks further down --
    // not a new pattern.
    render::WindowSize windowSize = render::Console::currentWindowSize();
    if (!render::MapRenderer::configureLayout(windowSize.columns, windowSize.rows)) {
        std::cerr << "Terminal too small: detected " << windowSize.columns << "x" << windowSize.rows
                   << ", need at least " << render::MapRenderer::kAbsoluteMinColumns << "x"
                   << render::MapRenderer::kAbsoluteMinRows << ". Enlarge your terminal window and try again.\n";
        return 1;
    }

    // Resolved relative to the running executable so a distributed build
    // (see tools/package_release.ps1) finds its own data/ and save slots
    // wherever it's unzipped, instead of the compile-time source-tree path
    // baked in below. Falls back to the compile-time ANSALON_DATA_DIR /
    // ANSALON_SAVE_FILE_BASE only if the executable's own path can't be
    // determined (non-Windows, or editor tooling that skips CMake
    // configuration) -- see docs/GOTCHAS.md.
    const std::string exeDir = render::Console::executableDirectory();
    const std::string dataDir = exeDir.empty() ? ANSALON_DATA_DIR : exeDir + "/data";
    const std::string saveBase = exeDir.empty() ? ANSALON_SAVE_FILE_BASE : exeDir + "/save";

    try {
        world::OverworldGrid grid =
            world::OverworldGrid::loadFromFile(dataDir + "/overworld.grid", dataDir + "/overworld_regions.grid");

        world::World world;
        world::WorldLoader::loadFromFile(dataDir + "/locations.txt", world);

        // Zones are loaded after World, deliberately: ZoneCatalog matches
        // zone files to locations by id, so it needs the location list
        // first -- see docs/ARCHITECTURE.md.
        world::ZoneCatalog zones =
            world::ZoneCatalog::loadForWorld(world, dataDir + "/zones");

        // Static content, same treatment as World/OverworldGrid/ZoneCatalog
        // above -- loaded fresh every run, never touched by GameState/save
        // data. See timeline::Timeline and docs/TIMELINE_NOTES.md.
        timeline::Timeline timeline;
        timeline::TimelineLoader::loadFromFile(dataDir + "/timeline.txt", timeline, checkTimeline);

        // Static content too, same treatment -- see combat::MonsterCatalog
        // and docs/COMBAT_NOTES.md.
        combat::MonsterCatalog monsters;
        combat::MonsterLoader::loadFromFile(dataDir + "/monsters.txt", monsters);

        // Static content too, loaded after zones so every zone's QUEST ids
        // can be cross-checked against it below (quest::QuestLoader itself
        // can't see zones -- see docs/QUEST_NOTES.md).
        quest::QuestCatalog quests;
        quest::QuestLoader::loadFromFile(dataDir + "/quests.txt", quests);

        // A zone's QUEST <char> <quest-id> line is validated by ZoneLoader
        // only against its own POI/TALK grammar (it can't see
        // quest::QuestCatalog) -- so a quest id that doesn't exist would
        // otherwise fail silently at play time (offerOrTurnInQuest just
        // returns). Cross-checked here, the established place for
        // cross-loader validation (see the starting-location checks above).
        for (const auto& [zoneId, zone] : zones.allZones()) {
            for (const auto& [code, questId] : zone.quests()) {
                if (quests.find(questId) == nullptr) {
                    std::cerr << "Zone '" << zoneId << "' offers quest '" << questId
                               << "' at POI '" << code << "', but no such quest is defined in "
                               << dataDir << "/quests.txt.\n";
                    return 1;
                }
            }
        }

        // Same reasoning as the QUEST cross-check above, for a zone's
        // SHOP_LOCKED <char> <quest-id> line -- ZoneLoader only validated
        // that the POI already has a SHOP line, not that the quest id is
        // real.
        for (const auto& [zoneId, zone] : zones.allZones()) {
            for (const auto& [code, questId] : zone.shopLocks()) {
                if (quests.find(questId) == nullptr) {
                    std::cerr << "Zone '" << zoneId << "' locks the shop at POI '" << code
                               << "' behind quest '" << questId << "', but no such quest is defined in "
                               << dataDir << "/quests.txt.\n";
                    return 1;
                }
            }
        }

        // Same reasoning as the QUEST/SHOP_LOCKED cross-checks above, for a
        // zone's RECRUIT <char> <companion-id> line -- ZoneLoader only
        // validated that the POI already has a TALK line, not that the
        // companion id is real (it can't see character:: -- see
        // docs/ZONE_NOTES.md's "Recruiting a companion").
        for (const auto& [zoneId, zone] : zones.allZones()) {
            for (const auto& [code, companionId] : zone.recruits()) {
                if (!character::isKnownCompanionId(companionId)) {
                    std::cerr << "Zone '" << zoneId << "' recruits companion '" << companionId
                               << "' at POI '" << code << "', but no such companion is defined.\n";
                    return 1;
                }
            }
        }

        const world::Location* start = world.getLocation(kStartingLocationId);
        if (!start) {
            std::cerr << "World data does not define the starting location '" << kStartingLocationId << "'.\n";
            return 1;
        }
        // Checked here, not in WorldLoader: WorldLoader has no knowledge of
        // OverworldGrid (deliberately decoupled -- see
        // docs/ARCHITECTURE.md), so grid-bounds validation for location
        // coordinates happens at this, the one place both are loaded together.
        if (start->x < 0 || start->y < 0 || start->x >= grid.width() || start->y >= grid.height()) {
            std::cerr << "Starting location '" << kStartingLocationId << "' is outside the overworld grid.\n";
            return 1;
        }

        // A pre-Milestone-89 single save.txt is migrated into Slot 1 the
        // first time it's found with no save1.txt already there -- this is
        // what carries an in-progress character forward with no manual
        // step. Non-fatal if the rename fails (e.g. permissions): the old
        // file is simply left where it is and the slot menu below shows
        // Slot 1 as empty.
        const std::string legacySavePath = saveBase + ".txt";
        const std::string slot1Path = saveBase + "1.txt";
        if (!game::SaveGame::exists(slot1Path) && game::SaveGame::exists(legacySavePath)) {
            std::error_code ec;
            std::filesystem::rename(legacySavePath, slot1Path, ec);
            if (!ec) {
                std::cout << "Found an existing save.txt -- migrated it to Save Slot 1.\n";
            } else {
                std::cerr << "Warning: found an existing save.txt but could not migrate it to "
                             "Slot 1 (" << ec.message() << "). Leaving it as-is.\n";
            }
        }

        std::array<SlotInfo, kSaveSlotCount> slots;
        for (int i = 0; i < kSaveSlotCount; ++i) {
            slots[static_cast<size_t>(i)] =
                describeSlot(saveBase + std::to_string(i + 1) + ".txt", zones);
        }

        // Slot picker -- plain std::cin/std::cout, same "before GameLoop's
        // raw-keypress world starts" interaction mode as CharacterCreator
        // (see docs/ARCHITECTURE.md). Picking an empty slot or confirming a
        // fresh character in an occupied one falls through to the ordinary
        // CharacterCreator flow below; picking "continue" loads that slot's
        // GameState directly.
        game::GameState state;
        bool loadedSave = false;
        std::string savePath;
        for (;;) {
            std::cout << "\nSave slots:\n";
            for (int i = 0; i < kSaveSlotCount; ++i) {
                std::cout << "  " << (i + 1) << ". " << slotLabel(slots[static_cast<size_t>(i)]) << "\n";
            }
            SlotChoice choice = promptSlotChoice();
            int chosen = choice.slot;
            SlotInfo& slot = slots[static_cast<size_t>(chosen - 1)];

            // Immediate, explicit delete -- distinct from the "start a new
            // character, overwrite on next autosave" flow below. Always
            // loops back to a freshly redrawn menu rather than falling
            // through to character creation.
            if (choice.deleteRequested) {
                if (!slot.exists) {
                    std::cout << "\nSlot " << chosen << " is already empty.\n";
                    continue;
                }
                if (promptYesNo("\nDelete Slot " + std::to_string(chosen) + " -- " + slotLabel(slot) +
                                 "? This cannot be undone. (y/n) ")) {
                    if (game::SaveGame::remove(slot.path)) {
                        slot = describeSlot(slot.path, zones);
                        std::cout << "Slot " << chosen << " deleted.\n";
                    } else {
                        std::cout << "Could not delete Slot " << chosen << "'s save file.\n";
                    }
                }
                continue;
            }

            savePath = slot.path;

            if (!slot.exists) break; // empty slot -- straight to character creation below

            if (slot.valid) {
                std::cout << "\n" << slot.summary << "\n";
                if (promptYesNo("Continue this character? (y/n) ")) {
                    state = std::move(slot.state);
                    loadedSave = true;
                    break;
                }
                if (promptYesNo("Start a new character in Slot " + std::to_string(chosen) +
                                 "? This will overwrite " + slot.state.character.name +
                                 " the next time you save. (y/n) ")) {
                    break; // fresh character -- straight to character creation below
                }
                continue; // back to the slot menu
            }

            std::cout << "\nSlot " << chosen << " could not be loaded: " << slot.error << "\n";
            if (promptYesNo("Start a new character in Slot " + std::to_string(chosen) +
                             " and overwrite it? (y/n) ")) {
                break;
            }
            continue;
        }

        // Character creation runs here, before the world is ever rendered:
        // it's a deliberate step-by-step wizard on plain std::cin/std::cout,
        // not part of the real-time keypress loop -- see
        // docs/ARCHITECTURE.md and character/CharacterCreator.h.
        if (!loadedSave) {
            state.character = character::CharacterCreator::run();
            state.x = start->x;
            state.y = start->y;
            state.visitedLocations.insert(start->id);
        }

        game::GameLoop loop(world, grid, zones, timeline, monsters, quests, std::move(state), savePath);
        loop.run();
    } catch (const std::exception& ex) {
        std::cerr << "Failed to start: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
