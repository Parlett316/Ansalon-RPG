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

// Colors a plain-text line without disturbing its visible width: pads to
// `width` first, then wraps the (now already-correct-width) text in an
// ANSI set/reset pair. Same "already exactly `width` visible columns by
// construction, never re-padded afterward" contract the map-row glyphs
// follow -- see padPlain's own comment above for why that matters.
std::string colorLine(const std::string& text, const char* ansiColor, int width) {
    std::string padded = padPlain(text, width);
    return std::string(ansiColor) + padded + "\x1b[0m";
}

// Wraps every raw log entry to `width - 2` (reserving 2 columns for a
// "> "/"  " prefix -- see below), takes the tail `height` physical lines
// (so once the panel fills, the oldest lines scroll off the top -- most
// recent always visible at the bottom, same convention as
// drawCombatFrame's own log), right-pads each to `width`, and pads the
// *end* with blank lines while there isn't yet `height` worth of content
// -- so early in a session the log anchors at the top and grows
// downward, matching the Caves of Qud reference this milestone is
// modeled on (see docs/ARCHITECTURE.md). As of Milestone 43, the first
// physical line of each entry is prefixed "> " and any wrapped
// continuation lines "  ", matching the reference layout's action-log
// convention -- except a literal empty entry (the blank-spacer lines
// GameLoop::pushLog("") pushes between arrival blocks), which stays a
// plain blank line with no prefix.
std::vector<std::string> buildLogPanel(const std::vector<std::string>& log, int width, int height) {
    std::vector<std::string> wrapped;
    for (const std::string& entry : log) {
        if (entry.empty()) {
            wrapped.push_back("");
            continue;
        }
        int contentWidth = std::max(1, width - 2);
        std::vector<std::string> entryLines = wrapText(entry, contentWidth);
        for (size_t i = 0; i < entryLines.size(); ++i) {
            wrapped.push_back((i == 0 ? "> " : "  ") + entryLines[i]);
        }
    }
    size_t start = wrapped.size() > static_cast<size_t>(height) ? wrapped.size() - static_cast<size_t>(height) : 0;
    std::vector<std::string> panel(wrapped.begin() + static_cast<long>(start), wrapped.end());
    for (std::string& line : panel) {
        if (static_cast<int>(line.size()) < width) line.append(static_cast<size_t>(width) - line.size(), ' ');
    }
    while (static_cast<int>(panel.size()) < height) panel.push_back(std::string(static_cast<size_t>(width), ' '));
    return panel;
}

// Builds the one-line header shared by drawOverworldFrame/drawZoneFrame
// (Milestone 43): the game's title on the left, and the character's
// name/class/HP/day-hour right-aligned on the same line -- AC/THAC0/Steel
// live in the status panel instead (see buildStatusPanel below), not
// crammed onto this line. Plain text, no embedded ANSI codes.
std::string writeHeaderLine(const character::Character& c, const game::GameState& state, int width) {
    static const std::string kTitle = "Ansalon: Age of Despair";

    std::ostringstream right;
    right << c.name << " (" << character::classInfo(c.charClass).name << ")   HP "
          << hpBar(c.currentHp, c.maxHp, 16) << " " << c.currentHp << "/" << c.maxHp << "   Day "
          << (state.hoursElapsed / 24) << ", Hour " << (state.hoursElapsed % 24);
    std::string rightStr = right.str();

    int gap = width - static_cast<int>(kTitle.size()) - static_cast<int>(rightStr.size());
    return kTitle + std::string(static_cast<size_t>(std::max(gap, 1)), ' ') + rightStr;
}

// Projects game::GameState::companions (a vector of game::RecruitedCompanion,
// pairing a save-format id with a real character::Character) down to just
// the Characters -- the only party the rest of this file (buildStatusPanel,
// and every caller of drawCharacterSheet/drawCombatFrame in GameLoop.cpp)
// actually needs to render. Keeps render:: from having to know
// RecruitedCompanion's id field exists.
std::vector<character::Character> companionCharacters(const game::GameState& state) {
    std::vector<character::Character> result;
    for (const game::RecruitedCompanion& companion : state.companions) result.push_back(companion.character);
    return result;
}

