#pragma once

#include "character/Equipment.h"
#include "combat/Monster.h"
#include "game/GameState.h"
#include "world/OverworldGrid.h"
#include "world/World.h"
#include "world/Zone.h"

#include <string>
#include <vector>

namespace render {

// Turns world data into a player-facing ASCII frame. Pure presentation:
// only ever reads its inputs and prints to stdout, never mutates anything.
class MapRenderer {
public:
    // Zones are authored to fit within kMinViewportWidth/Height -- these
    // are the true, fixed floor (the widest/tallest currently-authored
    // zone, solace_inn.txt, is 44x16 -- see docs/ZONE_NOTES.md), not the
    // adaptive kViewportWidth/Height below. A zone must fit this floor,
    // not whatever a given run's larger/preferred layout happens to be,
    // or it could clip on someone else's smaller terminal.
    static constexpr int kMinViewportWidth = 44;
    static constexpr int kMinViewportHeight = 16;
    // Used by configureLayout when there's enough room; sized down toward
    // the minimums above for a smaller console -- see docs/ARCHITECTURE.md.
    static constexpr int kPreferredViewportWidth = 78;
    static constexpr int kPreferredViewportHeight = 30;
    // Must fit not just wrapped log prose but real fixed status-panel
    // content -- the longest authored location/zone names ("High Clerist's
    // Tower", "Inn of the Last Home") are 21-23 columns, so this floor sits
    // just above that rather than being tuned for prose-wrap alone.
    static constexpr int kMinLogPanelWidth = 24;
    static constexpr int kMaxLogPanelWidth = 60;
    // Width of the literal " | " divider drawn between the map and the
    // status panel (Milestone 43) -- not a bare gap anymore.
    static constexpr int kLogPanelGap = 3;
    // Non-map/log "chrome" every frame always has (Milestone 43): a
    // one-line header, a '=' rule, a '-' rule, and a footer line (rows);
    // one column of margin, kept as cheap insurance against an off-by-one
    // in the detected terminal width rather than filling it exactly zero
    // (columns).
    static constexpr int kChromeRows = 4;
    static constexpr int kChromeColumns = 1;
    // Below these, configureLayout refuses to run at all -- see
    // docs/ARCHITECTURE.md and main.cpp's fail-fast check.
    static constexpr int kAbsoluteMinColumns =
        kMinViewportWidth + kLogPanelGap + kMinLogPanelWidth + kChromeColumns; // 72
    static constexpr int kAbsoluteMinRows = kMinViewportHeight + kChromeRows; // 20

    // The ACTIVE layout in use this run. Default-initialized (in
    // MapRenderer.cpp) to the preferred values above, so anything that
    // never calls configureLayout (a throwaway self-test, say) behaves
    // exactly like a fixed-size build. main.cpp calls configureLayout
    // once at startup with the real queried console size -- see
    // Console::currentWindowSize and docs/ARCHITECTURE.md. Caves of
    // Qud-style scrolling event log, rendered as a fixed-width column to
    // the right of the map in both drawOverworldFrame and drawZoneFrame;
    // this is only ever a tail view (the most recent kViewportHeight
    // wrapped lines) -- drawLogFrame below is the scrollable full-history
    // counterpart, added in Milestone 31 because the tail view alone gave
    // no way to read anything that had scrolled off.
    static int kViewportWidth;
    static int kViewportHeight;
    static int kLogPanelWidth;
    // Wrap width for the dedicated full-screen log-history pager
    // (drawLogFrame) -- wider than kLogPanelWidth since that screen has
    // the whole frame to itself rather than sharing it with the map.
    static int kLogFrameWidth;

    // Recomputes kViewportWidth/Height/kLogPanelWidth/kLogFrameWidth (and
    // the prose-wrap/organic-box-width internals in MapRenderer.cpp) to
    // fit a console of `columns` x `rows`: map width defaults to
    // kPreferredViewportWidth but shrinks toward kMinViewportWidth only
    // if needed to guarantee the log panel at least kMinLogPanelWidth;
    // the log panel takes whatever's left, clamped to
    // [kMinLogPanelWidth, kMaxLogPanelWidth]; map height is
    // min(kPreferredViewportHeight, available). Returns false (leaving
    // the layout unchanged) if columns/rows are below
    // kAbsoluteMinColumns/Rows -- the caller (main.cpp) should print a
    // clear error and exit rather than attempting to render. See
    // docs/ARCHITECTURE.md.
    static bool configureLayout(int columns, int rows);

