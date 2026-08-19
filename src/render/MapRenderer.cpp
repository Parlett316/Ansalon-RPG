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

// Definitions for MapRenderer's adaptive layout members -- see
// MapRenderer.h for what each means. Default-initialized to the
// preferred values so anything that never calls configureLayout (a
// throwaway self-test, say) behaves exactly like a fixed-size build.
int MapRenderer::kViewportWidth = MapRenderer::kPreferredViewportWidth;
int MapRenderer::kViewportHeight = MapRenderer::kPreferredViewportHeight;
int MapRenderer::kLogPanelWidth = 40; // matches the original fixed default
int MapRenderer::kLogFrameWidth = 110;

namespace {

// Comfortable reading width for wrapped prose (dialogue text, a long
// carried-inventory listing, a monster description) inside a bordered
// "organic" screen -- see writeBoxed below and docs/ARCHITECTURE.md.
// Adaptive, like MapRenderer's own layout members above (updated by
// configureLayout) -- default matches the original fixed value.
int kProseWrapWidth = 76;
// An "organic" screen's border hugs its own content, clamped into this
// range so a single short line never produces a comically tiny box and a
// screen's content never produces a box wider than the main map/log
// frame. Only the max is console-size-dependent; the min is a fixed
// "never comically tiny" floor.
constexpr int kSecondaryBoxMinWidth = 20;
int kSecondaryBoxMaxWidth = 100;

// `[####......]` style bar -- current is clamped into [0, max] first so a
// display glitch never reads a negative/over-full bar even transiently.
std::string hpBar(int current, int max, int width) {
    int clampedCurrent = std::clamp(current, 0, std::max(max, 0));
    int filled = max > 0 ? (clampedCurrent * width) / max : 0;
    std::string bar = "[";
    bar.append(static_cast<size_t>(filled), '#');
    bar.append(static_cast<size_t>(width - filled), '.');
    bar += "]";
    return bar;
}

// Greedy word wrap for the log panel -- a single token longer than width
// is hard-broken (defensive; nothing currently authored should trigger
// it, but a future long unbroken string shouldn't misrender the panel).
std::vector<std::string> wrapText(const std::string& text, int width) {
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word, current;
    while (words >> word) {
        while (static_cast<int>(word.size()) > width) {
            lines.push_back(word.substr(0, static_cast<size_t>(width)));
            word = word.substr(static_cast<size_t>(width));
        }
        if (current.empty()) {
            current = word;
        } else if (static_cast<int>(current.size() + 1 + word.size()) <= width) {
            current += " " + word;
        } else {
            lines.push_back(current);
            current = word;
        }
    }
    if (!current.empty() || lines.empty()) lines.push_back(current);
    return lines;
}

// Wraps every raw log entry, takes the tail `height` physical lines (so
// once the panel fills, the oldest lines scroll off the top -- most
// recent always visible at the bottom, same convention as
// drawCombatFrame's own log), right-pads each to `width`, and pads the
// *end* with blank lines while there isn't yet `height` worth of content
// -- so early in a session the log anchors at the top and grows
// downward, matching the Caves of Qud reference this milestone is
// modeled on (see docs/ARCHITECTURE.md).
std::vector<std::string> buildLogPanel(const std::vector<std::string>& log, int width, int height) {
    std::vector<std::string> wrapped;
    for (const std::string& entry : log) {
        for (std::string& line : wrapText(entry, width)) wrapped.push_back(std::move(line));
    }
    size_t start = wrapped.size() > static_cast<size_t>(height) ? wrapped.size() - static_cast<size_t>(height) : 0;
    std::vector<std::string> panel(wrapped.begin() + static_cast<long>(start), wrapped.end());
    for (std::string& line : panel) {
        if (static_cast<int>(line.size()) < width) line.append(static_cast<size_t>(width) - line.size(), ' ');
    }
    while (static_cast<int>(panel.size()) < height) panel.push_back(std::string(static_cast<size_t>(width), ' '));
    return panel;
}

// Shared 2-line HUD (name/day-hour/steel, HP bar/AC/THAC0) used by both
// drawOverworldFrame and drawZoneFrame -- appends its lines (no trailing
// blank line; callers add their own blank separator) to `lines`. Plain
// text, no embedded ANSI codes, so callers can pad these with padPlain.
void writeHud(std::vector<std::string>& lines, const character::Character& c, const game::GameState& state) {
    std::ostringstream line1;
    line1 << c.name << "   Day " << (state.hoursElapsed / 24) << ", hour " << (state.hoursElapsed % 24)
          << "   Steel: " << c.steelPieces << " stl";
    lines.push_back(line1.str());

    std::ostringstream line2;
    line2 << "HP " << hpBar(c.currentHp, c.maxHp, 24) << " " << c.currentHp << "/" << c.maxHp << "   AC "
          << c.armorClass << "   THAC0 " << c.thac0;
    lines.push_back(line2.str());
}

// Pads (or defensively truncates) a plain-text line to exactly `width`
// visible columns. Only ever used on lines with no embedded ANSI color
// codes -- measuring "visible width" through an escape sequence would
// silently miscount, which is why the map+log rows (which do carry
// per-glyph color codes) are never passed through this function; they're
// already exactly the right width by construction instead.
std::string padPlain(std::string line, int width) {
    if (static_cast<int>(line.size()) > width) line.resize(static_cast<size_t>(width));
    line.append(static_cast<size_t>(width) - line.size(), ' ');
    return line;
}

// Wraps any line longer than kProseWrapWidth via wrapText above (dialogue
// text, a long carried-inventory listing, a monster description) -- short
// lines (picker items, a "HP 17/30" style field) pass through untouched.
std::vector<std::string> wrapLongLines(const std::vector<std::string>& lines) {
    std::vector<std::string> result;
    for (const std::string& line : lines) {
        if (static_cast<int>(line.size()) > kProseWrapWidth) {
            for (std::string& wrapped : wrapText(line, kProseWrapWidth)) result.push_back(std::move(wrapped));
        } else {
            result.push_back(line);
        }
    }
    return result;
}

// Low-level plain-ASCII border primitive shared by every screen in the
// game -- see docs/ARCHITECTURE.md. Every element of `exactWidthLines`
// MUST already be exactly `width` visible columns: plain-text callers pad
// via padPlain first (see writeBoxed below); the map+log rows (which
// carry embedded ANSI color codes) are already exactly `width` by
// construction and must never be re-measured/padded here. Folds `title`
// into the top border as a title bar ("+-- Title ---...--+") instead of
// a separate content line.
void writeBorder(std::ostringstream& out, const std::string& title, int width,
                  const std::vector<std::string>& exactWidthLines) {
    std::string titleBar = "-- " + title + " ";
    if (static_cast<int>(titleBar.size()) < width + 2) {
        titleBar.append(static_cast<size_t>(width + 2) - titleBar.size(), '-');
    }
    out << "+" << titleBar << "+\n";
    for (const std::string& line : exactWidthLines) {
        out << "| " << line << " |\n";
    }
    out << "+" << std::string(static_cast<size_t>(width + 2), '-') << "+\n";
}

// Wraps `rawLines` (see wrapLongLines) and hands them to writeBorder,
// sizing the box to hug the actual content: width is the longest
// resulting line (or the title's own minimum width, whichever is
// bigger), clamped into [kSecondaryBoxMinWidth, kSecondaryBoxMaxWidth].
// Used by every "organic" screen (character sheet, combat, dialogue,
// picker, shop, inventory) -- see docs/ARCHITECTURE.md. The "fixed"
// screens (drawOverworldFrame/drawZoneFrame's map+log block,
// drawLogFrame's already-exact-width pager) build their own exact-width
// lines and call writeBorder directly instead, since some of that
// content carries ANSI color codes this width-measuring helper can't
// safely handle.
void writeBoxed(std::ostringstream& out, const std::string& title, const std::vector<std::string>& rawLines) {
    std::vector<std::string> wrapped = wrapLongLines(rawLines);
    int width = static_cast<int>(title.size()) + 2; // room for the title bar's "-- " + " " decoration
    for (const std::string& line : wrapped) width = std::max(width, static_cast<int>(line.size()));
    width = std::clamp(width, kSecondaryBoxMinWidth, kSecondaryBoxMaxWidth);

    std::vector<std::string> exact;
    exact.reserve(wrapped.size());
    for (const std::string& line : wrapped) exact.push_back(padPlain(line, width));

    writeBorder(out, title, width, exact);
}

} // namespace