// Builds the status panel shown to the right of the map in
// drawOverworldFrame/drawZoneFrame (Milestone 43): a fixed 12-row header
// (mode, current location/zone, coordinates, and the AC/THAC0/Steel
// figures moved out of the old 2-line HUD -- see writeHeaderLine above),
// then the scrolling "> "-prefixed log tail (buildLogPanel) filling
// whatever rows remain. Every returned line is already exactly `width`
// visible columns, safe to merge with a map row with no further padding.
// `companions` (Milestone 116 Phase 1; a real roster as of Milestone 118)
// is empty when no companion has been recruited -- adds one "Companion:"
// line per entry to the header when present; the header grows/shrinks
// around them via headerRows below, same as every other conditional-less
// line here.
std::vector<std::string> buildStatusPanel(const char* modeLabel, const std::string& standingOnName, int posX,
                                           int posY, const character::Character& c,
                                           const std::vector<character::Character>& companions,
                                           const std::vector<std::string>& log, int width, int height) {
    std::vector<std::string> lines;
    lines.push_back(colorLine(std::string("MODE: ") + modeLabel, "\x1b[96m", width));
    lines.push_back(padPlain("", width));
    lines.push_back(padPlain("Standing On:", width));
    lines.push_back(colorLine(standingOnName, "\x1b[93m", width));
    lines.push_back(padPlain("", width));
    std::ostringstream pos;
    pos << "Position: (" << posX << ", " << posY << ")";
    lines.push_back(padPlain(pos.str(), width));
    lines.push_back(padPlain("", width));
    std::ostringstream stats1;
    stats1 << "AC " << c.armorClass << "   THAC0 " << c.thac0;
    lines.push_back(padPlain(stats1.str(), width));
    std::ostringstream stats2;
    stats2 << "Steel: " << c.steelPieces << " stl";
    lines.push_back(padPlain(stats2.str(), width));
    for (const character::Character& companion : companions) {
        std::ostringstream companionLine;
        companionLine << "Companion: " << companion.name << "  HP " << companion.currentHp << "/"
                       << companion.maxHp;
        lines.push_back(padPlain(companionLine.str(), width));
    }
    lines.push_back(padPlain("", width));
    lines.push_back(padPlain("ACTION LOG:", width));
    lines.push_back(padPlain("", width));

    int headerRows = static_cast<int>(lines.size());
    std::vector<std::string> logTail = buildLogPanel(log, width, std::max(0, height - headerRows));
    for (std::string& line : logTail) lines.push_back(std::move(line));

    // Defensive: near MapRenderer::kAbsoluteMinRows the fixed header alone
    // could reach or exceed `height` -- clamp/pad so this always returns
    // exactly `height` rows, matching what the row-by-row merge in
    // drawOverworldFrame/drawZoneFrame assumes.
    if (static_cast<int>(lines.size()) > height) lines.resize(static_cast<size_t>(height));
    while (static_cast<int>(lines.size()) < height) lines.push_back(padPlain("", width));
    return lines;
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

// Colors reused by the "talking to an NPC" and combat screens below,
// extending the same bright-ANSI palette drawOverworldFrame/
// buildStatusPanel already use for the player (bright white, the '@'
// glyph) and for named things in the world (bright yellow, "Standing On"
// and location/POI glyphs) -- see docs/GOTCHAS.md on why every set code
// needs its paired reset, handled by colorLine below.
constexpr const char* kNpcNameColor = "\x1b[93m";       // bright yellow, matches "Standing On" / location glyphs
constexpr const char* kSelectedItemColor = "\x1b[97m";  // bright white, matches the player's own '@' glyph
constexpr const char* kPlayerCombatColor = "\x1b[97m";  // bright white, same "this is you" convention
constexpr const char* kMonsterCombatColor = "\x1b[91m"; // bright red -- the threat
// Bright green (Milestone 117) -- deliberately distinct from both the
// player's white and the monsters' red, an "ally" color of its own.
constexpr const char* kCompanionCombatColor = "\x1b[92m";
// Bright cyan, same value buildStatusPanel already uses inline for
// "MODE:" -- named here (Milestone 81) for reuse as the generic "fixed
// section/category label" meaning across the organic screens below
// (Saving Throws:, Level N:, -- Buying/Selling --, Carried items:,
// Completed:, and the Help screen's category headers).
constexpr const char* kSectionLabelColor = "\x1b[96m";

// A line of "organic" screen content (see writeBoxed above) that also
// carries an optional whole-line ANSI color, applied after wrapping and
// padding -- same "pad first, colorize the already-correct-width result"
// contract as colorLine, so a colored line's visible width still matches
// its plain neighbors exactly. nullptr means plain, uncolored text (the
// vast majority of any screen's lines -- footers, blank spacers, prose
// body text). Lets the dialogue/picker/combat screens below pick up the
// palette above without touching the plain writeBoxed(vector<string>)
// callers above (character sheet, spellbook, shop, inventory, journal,
// help).
struct BoxLine {
    std::string text;
    const char* color = nullptr;
};

// Same wrapping as the plain wrapLongLines above, but a wrapped line's
// color tag carries forward to every physical line it produces.
std::vector<BoxLine> wrapLongLines(const std::vector<BoxLine>& lines) {
    std::vector<BoxLine> result;
    for (const BoxLine& line : lines) {
        if (static_cast<int>(line.text.size()) > kProseWrapWidth) {
            for (std::string& wrapped : wrapText(line.text, kProseWrapWidth)) {
                result.push_back({std::move(wrapped), line.color});
            }
        } else {
            result.push_back(line);
        }
    }
    return result;
}

// Same box-hugging shape as the plain writeBoxed above, but colorLine's
// pad-then-wrap treatment is applied to any line carrying a color tag
// instead of a bare padPlain.
void writeBoxed(std::ostringstream& out, const std::string& title, const std::vector<BoxLine>& rawLines) {
    std::vector<BoxLine> wrapped = wrapLongLines(rawLines);
    int width = static_cast<int>(title.size()) + 2;
    for (const BoxLine& line : wrapped) width = std::max(width, static_cast<int>(line.text.size()));
    width = std::clamp(width, kSecondaryBoxMinWidth, kSecondaryBoxMaxWidth);

    std::vector<std::string> exact;
    exact.reserve(wrapped.size());
    for (const BoxLine& line : wrapped) {
        exact.push_back(line.color != nullptr ? colorLine(line.text, line.color, width) : padPlain(line.text, width));
    }

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

    const world::Location* here = world.locationAt(state.x, state.y);
    std::string standingOn =
        here != nullptr ? here->name : std::string(world::terrainFor(grid.terrainCodeAt(state.x, state.y)).name);
    std::vector<std::string> statusPanel =
        buildStatusPanel("EXPLORING", standingOn, state.x, state.y, state.character, companionCharacters(state), log,
                          kLogPanelWidth, kViewportHeight);

    std::ostringstream out;
    out << "\x1b[2J\x1b[H"; // clear + cursor home (see Console::clearScreen -- same VT100 sequence)
    out << padPlain(writeHeaderLine(state.character, state, kContentWidth), kContentWidth) << "\n";
    out << std::string(static_cast<size_t>(kContentWidth), '=') << "\n";

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
        rowStream << " | " << statusPanel[static_cast<size_t>(row)];
        // Not padded via padPlain -- already exactly kContentWidth visible
        // columns by construction (kViewportWidth glyphs + the " | "
        // divider + a pre-padded kLogPanelWidth-wide status panel line).
        out << rowStream.str() << "\n";
    }

    out << std::string(static_cast<size_t>(kContentWidth), '-') << "\n";
    out << padPlain(
        "Move: wasd   l=look around   t=talk   p=shop   v=log   Enter=step in   ?=help   q=quit",
        kContentWidth) << "\n";

    // The whole frame is built as one string and written in a single
    // flush -- this matters far more here than in Milestone 1, since a
    // redraw now happens on every single keystroke instead of every typed
    // command. Many small writes would flicker visibly. See docs/GOTCHAS.md.
    std::cout << out.str();
}