    // Renders one full overworld frame (Milestone 43 layout: no outer box
    // border): a one-line header (title, name/class, HP bar, day/hour), a
    // '=' rule, then side by side a scrolling colored viewport of the
    // overworld grid centered on the player (clamped at map edges) with
    // location glyphs and the player's '@' overlaid, a " | " divider, and
    // a status panel (MODE/Standing On/Position/AC/THAC0/Steel, then a
    // scrolling "> "-prefixed event log tail -- see kLogPanelWidth above),
    // a '-' rule, and the footer control hint. `log` is GameLoop's
    // persistent event history -- including, as of Milestone 30, the
    // "standing here" Location description/timeline-presence text
    // GameLoop pushes once on arrival (GameLoop::announceOverworldTile)
    // rather than this function recomputing and redrawing it every frame
    // -- oldest entries scroll off the top of the panel once it fills.
    // See docs/ARCHITECTURE.md.
    static void drawOverworldFrame(const world::OverworldGrid& grid, const world::World& world,
                                    const game::GameState& state, const std::vector<std::string>& log);

    // Renders one full zone (interior) frame: the same Milestone 43
    // header/rule/status-panel layout as drawOverworldFrame (see above),
    // the whole zone grid (wall-padded to kViewportWidth/Height if smaller
    // -- see above), the entry/exit tile marked, and POI glyphs and the
    // player's '@' overlaid. "Standing On" is always the zone's own name
    // (Zone::name()); the status panel's mode label reads "INDOORS". As of
    // Milestone 30, a POI's name/description and any TIMELINE_ANCHOR
    // presence are pushed to `log` once on arrival by
    // GameLoop::announceZoneTile rather than redrawn here every frame --
    // see docs/ARCHITECTURE.md.
    static void drawZoneFrame(const world::Zone& zone, const game::GameState& state,
                               const std::vector<std::string>& log);

    // Renders the full character sheet as its own frame. GameLoop shows
    // this on demand ('c'), blocks for one keypress to dismiss it, then
    // resumes normal rendering -- see docs/ARCHITECTURE.md. `currentDay`
    // (hoursElapsed/24) is only used to display whether today's spells
    // have been memorized yet (character::Character::spellsCastDay) and,
    // if so, how many slots remain -- see character::memorizeSpells.
    static void drawCharacterSheet(const character::Character& character, long long currentDay);

    // Renders one combat frame: both combatants' HP/AC, a scrolling combat
    // log (most recent entries last -- only the tail that fits is shown),
    // and the available actions. `monsterHp` is tracked by
    // GameLoop::runCombat, not the Monster struct itself (which is static
    // content shared by every encounter with that monster type) -- see
    // docs/COMBAT_NOTES.md. `currentDay` (hoursElapsed/24, same convention
    // as drawCharacterSheet above) is only used to decide whether the
    // footer hints "i=use brooch" -- see character::broochAvailableToday.
    static void drawCombatFrame(const character::Character& character, const combat::Monster& monster,
                                 int monsterHp, int monsterMaxHp, const std::vector<std::string>& log,
                                 long long currentDay);

    struct DialogueLine {
        std::string speaker;
        std::string text;
    };

    // Renders a one-shot "conversation" frame -- one speaker/line pair per
    // entry. GameLoop::talkTo always passes exactly one now (when several
    // characters are present at once, GameLoop::handleTalk asks which one
    // via drawTalkToWhomFrame below first). Same block-until-a-keypress
    // shape as drawCharacterSheet.
    static void drawDialogueFrame(const std::vector<DialogueLine>& lines);