bool MapRenderer::configureLayout(int columns, int rows) {
    if (columns < kAbsoluteMinColumns || rows < kAbsoluteMinRows) return false;

    // Map width gets priority up to the preferred size; only shrinks
    // toward the floor if that's needed to still guarantee the log panel
    // its own minimum width.
    int contentWidth = columns - kChromeColumns;
    int mapWidth = kPreferredViewportWidth;
    if (mapWidth + kLogPanelGap + kMinLogPanelWidth > contentWidth) {
        mapWidth = std::max(kMinViewportWidth, contentWidth - kLogPanelGap - kMinLogPanelWidth);
    }
    int logWidth = std::clamp(contentWidth - mapWidth - kLogPanelGap, kMinLogPanelWidth, kMaxLogPanelWidth);

    int contentHeight = rows - kChromeRows;
    int mapHeight = std::min(kPreferredViewportHeight, contentHeight);

    kViewportWidth = mapWidth;
    kViewportHeight = mapHeight;
    kLogPanelWidth = logWidth;
    kLogFrameWidth = std::min(110, contentWidth);

    // The "organic" screen internals (character sheet, combat, dialogue,
    // picker, shop, inventory) need to fit the same real console -- a
    // narrow-but-still-valid terminal could otherwise overflow on a long
    // dialogue line even though the main frame fit fine.
    kSecondaryBoxMaxWidth = std::min(100, contentWidth);
    kProseWrapWidth = std::min(76, kSecondaryBoxMaxWidth);

    return true;
}