void MapRenderer::drawZoneFrame(const world::Zone& zone, const game::GameState& state,
                                 const std::vector<std::string>& log) {
    const int kContentWidth = kViewportWidth + kLogPanelGap + kLogPanelWidth;

    std::vector<std::string> statusPanel =
        buildStatusPanel("INDOORS", zone.name(), state.zoneX, state.zoneY, state.character, companionCharacters(state),
                          log, kLogPanelWidth, kViewportHeight);

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    out << padPlain(writeHeaderLine(state.character, state, kContentWidth), kContentWidth) << "\n";
    out << std::string(static_cast<size_t>(kContentWidth), '=') << "\n";

    // Always renders the full kViewportWidth/Height, not zone.width()/
    // height() -- a zone smaller than the viewport wall-pads out to fill
    // it (Zone::tileCodeAt/poiAt return '#'/nullptr for any out-of-bounds
    // coordinate, so this is safe -- see docs/GOTCHAS.md), which keeps the
    // status panel's left edge at a stable screen column regardless of
    // which zone is showing.
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
        rowStream << " | " << statusPanel[static_cast<size_t>(y)];
        out << rowStream.str() << "\n";
    }

    out << std::string(static_cast<size_t>(kContentWidth), '-') << "\n";
    out << padPlain(
        "Move: wasd   l=look   t=talk   p=shop   v=log   Enter=leave (from the '>' marker)   ?=help   q=quit",
        kContentWidth) << "\n";

    std::cout << out.str();
}