    // A generic cursor-list picker: `title` as a heading, one `items`
    // entry per line with `selectedIndex` marked by a '>' cursor, `footer`
    // as the controls hint. Used for both "Talk to whom?" (when more than
    // one talkable character is present at once) and topic menus ("Ask
    // about...") -- see game::GameLoop::handleTalk/talkTo. Same
    // nested-loop, does-not-block-itself shape as drawShopFrame
    // (North/South move the cursor, Enter picks, Quit cancels).
    static void drawPickerFrame(const std::string& title, const std::vector<std::string>& items,
                                 int selectedIndex, const std::string& footer);

    // Renders the shop screen: `shopName` (the POI's own name, e.g.
    // "General Store" or "Market Stalls") as the heading, the character's
    // steel, and either `buyItems` (character::availableShopItems) or
    // `sellItems` (character::sellableItems) depending on `sellMode`, with
    // `selectedIndex` marked by a '>' cursor, and `message` (the result of
    // the last buy/sell attempt, or empty). GameLoop::handleShop() takes
    // over input itself while this is showing (North/South move the
    // cursor, Enter buys/sells, 'i' toggles buy/sell view, Quit exits) --
    // same nested-loop shape as drawCombatFrame -- so unlike
    // drawCharacterSheet/drawDialogueFrame this does not block for a
    // keypress itself.
    static void drawShopFrame(const character::Character& character, const std::string& shopName,
                               const std::vector<character::ShopItem>& buyItems,
                               const std::vector<character::SellItem>& sellItems, bool sellMode,
                               int selectedIndex, const std::string& message);

    // Renders the carried-items screen: current HP, then the three equipped
    // slots (Armor/Shield/Weapon) at top, then a cursor list of
    // character.inventory (label format matches ShopItem::label) with
    // `selectedIndex` marked. GameLoop::handleInventory() takes over input
    // itself while this is showing -- same nested-loop shape as
    // drawShopFrame (North/South move the cursor, Enter equips a
    // gear item or drinks a Potion, Quit leaves).
    static void drawInventoryFrame(const character::Character& character, int selectedIndex);

    // Renders a dedicated, full-screen scrollable view of the *entire*
    // log_ history -- the live side panel (see kLogPanelWidth above) only
    // ever shows a tail view with no way to read anything that's scrolled
    // off; this is that missing scrollback. Wraps every raw `log` entry to
    // kLogFrameWidth and shows a kViewportHeight-tall window of the result
    // starting at `scrollOffset` (a negative value means "start at the
    // bottom," i.e. the most recent entries, matching the live panel's own
    // convention). `scrollOffset` is clamped internally to the real valid
    // range; the clamped value actually used is returned so
    // GameLoop::handleLog's nested loop can keep adjusting it by a fixed
    // step across calls without duplicating the wrap/line-count math here.
    static int drawLogFrame(const std::vector<std::string>& log, int scrollOffset);

    // One quest's journal entry, adapted by GameLoop::showJournal from
    // quest::Quest + game::GameState::quests/monsterKills -- MapRenderer
    // never learns about quest:: itself (same decoupling as DialogueLine),
    // and never decides what's complete. `objectiveLines` are pre-marked by
    // the caller ("[x] ..." / "[ ] ... (2/3)").
    struct JournalEntry {
        std::string title;
        bool complete = false;
        std::vector<std::string> objectiveLines;
    };

    // Renders the quest journal ('g'): active quests first, then a
    // "Completed" section, or "(no quests yet)" if `entries` is empty.
    // GameLoop::showJournal() blocks for one keypress to dismiss it, same
    // shape as drawCharacterSheet/drawHelpFrame -- no scrolling in v1 (see
    // docs/QUEST_NOTES.md), same known limit as drawInventoryFrame today.
    static void drawJournalFrame(const std::vector<JournalEntry>& entries);

    // Renders the '?' help screen: every command bound in
    // render::Console::readKey, grouped by context (movement, overworld/
    // zone actions, combat-only actions, other). Static content -- no
    // parameters -- since the binding list itself doesn't depend on game
    // state. GameLoop::showHelp() blocks for one keypress to dismiss it,
    // same shape as drawCharacterSheet.
    static void drawHelpFrame();
};

} // namespace render