void MapRenderer::drawOverworldFrame(const world::OverworldGrid& grid, const world::World& world,
                                      const game::GameState& state, const std::vector<std::string>& log) {
    // Center the viewport on the player, then clamp so it never scrolls
    // past the grid edge (which would show wasted blank space) -- the
    // camera simply stops following the player once they're within half a
    // viewport of a border.
    int left = std::clamp(state.x - kViewportWidth / 2, 0, std::max(0, grid.width() - kViewportWidth));
    int top = std::clamp(state.y - kViewportHeight / 2, 0, std::max(0, grid.height() - kViewportHeight));

    const int kContentWidth = kViewportWidth + kLogPanelGap + kLogPanelWidth;

    std::vector<std::string> lines;
    std::vector<std::string> hudLines;
    writeHud(hudLines, state.character, state);
    for (const std::string& hudLine : hudLines) lines.push_back(padPlain(hudLine, kContentWidth));
    lines.push_back(padPlain("", kContentWidth));

    std::vector<std::string> logPanel = buildLogPanel(log, kLogPanelWidth, kViewportHeight);
    for (int row = 0; row < kViewportHeight; ++row) {
        int gy = top + row;
        std::ostringstream rowStream;
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
            rowStream << color << displayChar << "\x1b[0m";
        }
        rowStream << std::string(static_cast<size_t>(kLogPanelGap), ' ') << logPanel[static_cast<size_t>(row)];
        // Not padded via padPlain -- already exactly kContentWidth visible
        // columns by construction (kViewportWidth glyphs + gap + a
        // pre-padded kLogPanelWidth-wide log panel line).
        lines.push_back(rowStream.str());
    }

    lines.push_back(padPlain("", kContentWidth));
    lines.push_back(padPlain(
        "Move: arrows/hjkl/yubn/wasd   ;=look around   t=talk   p=shop   v=log   Enter=step in   q=quit",
        kContentWidth));

    std::ostringstream out;
    out << "\x1b[2J\x1b[H"; // clear + cursor home (see Console::clearScreen -- same VT100 sequence)
    writeBorder(out, "Ansalon: Age of Despair", kContentWidth, lines);

    // The whole frame is built as one string and written in a single
    // flush -- this matters far more here than in Milestone 1, since a
    // redraw now happens on every single keystroke instead of every typed
    // command. Many small writes would flicker visibly. See docs/GOTCHAS.md.
    std::cout << out.str();
}

void MapRenderer::drawZoneFrame(const world::Zone& zone, const game::GameState& state,
                                 const std::vector<std::string>& log) {
    const int kContentWidth = kViewportWidth + kLogPanelGap + kLogPanelWidth;

    std::vector<std::string> lines;
    std::vector<std::string> hudLines;
    writeHud(hudLines, state.character, state);
    for (const std::string& hudLine : hudLines) lines.push_back(padPlain(hudLine, kContentWidth));
    lines.push_back(padPlain("", kContentWidth));

    // Always renders the full kViewportWidth/Height, not zone.width()/
    // height() -- a zone smaller than the viewport wall-pads out to fill
    // it (Zone::tileCodeAt/poiAt return '#'/nullptr for any out-of-bounds
    // coordinate, so this is safe -- see docs/GOTCHAS.md), which keeps the
    // log panel's left edge at a stable screen column regardless of which
    // zone is showing.
    std::vector<std::string> logPanel = buildLogPanel(log, kLogPanelWidth, kViewportHeight);
    for (int y = 0; y < kViewportHeight; ++y) {
        std::ostringstream rowStream;
        for (int x = 0; x < kViewportWidth; ++x) {
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
            rowStream << color << displayChar << "\x1b[0m";
        }
        rowStream << std::string(static_cast<size_t>(kLogPanelGap), ' ') << logPanel[static_cast<size_t>(y)];
        lines.push_back(rowStream.str());
    }

    lines.push_back(padPlain("", kContentWidth));
    lines.push_back(padPlain(
        "Move: arrows/hjkl/yubn/wasd   ;=look   t=talk   p=shop   v=log   Enter=leave (from the '>' marker)   q=quit",
        kContentWidth));

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBorder(out, "Ansalon: Age of Despair", kContentWidth, lines);

    std::cout << out.str();
}