void MapRenderer::drawCharacterSheet(const character::Character& c, long long currentDay,
                                      const std::vector<character::Character>& companions) {
    const auto& race = character::raceInfo(c.race);
    const auto& cls = character::classInfo(c.charClass);
    const character::SubraceInfo* sub = character::subraceInfo(c.subrace);

    std::vector<BoxLine> lines;

    std::ostringstream classLine;
    classLine << (sub != nullptr ? sub->name : race.name) << " " << cls.name << ", level " << c.level << " ("
              << c.experience << " XP)";
    lines.push_back({classLine.str(), nullptr});
    lines.push_back({character::alignmentName(c.alignment), nullptr});
    if (c.knightOrder != character::KnightOrder::None) {
        lines.push_back({character::knightOrderName(c.knightOrder), nullptr});
    }
    if (c.charClass == character::ClassId::Mage) {
        if (c.robeColor == character::RobeColor::None) {
            lines.push_back({"Unaffiliated student of the arcane", nullptr});
        } else {
            std::ostringstream robeLine;
            robeLine << character::robeColorName(c.robeColor) << ", sworn to " << character::robeMoonName(c.robeColor);
            lines.push_back({robeLine.str(), nullptr});
        }
    }
    lines.push_back({"", nullptr});

    std::ostringstream abilities1;
    abilities1 << "STR " << c.scores.strength;
    if (c.exceptionalStrengthPercentile > 0) {
        // 18/01-18/99 zero-pad to two digits; 18/00 is the top bracket,
        // conventionally written "00" rather than "100".
        int pct = c.exceptionalStrengthPercentile;
        abilities1 << "/" << (pct == 100 ? "00" : (pct < 10 ? "0" : "")) << (pct == 100 ? "" : std::to_string(pct));
    }
    abilities1 << "   DEX " << c.scores.dexterity << "   CON " << c.scores.constitution;
    lines.push_back({abilities1.str(), nullptr});

    std::ostringstream abilities2;
    abilities2 << "INT " << c.scores.intelligence << "   WIS " << c.scores.wisdom << "   CHA " << c.scores.charisma;
    lines.push_back({abilities2.str(), nullptr});
    lines.push_back({"", nullptr});

    std::ostringstream hpLine;
    hpLine << "HP " << c.currentHp << "/" << c.maxHp << "   AC " << c.armorClass << "   THAC0 " << c.thac0;
    lines.push_back({hpLine.str(), nullptr});

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
    lines.push_back({weaponLine.str(), nullptr});
    lines.push_back({"", nullptr});

    lines.push_back({"Saving Throws:", kSectionLabelColor});
    for (int i = 0; i < static_cast<int>(character::SaveCategory::Count); ++i) {
        auto category = static_cast<character::SaveCategory>(i);
        std::ostringstream saveLine;
        saveLine << "  " << character::saveCategoryName(category) << ": " << c.saves.at(category);
        lines.push_back({saveLine.str(), nullptr});
    }
    lines.push_back({"", nullptr});

    std::ostringstream steelLine;
    steelLine << "Steel: " << c.steelPieces << " stl";
    lines.push_back({steelLine.str(), nullptr});

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
    lines.push_back({carriedLine.str(), nullptr});

    if (character::canCastSpells(c.charClass)) {
        lines.push_back({"", nullptr});
        if (character::maxAccessibleSpellLevel(c) == 0) {
            lines.push_back({"Spells: cannot cast arcane magic", nullptr});
        } else if (c.spellsCastDay != currentDay) {
            // Not memorized today -- see character::memorizeSpells /
            // game::GameLoop::handleRest ('r').
            lines.push_back({"Spells: not memorized today -- rest to prepare", nullptr});
        } else if (c.memorizedSpellIds.empty()) {
            lines.push_back({"Spells: none remaining today -- rest to re-prepare", nullptr});
        } else {
            std::ostringstream spellLine;
            spellLine << "Spells memorized: ";
            std::vector<std::string> distinctIds;
            for (const auto& id : c.memorizedSpellIds) {
                if (std::find(distinctIds.begin(), distinctIds.end(), id) == distinctIds.end()) {
                    distinctIds.push_back(id);
                }
            }
            for (size_t i = 0; i < distinctIds.size(); ++i) {
                if (i > 0) spellLine << ", ";
                const character::SpellInfo* spell = character::findSpell(c.charClass, distinctIds[i]);
                int count = static_cast<int>(
                    std::count(c.memorizedSpellIds.begin(), c.memorizedSpellIds.end(), distinctIds[i]));
                spellLine << (spell != nullptr ? spell->name : distinctIds[i]);
                if (count > 1) spellLine << " (x" << count << ")";
            }
            lines.push_back({spellLine.str(), nullptr});
        }
    }

    // Recruitable party companions -- Milestone 116 Phase 1 shipped one
    // slot, Milestone 118 a real roster (0+ entries). Deliberately terse
    // per entry (no saves/weapon/steel breakdown): only identity/HP/AC/
    // THAC0 -- see docs/COMBAT_NOTES.md's "Extending this later".
    if (!companions.empty()) {
        lines.push_back({"", nullptr});
        lines.push_back({"Companions:", kSectionLabelColor});
        for (const character::Character& companion : companions) {
            const auto& compRace = character::raceInfo(companion.race);
            const character::SubraceInfo* compSub = character::subraceInfo(companion.subrace);
            std::ostringstream compLine1;
            compLine1 << "  " << companion.name << ", " << (compSub != nullptr ? compSub->name : compRace.name)
                       << " " << character::classInfo(companion.charClass).name << ", level " << companion.level;
            lines.push_back({compLine1.str(), nullptr});
            std::ostringstream compLine2;
            compLine2 << "  HP " << companion.currentHp << "/" << companion.maxHp << "   AC "
                       << companion.armorClass << "   THAC0 " << companion.thac0;
            lines.push_back({compLine2.str(), nullptr});
        }
    }

    lines.push_back({"", nullptr});
    if (character::canCastSpells(c.charClass)) {
        lines.push_back({"(s=view spells known, any other key to continue)", nullptr});
    } else {
        lines.push_back({"(press any key to continue)", nullptr});
    }

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, c.name, lines);
    std::cout << out.str();
}

