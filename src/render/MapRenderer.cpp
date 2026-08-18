#include "render/MapRenderer.h"
#include "character/CharClass.h"
#include "character/Knighthood.h"
#include "character/Race.h"
#include "character/Spellcasting.h"
#include "character/WizardOrder.h"
#include "world/Terrain.h"
#include "world/ZoneTile.h"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace render {

void MapRenderer::drawOverworldFrame(const world::OverworldGrid& grid, const world::World& world,
                                      const timeline::Timeline& timeline, const game::GameState& state,
                                      const std::string& message) {
    std::ostringstream out;

    // Center the viewport on the player, then clamp so it never scrolls
    // past the grid edge (which would show wasted blank space) -- the
    // camera simply stops following the player once they're within half a
    // viewport of a border.
    int left = std::clamp(state.x - kViewportWidth / 2, 0, std::max(0, grid.width() - kViewportWidth));
    int top = std::clamp(state.y - kViewportHeight / 2, 0, std::max(0, grid.height() - kViewportHeight));

    out << "\x1b[2J\x1b[H"; // clear + cursor home (see Console::clearScreen -- same VT100 sequence)

    for (int row = 0; row < kViewportHeight; ++row) {
        int gy = top + row;
        for (int col = 0; col < kViewportWidth; ++col) {
            int gx = left + col;

            char displayChar;
            const char* color;
            if (gx == state.x && gy == state.y) {
                displayChar = '@';
                color = "\x1b[97m"; // bright white -- the player must always read clearly against any terrain
            } else if (const world::Location* loc = world.locationAt(gx, gy)) {
                displayChar = loc->glyph;
                color = "\x1b[93m"; // bright yellow -- locations stand out from raw terrain
            } else {
                const world::TerrainInfo& terrain = world::terrainFor(grid.terrainCodeAt(gx, gy));
                displayChar = terrain.glyph;
                color = terrain.ansiColor;
            }

            // Every color-set code is paired with a reset immediately
            // after -- see docs/GOTCHAS.md on why (color bleed).
            out << color << displayChar << "\x1b[0m";
        }
        out << "\n";
    }

    out << "\n";
    if (const world::Location* here = world.locationAt(state.x, state.y)) {
        out << "== " << here->name << " (" << here->region << ") ==\n" << here->description << "\n";
        for (const timeline::Presence& presence :
             timeline.presentAt(here->id, static_cast<int>(state.hoursElapsed / 24))) {
            out << presence.character->name << ": " << presence.window->flavorText << "\n";
        }
    } else {
        const world::TerrainInfo& underfoot = world::terrainFor(grid.terrainCodeAt(state.x, state.y));
        out << "You are in " << underfoot.name << ".\n";
    }
    out << "Day " << (state.hoursElapsed / 24) << ", hour " << (state.hoursElapsed % 24) << ".";
    if (!message.empty()) {
        out << "  " << message;
    }
    out << "\n";
    out << "Move: arrows/hjkl/yubn/wasd   ;=look around   t=talk   p=shop   Enter=step in   q=quit\n";

    // The whole frame is built as one string and written in a single
    // flush -- this matters far more here than in Milestone 1, since a
    // redraw now happens on every single keystroke instead of every typed
    // command. Many small writes would flicker visibly. See docs/GOTCHAS.md.
    std::cout << out.str();
}

void MapRenderer::drawZoneFrame(const world::Zone& zone, const timeline::Timeline& timeline,
                                 const game::GameState& state, const std::string& message) {
    std::ostringstream out;
    out << "\x1b[2J\x1b[H";

    for (int y = 0; y < zone.height(); ++y) {
        for (int x = 0; x < zone.width(); ++x) {
            char displayChar;
            const char* color;
            if (x == state.zoneX && y == state.zoneY) {
                displayChar = '@';
                color = "\x1b[97m";
            } else if (const world::PointOfInterest* poi = zone.poiAt(x, y)) {
                displayChar = poi->code;
                color = "\x1b[93m"; // bright yellow, matching overworld location markers
            } else if (x == zone.entryX() && y == zone.entryY()) {
                displayChar = '>';
                color = "\x1b[96m"; // bright cyan -- the way back out
            } else {
                const world::ZoneTileInfo& tile = world::zoneTileFor(zone.tileCodeAt(x, y));
                displayChar = tile.glyph;
                color = tile.ansiColor;
            }
            out << color << displayChar << "\x1b[0m";
        }
        out << "\n";
    }

    out << "\n== " << zone.name() << " ==\n";
    if (const world::PointOfInterest* here = zone.poiAt(state.zoneX, state.zoneY)) {
        out << here->name << ": " << here->description << "\n";
        if (here->code == zone.timelineAnchorPoi()) {
            const std::string& effectiveId =
                zone.timelineLocationId().empty() ? state.currentZoneId : zone.timelineLocationId();
            for (const timeline::Presence& presence :
                 timeline.presentAt(effectiveId, static_cast<int>(state.hoursElapsed / 24))) {
                out << presence.character->name << ": " << presence.window->flavorText << "\n";
            }
        }
    } else if (state.zoneX == zone.entryX() && state.zoneY == zone.entryY()) {
        out << "You stand at the way back out.\n";
    } else {
        out << "\n";
    }
    if (!message.empty()) {
        out << message << "\n";
    }
    out << "Move: arrows/hjkl/yubn/wasd   ;=look   t=talk   p=shop   Enter=leave (from the '>' marker)   q=quit\n";

    std::cout << out.str();
}