void MapRenderer::drawCharacterSheet(const character::Character& c, long long currentDay) {
    const auto& race = character::raceInfo(c.race);
    const auto& cls = character::classInfo(c.charClass);
    const character::SubraceInfo* sub = character::subraceInfo(c.subrace);

    std::vector<std::string> lines;

    std::ostringstream classLine;
    classLine << (sub != nullptr ? sub->name : race.name) << " " << cls.name << ", level " << c.level << " ("
              << c.experience << " XP)";
    lines.push_back(classLine.str());
    lines.push_back(character::alignmentName(c.alignment));
    if (c.knightOrder != character::KnightOrder::None) {
        lines.push_back(character::knightOrderName(c.knightOrder));
    }
    if (c.charClass == character::ClassId::Mage) {
        if (c.robeColor == character::RobeColor::None) {
            lines.push_back("Unaffiliated student of the arcane");
        } else {
            std::ostringstream robeLine;
            robeLine << character::robeColorName(c.robeColor) << ", sworn to " << character::robeMoonName(c.robeColor);
            lines.push_back(robeLine.str());
        }
    }
    lines.push_back("");

    std::ostringstream abilities1;
    abilities1 << "STR " << c.scores.strength;
    if (c.exceptionalStrengthPercentile > 0) {
        // 18/01-18/99 zero-pad to two digits; 18/00 is the top bracket,
        // conventionally written "00" rather than "100".
        int pct = c.exceptionalStrengthPercentile;
        abilities1 << "/" << (pct == 100 ? "00" : (pct < 10 ? "0" : "")) << (pct == 100 ? "" : std::to_string(pct));
    }
    abilities1 << "   DEX " << c.scores.dexterity << "   CON " << c.scores.constitution;
    lines.push_back(abilities1.str());

    std::ostringstream abilities2;
    abilities2 << "INT " << c.scores.intelligence << "   WIS " << c.scores.wisdom << "   CHA " << c.scores.charisma;
    lines.push_back(abilities2.str());
    lines.push_back("");

    std::ostringstream hpLine;
    hpLine << "HP " << c.currentHp << "/" << c.maxHp << "   AC " << c.armorClass << "   THAC0 " << c.thac0;
    lines.push_back(hpLine.str());

    std::ostringstream weaponLine;
    weaponLine << "Weapon: " << c.weaponName;
    if (c.equippedArmor != character::ArmorId::None || c.hasShield) {
        weaponLine << "   Armor: ";
        if (c.equippedArmor != character::ArmorId::None) {
            weaponLine << character::armorInfo(c.equippedArmor).name;
            if (c.hasShield) weaponLine << " + Shield";
        } else {
            weaponLine << "Shield only";
        }
    }
    lines.push_back(weaponLine.str());
    lines.push_back("");

    lines.push_back("Saving Throws:");
    for (int i = 0; i < static_cast<int>(character::SaveCategory::Count); ++i) {
        auto category = static_cast<character::SaveCategory>(i);
        std::ostringstream saveLine;
        saveLine << "  " << character::saveCategoryName(category) << ": " << c.saves.at(category);
        lines.push_back(saveLine.str());
    }
    lines.push_back("");

    std::ostringstream steelLine;
    steelLine << "Steel: " << c.steelPieces << " stl";
    lines.push_back(steelLine.str());

    std::ostringstream carriedLine;
    carriedLine << "Carried: ";
    if (c.inventory.empty()) {
        carriedLine << "nothing (press 'i' to view/equip)";
    } else {
        for (size_t i = 0; i < c.inventory.size(); ++i) {
            if (i > 0) carriedLine << ", ";
            carriedLine << character::inventoryItemLabel(c.inventory[i]);
        }
        carriedLine << " (press 'i' to equip)";
    }
    lines.push_back(carriedLine.str());

    if (character::canCastSpells(c.charClass)) {
        lines.push_back("");
        int maxSlots = character::maxSpellSlotsPerDay(c);
        if (maxSlots == 0) {
            lines.push_back("Spells: cannot cast arcane magic");
        } else if (c.spellsCastDay != currentDay) {
            // Not memorized today -- see character::memorizeSpells /
            // game::GameLoop::handleRest ('r').
            std::ostringstream spellLine;
            spellLine << "Spells: " << character::knownSpellName(c.charClass)
                       << " (not memorized today -- rest to prepare)";
            lines.push_back(spellLine.str());
        } else {
            std::ostringstream spellLine;
            spellLine << "Spells: " << character::knownSpellName(c.charClass) << " ("
                       << (maxSlots - c.spellsCastToday) << "/" << maxSlots << " remaining today)";
            lines.push_back(spellLine.str());
        }
    }

    lines.push_back("");
    lines.push_back("(press any key to continue)");

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, c.name, lines);
    std::cout << out.str();
}