void MapRenderer::drawSpellbookFrame(const character::Character& c, long long currentDay) {
    std::vector<BoxLine> lines;

    int maxLevel = character::maxAccessibleSpellLevel(c);
    if (maxLevel == 0) {
        lines.push_back({"Cannot cast arcane magic.", nullptr});
    } else {
        bool memorizedToday = c.spellsCastDay == currentDay;
        for (int lvl = 1; lvl <= maxLevel; ++lvl) {
            std::vector<const character::SpellInfo*> atLevel;
            for (const auto& spell : character::spellListFor(c.charClass)) {
                if (spell.level == lvl) atLevel.push_back(&spell);
            }
            if (atLevel.empty()) continue; // nothing implemented at this level yet -- see docs/CHARACTER_NOTES.md

            std::ostringstream header;
            header << "Level " << lvl << " (" << character::spellSlotsPerDay(c, lvl) << " slot"
                   << (character::spellSlotsPerDay(c, lvl) == 1 ? "" : "s") << "/day):";
            lines.push_back({header.str(), kSectionLabelColor});
            for (const auto* spell : atLevel) {
                std::ostringstream line;
                line << "  " << spell->name;
                if (memorizedToday) {
                    int count = static_cast<int>(
                        std::count(c.memorizedSpellIds.begin(), c.memorizedSpellIds.end(), spell->id));
                    if (count > 0) {
                        line << " (memorized";
                        if (count > 1) line << " x" << count;
                        line << ")";
                    }
                }
                lines.push_back({line.str(), nullptr});
            }
        }
    }

    lines.push_back({"", nullptr});
    lines.push_back({"(press any key to return)", nullptr});

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, "Spells Known", lines);
    std::cout << out.str();
}