void MapRenderer::drawCharacterSheet(const character::Character& c, long long currentDay) {
    std::ostringstream out;
    out << "\x1b[2J\x1b[H";

    const auto& race = character::raceInfo(c.race);
    const auto& cls = character::classInfo(c.charClass);
    const character::SubraceInfo* sub = character::subraceInfo(c.subrace);

    out << "=== " << c.name << " ===\n";
    out << (sub != nullptr ? sub->name : race.name) << " " << cls.name << ", level " << c.level
        << " (" << c.experience << " XP)\n";
    out << character::alignmentName(c.alignment) << "\n";
    if (c.knightOrder != character::KnightOrder::None) {
        out << character::knightOrderName(c.knightOrder) << "\n";
    }
    if (c.charClass == character::ClassId::Mage) {
        if (c.robeColor == character::RobeColor::None) {
            out << "Unaffiliated student of the arcane\n";
        } else {
            out << character::robeColorName(c.robeColor) << ", sworn to " << character::robeMoonName(c.robeColor)
                << "\n";
        }
    }
    out << "\n";

    out << "STR " << c.scores.strength;
    if (c.exceptionalStrengthPercentile > 0) {
        // 18/01-18/99 zero-pad to two digits; 18/00 is the top bracket,
        // conventionally written "00" rather than "100".
        int pct = c.exceptionalStrengthPercentile;
        out << "/" << (pct == 100 ? "00" : (pct < 10 ? "0" : "")) << (pct == 100 ? "" : std::to_string(pct));
    }
    out << "   DEX " << c.scores.dexterity << "   CON " << c.scores.constitution << "\n";
    out << "INT " << c.scores.intelligence << "   WIS " << c.scores.wisdom
        << "   CHA " << c.scores.charisma << "\n\n";

    out << "HP " << c.currentHp << "/" << c.maxHp << "   AC " << c.armorClass
        << "   THAC0 " << c.thac0 << "\n";
    out << "Weapon: " << c.weaponName;
    if (c.equippedArmor != character::ArmorId::None || c.hasShield) {
        out << "   Armor: ";
        if (c.equippedArmor != character::ArmorId::None) {
            out << character::armorInfo(c.equippedArmor).name;
            if (c.hasShield) out << " + Shield";
        } else {
            out << "Shield only";
        }
    }
    out << "\n\n";

    out << "Saving Throws:\n";
    for (int i = 0; i < static_cast<int>(character::SaveCategory::Count); ++i) {
        auto category = static_cast<character::SaveCategory>(i);
        out << "  " << character::saveCategoryName(category) << ": " << c.saves.at(category) << "\n";
    }
    out << "\nSteel: " << c.steelPieces << " stl\n";

    out << "Carried: ";
    if (c.inventory.empty()) {
        out << "nothing (press 'i' to view/equip)\n";
    } else {
        for (size_t i = 0; i < c.inventory.size(); ++i) {
            if (i > 0) out << ", ";
            out << character::inventoryItemLabel(c.inventory[i]);
        }
        out << " (press 'i' to equip)\n";
    }

    if (character::canCastSpells(c.charClass)) {
        int maxSlots = character::maxSpellSlotsPerDay(c);
        if (maxSlots == 0) {
            out << "\nSpells: cannot cast arcane magic\n";
        } else {
            int usedToday = c.spellsCastDay == currentDay ? c.spellsCastToday : 0;
            out << "\nSpells: " << character::knownSpellName(c.charClass) << " ("
                << (maxSlots - usedToday) << "/" << maxSlots << " remaining today)\n";
        }
    }

    out << "\n(press any key to continue)\n";

    std::cout << out.str();
}