void MapRenderer::drawCombatFrame(const character::Character& character, const combat::Monster& monster,
                                   int monsterHp, int monsterMaxHp, const std::vector<std::string>& log) {
    std::vector<std::string> lines;

    std::ostringstream playerLine;
    playerLine << character.name << " -- HP " << character.currentHp << "/" << character.maxHp << "   AC "
               << character.armorClass << "   Weapon: " << character.weaponName;
    lines.push_back(playerLine.str());

    std::ostringstream monsterLine;
    monsterLine << monster.name << " -- HP " << std::max(0, monsterHp) << "/" << monsterMaxHp << "   AC "
                << monster.armorClass;
    lines.push_back(monsterLine.str());
    lines.push_back("");

    // Only the tail fits comfortably in the viewport -- older lines scroll
    // off, same "most recent last" convention as a chat/console log.
    constexpr size_t kMaxLogLines = 12;
    size_t start = log.size() > kMaxLogLines ? log.size() - kMaxLogLines : 0;
    for (size_t i = start; i < log.size(); ++i) lines.push_back(log[i]);

    lines.push_back("");
    std::ostringstream footer;
    footer << "Enter=attack";
    if (character::canCastSpells(character.charClass)) {
        footer << "   m=cast " << character::knownSpellName(character.charClass);
    }
    // Only hinted when there's actually a potion to drink -- same "only
    // show it when it's usable" precedent m=cast already follows for
    // non-casters.
    if (character::firstPotionIndex(character) >= 0) {
        footer << "   i=drink potion";
    }
    footer << "   f=flee";
    lines.push_back(footer.str());

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, "Combat", lines);
    std::cout << out.str();
}

void MapRenderer::drawShopFrame(const character::Character& character, const std::string& shopName,
                                 const std::vector<character::ShopItem>& buyItems,
                                 const std::vector<character::SellItem>& sellItems, bool sellMode,
                                 int selectedIndex, const std::string& message) {
    std::vector<std::string> lines;

    std::ostringstream steelLine;
    steelLine << "Steel: " << character.steelPieces << " stl";
    lines.push_back(steelLine.str());
    lines.push_back("");

    if (sellMode) {
        lines.push_back("-- Selling --");
        for (size_t i = 0; i < sellItems.size(); ++i) {
            const character::SellItem& item = sellItems[i];
            std::ostringstream itemLine;
            itemLine << (static_cast<int>(i) == selectedIndex ? "> " : "  ") << item.label << " -- "
                     << item.valueStl << " stl";
            if (!item.sellable) itemLine << "  (cannot sell)";
            lines.push_back(itemLine.str());
        }
    } else {
        lines.push_back("-- Buying --");
        for (size_t i = 0; i < buyItems.size(); ++i) {
            const character::ShopItem& item = buyItems[i];
            std::ostringstream itemLine;
            itemLine << (static_cast<int>(i) == selectedIndex ? "> " : "  ") << item.label << " -- "
                     << item.costStl << " stl";
            if (item.alreadyOwned) {
                itemLine << "  (owned)";
            } else if (!item.buyable) {
                itemLine << "  (cannot use)";
            }
            lines.push_back(itemLine.str());
        }
    }

    if (!message.empty()) {
        lines.push_back("");
        lines.push_back(message);
    }
    lines.push_back("");
    std::ostringstream footer;
    footer << "up/down=select   Enter=" << (sellMode ? "sell" : "buy")
           << "   i=" << (sellMode ? "view buy list" : "view sell list") << "   q=leave";
    lines.push_back(footer.str());

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, shopName, lines);
    std::cout << out.str();
}