void MapRenderer::drawCombatFrame(const character::Character& character,
                                   const std::vector<CombatMonsterView>& monsters,
                                   const std::vector<std::string>& log, long long currentDay,
                                   const world::TerrainInfo& floorTerrain, combat::GridPos playerPos,
                                   const CombatPrompt& prompt, const std::vector<CombatMonsterView>& companions) {
    std::vector<BoxLine> lines;

    // Tactical grid (Milestone 114). Plain text, no per-cell ANSI -- this
    // "organic" screen family's BoxLine only supports one color for an
    // entire line (see writeBoxed/colorLine above), the same restraint
    // Milestone 32 already established for every non-map screen.
    //
    // Milestone 115: each cell is 3 columns wide (" X " / floor, "[X]" for
    // whichever monster prompt.gridCursorIndex names) instead of 1 --
    // that's the in-frame target picker (see CombatPrompt's own comment in
    // MapRenderer.h and docs/COMBAT_NOTES.md's "In-frame combat actions"
    // section): the grid itself is the picker now, so the cursored
    // instance needs to visibly stand out from the rest. kCombatGridWidth
    // cells * 3 columns (33 at the original 11-wide grid, 45 at Milestone
    // 129's 15-wide one) stays well under kProseWrapWidth either way.
    //
    // Milestone 120: the empty floor is a uniform '.' and the terrain it
    // stands for is named once on its own label line, rather than tiling
    // the real tile's world::TerrainInfo glyph across every cell -- a
    // forest fight used to fill the board with '%', which drowned out the
    // '@'/letter glyphs that actually matter. The grid also gets its own
    // ASCII border so the battlefield reads as a bounded map instead of
    // floating text inside the much wider combat box.
    std::ostringstream battlefieldLine;
    battlefieldLine << "Battlefield: " << floorTerrain.name;
    lines.push_back({battlefieldLine.str(), kSectionLabelColor});
    lines.push_back({"", nullptr});

    constexpr char kCombatFloorGlyph = '.';
    const std::string gridBorder = "+" + std::string(static_cast<size_t>(kCombatGridWidth) * 3, '-') + "+";
    lines.push_back({gridBorder, nullptr});
    for (int gy = 0; gy < kCombatGridHeight; ++gy) {
        std::string row = "|";
        row.reserve(static_cast<size_t>(kCombatGridWidth) * 3 + 2);
        for (int gx = 0; gx < kCombatGridWidth; ++gx) {
            int monsterIdx = -1;
            for (size_t i = 0; i < monsters.size(); ++i) {
                if (monsters[i].alive && monsters[i].pos.x == gx && monsters[i].pos.y == gy) {
                    monsterIdx = static_cast<int>(i);
                    break;
                }
            }
            if (monsterIdx >= 0) {
                char glyph = monsters[static_cast<size_t>(monsterIdx)].glyph;
                row += monsterIdx == prompt.gridCursorIndex ? std::string("[") + glyph + "]"
                                                             : std::string(" ") + glyph + " ";
            } else if (playerPos.x == gx && playerPos.y == gy) {
                row += " @ ";
            } else if (const CombatMonsterView* here = [&]() -> const CombatMonsterView* {
                           for (const CombatMonsterView& companion : companions) {
                               if (companion.alive && companion.pos.x == gx && companion.pos.y == gy) return &companion;
                           }
                           return nullptr;
                       }()) {
                // Milestone 117/118 -- a companion's own cell, never a
                // pickTarget candidate (gridCursorIndex only ever names an
                // index into `monsters`), so it never gets the "[X]" bracket
                // treatment, just its plain glyph like the player's own '@'.
                row += std::string(" ") + here->glyph + " ";
            } else {
                row += std::string(" ") + kCombatFloorGlyph + " ";
            }
        }
        row += "|";
        lines.push_back({row, nullptr});
    }
    lines.push_back({gridBorder, nullptr});
    lines.push_back({"", nullptr});

    std::ostringstream playerLine;
    playerLine << character.name << " -- HP " << character.currentHp << "/" << character.maxHp << "   AC "
               << character.armorClass << "   Weapon: " << character.weaponName;
    lines.push_back({playerLine.str(), kPlayerCombatColor});

    // Milestone 117/118: one HP/AC line per companion, right after the
    // player's -- never gets the roster's "> " target-picker cursor (not a
    // pickTarget candidate), same "informational only" treatment as the
    // player's own line above. "(knocked out)" once HP reaches 0, same
    // wording convention as a defeated monster's "(defeated)" below, but
    // distinct -- a knocked-out companion isn't dead, just out of the rest
    // of this fight (see docs/COMBAT_NOTES.md).
    for (const CombatMonsterView& companion : companions) {
        std::ostringstream companionLine;
        companionLine << companion.name << " -- HP " << std::max(0, companion.hp) << "/" << companion.maxHp
                       << "   AC " << companion.armorClass;
        if (!companion.alive) companionLine << " (knocked out)";
        lines.push_back({companionLine.str(), kCompanionCombatColor});
    }

    // Roster lines only grow the "> "/"  " cursor prefix while
    // prompt.gridCursorIndex actually names one of them (target picking) --
    // left off entirely the rest of the time so the ordinary round display
    // (and the spell/item choosers below, which cursor their own separate
    // option list instead) look exactly as they did before Milestone 115.
    bool showRosterCursor = prompt.gridCursorIndex >= 0;
    for (size_t i = 0; i < monsters.size(); ++i) {
        const CombatMonsterView& m = monsters[i];
        std::ostringstream monsterLine;
        if (showRosterCursor) monsterLine << (static_cast<int>(i) == prompt.gridCursorIndex ? "> " : "  ");
        monsterLine << m.name << " -- HP " << std::max(0, m.hp) << "/" << m.maxHp << "   AC " << m.armorClass;
        if (!m.alive) monsterLine << " (defeated)";
        lines.push_back({monsterLine.str(), kMonsterCombatColor});
    }
    lines.push_back({"", nullptr});

    // Only the tail fits comfortably in the viewport -- older lines scroll
    // off, same "most recent last" convention as a chat/console log. Trimmed
    // from 12 (Milestone 73's colored-combat-screen value) to 8 now that a
    // hit/miss entry carries its own bracketed to-hit/damage math (see
    // GameLoop.cpp's describeToHit/describeDamage) and usually wraps to two
    // physical lines instead of one -- keeps the box roughly the same
    // height it was before that addition.
    constexpr size_t kMaxLogLines = 8;
    size_t start = log.size() > kMaxLogLines ? log.size() - kMaxLogLines : 0;
    for (size_t i = start; i < log.size(); ++i) lines.push_back({log[i], nullptr});
    lines.push_back({"", nullptr});

    // Milestone 115: an open chooser (prompt.title set -- spell/item
    // selection, which has no grid representation of its own, or target
    // picking, where prompt.options is left empty since the grid/roster
    // above already show the choices) replaces the old fixed footer hints
    // entirely; an idle frame (prompt.title empty, the default) renders a
    // real command row instead, in the manual's own command-name spirit
    // (CAST/USE) rather than bare "m="/"i=" hints -- see
    // docs/COMBAT_NOTES.md's "In-frame combat actions" section.
    if (!prompt.title.empty()) {
        lines.push_back({prompt.title, kSectionLabelColor});
        for (size_t i = 0; i < prompt.options.size(); ++i) {
            bool isSelected = static_cast<int>(i) == prompt.selected;
            std::ostringstream optionLine;
            optionLine << (isSelected ? "> " : "  ") << prompt.options[i];
            lines.push_back({optionLine.str(), isSelected ? kSelectedItemColor : nullptr});
        }
        lines.push_back({"", nullptr});
        lines.push_back({prompt.footer, nullptr});
    } else {
        std::ostringstream footer;
        footer << "ATTACK (Enter)   MOVE (wasd)";
        // Only hinted when a spell is actually memorized and unspent today --
        // same "only show it when it's usable" precedent USE below follows.
        if (character::hasMemorizedSpellsAvailable(character, currentDay)) {
            footer << "   CAST (m)";
        }
        // Names whichever consumable/item pressing 'i' will actually open --
        // character::availableCombatItems is the same list GameLoop::
        // runCombat's own Inventory-key handling builds, so this can never
        // drift out of sync with what 'i' really does the way the old
        // hand-duplicated priority chain could.
        std::vector<character::CombatItem> items = character::availableCombatItems(character, currentDay);
        if (!items.empty()) footer << "   USE: " << items.front().label << " (i)";
        footer << "   FLEE (f)";
        lines.push_back({footer.str(), nullptr});
    }

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, "Combat", lines);
    std::cout << out.str();
}