void MapRenderer::drawCombatFrame(const character::Character& character, const combat::Monster& monster,
                                   int monsterHp, int monsterMaxHp, const std::vector<std::string>& log) {
    std::ostringstream out;
    out << "\x1b[2J\x1b[H";

    out << "=== Combat ===\n\n";
    out << character.name << " -- HP " << character.currentHp << "/" << character.maxHp << "   AC "
        << character.armorClass << "   Weapon: " << character.weaponName << "\n";
    out << monster.name << " -- HP " << std::max(0, monsterHp) << "/" << monsterMaxHp << "   AC "
        << monster.armorClass << "\n\n";

    // Only the tail fits comfortably in the viewport -- older lines scroll
    // off, same "most recent last" convention as a chat/console log.
    constexpr size_t kMaxLogLines = 12;
    size_t start = log.size() > kMaxLogLines ? log.size() - kMaxLogLines : 0;
    for (size_t i = start; i < log.size(); ++i) {
        out << log[i] << "\n";
    }

    if (character::canCastSpells(character.charClass)) {
        out << "\nEnter=attack   m=cast " << character::knownSpellName(character.charClass) << "   f=flee\n";
    } else {
        out << "\nEnter=attack   f=flee\n";
    }

    std::cout << out.str();
}

void MapRenderer::drawShopFrame(const character::Character& character, const std::string& shopName,
                                 const std::vector<character::ShopItem>& buyItems,
                                 const std::vector<character::SellItem>& sellItems, bool sellMode,
                                 int selectedIndex, const std::string& message) {
    std::ostringstream out;
    out << "\x1b[2J\x1b[H";

    out << "=== " << shopName << " ===\n";
    out << "Steel: " << character.steelPieces << " stl\n\n";

    if (sellMode) {
        out << "-- Selling --\n";
        for (size_t i = 0; i < sellItems.size(); ++i) {
            const character::SellItem& item = sellItems[i];
            out << (static_cast<int>(i) == selectedIndex ? "> " : "  ");
            out << item.label << " -- " << item.valueStl << " stl";
            if (!item.sellable) out << "  (cannot sell)";
            out << "\n";
        }
    } else {
        out << "-- Buying --\n";
        for (size_t i = 0; i < buyItems.size(); ++i) {
            const character::ShopItem& item = buyItems[i];
            out << (static_cast<int>(i) == selectedIndex ? "> " : "  ");
            out << item.label << " -- " << item.costStl << " stl";
            if (item.alreadyOwned) {
                out << "  (owned)";
            } else if (!item.buyable) {
                out << "  (cannot use)";
            }
            out << "\n";
        }
    }

    if (!message.empty()) {
        out << "\n" << message << "\n";
    }
    out << "\nup/down=select   Enter=" << (sellMode ? "sell" : "buy")
        << "   i=" << (sellMode ? "view buy list" : "view sell list") << "   q=leave\n";

    std::cout << out.str();
}

void MapRenderer::drawInventoryFrame(const character::Character& character, int selectedIndex) {
    std::ostringstream out;
    out << "\x1b[2J\x1b[H";

    out << "=== Inventory ===\n";
    out << "Weapon: " << character.weaponName << "\n";
    out << "Armor: " << (character.equippedArmor == character::ArmorId::None
                              ? "none"
                              : character::armorInfo(character.equippedArmor).name)
        << (character.hasShield ? " + Shield" : "") << "\n\n";

    if (character.inventory.empty()) {
        out << "(nothing carried)\n";
    } else {
        out << "Carried items:\n";
        for (size_t i = 0; i < character.inventory.size(); ++i) {
            out << (static_cast<int>(i) == selectedIndex ? "> " : "  ");
            out << character::inventoryItemLabel(character.inventory[i]) << "\n";
        }
    }

    out << "\nup/down=select   Enter=equip   q=leave\n";

    std::cout << out.str();
}

void MapRenderer::drawPickerFrame(const std::string& title, const std::vector<std::string>& items,
                                   int selectedIndex, const std::string& footer) {
    std::ostringstream out;
    out << "\x1b[2J\x1b[H";

    out << "=== " << title << " ===\n\n";
    for (size_t i = 0; i < items.size(); ++i) {
        out << (static_cast<int>(i) == selectedIndex ? "> " : "  ") << items[i] << "\n";
    }
    out << "\n" << footer << "\n";

    std::cout << out.str();
}

void MapRenderer::drawDialogueFrame(const std::vector<DialogueLine>& lines) {
    std::ostringstream out;
    out << "\x1b[2J\x1b[H";

    out << "=== Conversation ===\n\n";
    for (const auto& line : lines) {
        out << line.speaker << ": " << line.text << "\n\n";
    }
    out << "(press any key to continue)\n";

    std::cout << out.str();
}

} // namespace render