void MapRenderer::drawInventoryFrame(const character::Character& character, int selectedIndex) {
    std::vector<std::string> lines;

    // Shown mainly so drinking a potion here (see GameLoop::handleInventory)
    // is visibly reflected the same way equipping already is via the
    // Weapon:/Armor: lines below -- this screen has no separate message
    // parameter the way drawShopFrame does.
    std::ostringstream hpLine;
    hpLine << "HP: " << character.currentHp << "/" << character.maxHp;
    lines.push_back(hpLine.str());

    std::ostringstream weaponLine;
    weaponLine << "Weapon: " << character.weaponName;
    lines.push_back(weaponLine.str());

    std::ostringstream armorLine;
    armorLine << "Armor: "
              << (character.equippedArmor == character::ArmorId::None ? "none"
                                                                        : character::armorInfo(character.equippedArmor).name)
              << (character.hasShield ? " + Shield" : "");
    lines.push_back(armorLine.str());
    lines.push_back("");

    if (character.inventory.empty()) {
        lines.push_back("(nothing carried)");
    } else {
        lines.push_back("Carried items:");
        for (size_t i = 0; i < character.inventory.size(); ++i) {
            std::ostringstream itemLine;
            itemLine << (static_cast<int>(i) == selectedIndex ? "> " : "  ")
                     << character::inventoryItemLabel(character.inventory[i]);
            lines.push_back(itemLine.str());
        }
    }

    lines.push_back("");
    lines.push_back("up/down=select   Enter=equip/use   q=leave");

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, "Inventory", lines);
    std::cout << out.str();
}

void MapRenderer::drawPickerFrame(const std::string& title, const std::vector<std::string>& items,
                                   int selectedIndex, const std::string& footer) {
    std::vector<std::string> lines;
    for (size_t i = 0; i < items.size(); ++i) {
        std::ostringstream itemLine;
        itemLine << (static_cast<int>(i) == selectedIndex ? "> " : "  ") << items[i];
        lines.push_back(itemLine.str());
    }
    lines.push_back("");
    lines.push_back(footer);

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, title, lines);
    std::cout << out.str();
}

void MapRenderer::drawDialogueFrame(const std::vector<DialogueLine>& dialogueLines) {
    std::vector<std::string> lines;
    for (const auto& line : dialogueLines) {
        std::ostringstream speechLine;
        speechLine << line.speaker << ": " << line.text;
        lines.push_back(speechLine.str());
        lines.push_back("");
    }
    lines.push_back("(press any key to continue)");

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, "Conversation", lines);
    std::cout << out.str();
}

int MapRenderer::drawLogFrame(const std::vector<std::string>& log, int scrollOffset) {
    std::vector<std::string> wrapped;
    for (const std::string& entry : log) {
        for (std::string& line : wrapText(entry, kLogFrameWidth)) wrapped.push_back(std::move(line));
    }
    int total = static_cast<int>(wrapped.size());
    int maxOffset = std::max(0, total - kViewportHeight);
    // Negative means "start at the bottom" (most recent), matching the
    // live panel's own default -- otherwise clamp into the real range so
    // GameLoop::handleLog can freely add/subtract a scroll step without
    // tracking wrapped's size itself.
    int offset = scrollOffset < 0 ? maxOffset : std::clamp(scrollOffset, 0, maxOffset);

    std::vector<std::string> lines;
    for (int row = 0; row < kViewportHeight; ++row) {
        int idx = offset + row;
        std::string text = idx < total ? wrapped[static_cast<size_t>(idx)] : std::string();
        lines.push_back(padPlain(text, kLogFrameWidth));
    }

    lines.push_back(padPlain("", kLogFrameWidth));
    if (total == 0) {
        lines.push_back(padPlain("(nothing logged yet)", kLogFrameWidth));
    } else {
        std::ostringstream status;
        status << "Lines " << (offset + 1) << "-" << std::min(total, offset + kViewportHeight) << " of " << total;
        lines.push_back(padPlain(status.str(), kLogFrameWidth));
    }
    lines.push_back(padPlain("up/down=scroll   v/q=return", kLogFrameWidth));

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBorder(out, "Event Log", kLogFrameWidth, lines);

    std::cout << out.str();
    return offset;
}

} // namespace render