void MapRenderer::drawShopFrame(const character::Character& character, const std::string& shopName,
                                 const std::vector<character::ShopItem>& buyItems,
                                 const std::vector<character::SellItem>& sellItems, bool sellMode,
                                 int selectedIndex, const std::string& message) {
    std::vector<BoxLine> lines;

    std::ostringstream steelLine;
    steelLine << "Steel: " << character.steelPieces << " stl";
    lines.push_back({steelLine.str(), nullptr});
    lines.push_back({"", nullptr});

    if (sellMode) {
        lines.push_back({"-- Selling --", kSectionLabelColor});
        for (size_t i = 0; i < sellItems.size(); ++i) {
            const character::SellItem& item = sellItems[i];
            bool isSelected = static_cast<int>(i) == selectedIndex;
            std::ostringstream itemLine;
            itemLine << (isSelected ? "> " : "  ") << item.label << " -- " << item.valueStl << " stl";
            if (!item.sellable) itemLine << "  (cannot sell)";
            lines.push_back({itemLine.str(), isSelected ? kSelectedItemColor : nullptr});
        }
    } else {
        lines.push_back({"-- Buying --", kSectionLabelColor});
        for (size_t i = 0; i < buyItems.size(); ++i) {
            const character::ShopItem& item = buyItems[i];
            bool isSelected = static_cast<int>(i) == selectedIndex;
            std::ostringstream itemLine;
            itemLine << (isSelected ? "> " : "  ") << item.label << " -- " << item.costStl << " stl";
            if (item.alreadyOwned) {
                itemLine << "  (owned)";
            } else if (!item.buyable) {
                itemLine << "  (cannot use)";
            }
            lines.push_back({itemLine.str(), isSelected ? kSelectedItemColor : nullptr});
        }
    }

    if (!message.empty()) {
        lines.push_back({"", nullptr});
        lines.push_back({message, nullptr});
    }
    lines.push_back({"", nullptr});
    std::ostringstream footer;
    footer << "up/down=select   Enter=" << (sellMode ? "sell" : "buy")
           << "   i=" << (sellMode ? "view buy list" : "view sell list") << "   q=leave";
    lines.push_back({footer.str(), nullptr});

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, shopName, lines);
    std::cout << out.str();
}

void MapRenderer::drawInventoryFrame(const character::Character& character, int selectedIndex) {
    std::vector<BoxLine> lines;

    // Shown mainly so drinking a potion here (see GameLoop::handleInventory)
    // is visibly reflected the same way equipping already is via the
    // Weapon:/Armor: lines below -- this screen has no separate message
    // parameter the way drawShopFrame does.
    std::ostringstream hpLine;
    hpLine << "HP: " << character.currentHp << "/" << character.maxHp;
    lines.push_back({hpLine.str(), nullptr});

    std::ostringstream weaponLine;
    weaponLine << "Weapon: " << character.weaponName;
    lines.push_back({weaponLine.str(), nullptr});

    std::ostringstream armorLine;
    armorLine << "Armor: "
              << (character.equippedArmor == character::ArmorId::None ? "none"
                                                                        : character::armorInfo(character.equippedArmor).name)
              << (character.hasShield ? " + Shield" : "");
    lines.push_back({armorLine.str(), nullptr});
    lines.push_back({"", nullptr});

    if (character.inventory.empty()) {
        lines.push_back({"(nothing carried)", nullptr});
    } else {
        lines.push_back({"Carried items:", kSectionLabelColor});
        for (size_t i = 0; i < character.inventory.size(); ++i) {
            bool isSelected = static_cast<int>(i) == selectedIndex;
            std::ostringstream itemLine;
            itemLine << (isSelected ? "> " : "  ") << character::inventoryItemLabel(character.inventory[i]);
            lines.push_back({itemLine.str(), isSelected ? kSelectedItemColor : nullptr});
        }
    }

    lines.push_back({"", nullptr});
    lines.push_back({"up/down=select   Enter=equip/use   q=leave", nullptr});

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, "Inventory", lines);
    std::cout << out.str();
}

void MapRenderer::drawPickerFrame(const std::string& title, const std::vector<std::string>& items,
                                   int selectedIndex, const std::string& footer) {
    // The selected row is colored (bright white, matching the player's own
    // '@' glyph) so the cursor reads clearly at a glance -- every screen
    // that reuses this picker (Talk to whom?, topic menus, Look at whom?,
    // quest Accept/Decline) picks this up for free.
    std::vector<BoxLine> lines;
    for (size_t i = 0; i < items.size(); ++i) {
        bool isSelected = static_cast<int>(i) == selectedIndex;
        std::ostringstream itemLine;
        itemLine << (isSelected ? "> " : "  ") << items[i];
        lines.push_back({itemLine.str(), isSelected ? kSelectedItemColor : nullptr});
    }
    lines.push_back({"", nullptr});
    lines.push_back({footer, nullptr});

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, title, lines);
    std::cout << out.str();
}

void MapRenderer::drawAskInputFrame(const std::string& npcName, const std::vector<std::string>& hints) {
    std::vector<std::string> lines;
    if (!hints.empty()) {
        std::string hintLine = "You could ask about: ";
        for (size_t i = 0; i < hints.size(); ++i) {
            if (i > 0) hintLine += ", ";
            hintLine += hints[i];
        }
        lines.push_back(hintLine);
        lines.push_back("");
    }
    lines.push_back("Type a subject to ask " + npcName + " about, then press Enter.");
    lines.push_back("(Esc cancels.)");

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, "Ask " + npcName + " About...", lines);
    // Deliberately outside the box -- writeBoxed's lines are always padded
    // to exact width and newline-terminated, so there's no clean way to
    // leave the cursor mid-line inside it. A plain prompt below the box
    // keeps this simple and correct instead of fighting absolute
    // ANSI cursor positioning for a cosmetic gain.
    out << "\n> ";
    std::cout << out.str();
    std::cout.flush();
}

void MapRenderer::drawDialogueFrame(const std::vector<DialogueLine>& dialogueLines) {
    // The speaker's name gets its own colored line (bright yellow, matching
    // "Standing On" and location/POI glyphs -- the same "notable named
    // thing" convention) rather than a colored "Name: text" prefix --
    // colorLine only ever colors a whole already-padded line (see BoxLine
    // above); safely tinting just a prefix before wrapText/padPlain have
    // measured the line's real width isn't possible without miscounting
    // invisible escape bytes as visible ones.
    std::vector<BoxLine> lines;
    for (const auto& line : dialogueLines) {
        lines.push_back({line.speaker + ":", kNpcNameColor});
        lines.push_back({line.text, nullptr});
        lines.push_back({"", nullptr});
    }
    lines.push_back({"(press any key to continue)", nullptr});

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

void MapRenderer::drawJournalFrame(const std::vector<JournalEntry>& entries) {
    std::vector<BoxLine> lines;

    if (entries.empty()) {
        lines.push_back({"(no quests yet)", nullptr});
    } else {
        bool anyActive = false;
        for (const auto& entry : entries) {
            if (entry.complete) continue;
            anyActive = true;
            lines.push_back({entry.title, kNpcNameColor});
            for (const auto& objectiveLine : entry.objectiveLines) lines.push_back({"  " + objectiveLine, nullptr});
            lines.push_back({"", nullptr});
        }
        if (!anyActive) lines.push_back({"(no quests active)", nullptr});

        bool anyComplete = false;
        for (const auto& entry : entries) {
            if (!entry.complete) continue;
            if (!anyComplete) {
                lines.push_back({"Completed:", kSectionLabelColor});
                anyComplete = true;
            }
            lines.push_back({entry.title, kNpcNameColor});
            for (const auto& objectiveLine : entry.objectiveLines) lines.push_back({"  " + objectiveLine, nullptr});
            lines.push_back({"", nullptr});
        }
    }

    lines.push_back({"(press any key to continue)", nullptr});

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, "Journal", lines);
    std::cout << out.str();
}

void MapRenderer::drawHelpFrame() {
    std::vector<BoxLine> lines;

    lines.push_back({"Movement:", kSectionLabelColor});
    lines.push_back({"  wasd = move (no diagonals)", nullptr});
    lines.push_back({"", nullptr});
    lines.push_back({"Overworld / zone:", kSectionLabelColor});
    lines.push_back({"  l = look around        t = talk to someone here", nullptr});
    lines.push_back({"  Enter = step in/out     c = character sheet", nullptr});
    lines.push_back({"  p = shop (at a shop)    i = inventory / equip", nullptr});
    lines.push_back({"  v = full event log      g = quest journal", nullptr});
    lines.push_back({"  r = rest                z = bed rest (at a bed)", nullptr});
    lines.push_back({"  ? = this help screen", nullptr});
    lines.push_back({"", nullptr});
    lines.push_back({"Combat:", kSectionLabelColor});
    lines.push_back({"  Enter = attack          m = cast (if a caster)", nullptr});
    lines.push_back({"  i = drink a potion      f = flee", nullptr});
    lines.push_back({"", nullptr});
    lines.push_back({"q / Esc = quit (or leave the current screen)", nullptr});
    lines.push_back({"", nullptr});
    lines.push_back({"(press any key to continue)", nullptr});

    std::ostringstream out;
    out << "\x1b[2J\x1b[H";
    writeBoxed(out, "Help", lines);
    std::cout << out.str();
}

} // namespace render
