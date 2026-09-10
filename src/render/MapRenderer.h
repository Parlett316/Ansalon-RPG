#pragma once

#include "character/Equipment.h"
#include "combat/CombatGrid.h"
#include "combat/Monster.h"
#include "game/GameState.h"
#include "world/OverworldGrid.h"
#include "world/Terrain.h"
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
    // The original fixed frame size this project shipped at (Milestone
    // 43). configureLayout uses kPreferredViewportWidth as the map's
    // assumed share when sizing the log panel (see its own comment), but
    // neither constant is a runtime CEILING on the map itself anymore --
    // on a console bigger than this, the map absorbs all the leftover
    // width/height rather than staying pinned here and leaving it blank.
    // Only reachable value at all on a console AT the documented minimum,
    // and still the compile-time default below for any caller that never
    // runs configureLayout (e.g. a throwaway self-test) -- see
    // docs/ARCHITECTURE.md.
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
    // one-line header, a '=' rule, a '-' rule, and a footer line (4 literal
    // rows), plus one spare row of margin (Milestone 150) -- kept as cheap
    // insurance against an off-by-one the same way kChromeColumns already
    // was on the width axis (see its own comment), just missing here until
    // now. Without it, a console whose height configureLayout measures
    // exactly right still gets a frame that fills 100% of the window with
    // no row to spare: the last line's trailing "\n" then asks the cursor
    // to advance past the window's last row, and classic Windows conhost
    // responds by scrolling the whole buffer (and its notion of "home")
    // down by one -- which then compounds every redraw, since every
    // subsequent frame re-triggers the same overflow. kChromeColumns
    // avoids the equivalent auto-wrap trigger on the width axis; this is
    // the missing row-axis counterpart.
    static constexpr int kChromeRows = 5;
    static constexpr int kChromeColumns = 1;
    // Below these, configureLayout refuses to run at all -- see
    // docs/ARCHITECTURE.md and main.cpp's fail-fast check.
    static constexpr int kAbsoluteMinColumns =
        kMinViewportWidth + kLogPanelGap + kMinLogPanelWidth + kChromeColumns; // 72
    static constexpr int kAbsoluteMinRows = kMinViewportHeight + kChromeRows; // 21

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

    // World Map screen (see drawWorldMapFrame below) -- its own adaptive
    // size, independent of kViewportWidth/Height above since it's a
    // separate downsampled view of the whole 480x320 grid, not the
    // real-time walking camera. kWorldMapColumns is always exactly
    // 3 * kWorldMapRows (see drawWorldMapFrame's own comment for why
    // that ratio), both recomputed by configureLayout. The legend column
    // beside the map is a fixed, non-adaptive width -- sized to the
    // longest authored location name ("High Clerist's Tower") -- since
    // its content never changes size, unlike the map itself.
    static constexpr int kWorldMapMinRows = 13;
    static constexpr int kWorldMapPreferredRows = 25; // exactly data/locations.txt's location count, so the
                                                        // legend needs no truncation at this size or above --
                                                        // bump this if a future session adds a 26th location,
                                                        // or the legend will start silently truncating again
                                                        // even on a generously-sized console
    static constexpr int kWorldMapLegendWidth = 24;
    static constexpr int kWorldMapLegendGap = 3; // matches kLogPanelGap's literal " | " divider
    static int kWorldMapRows;
    static int kWorldMapColumns;

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
    // the zone grid, the entry/exit tile marked, and POI glyphs and the
    // player's '@' overlaid. Every authored zone (data/zones/*.txt, at
    // most 44x16) is centered inside the full kViewportWidth/Height
    // canvas rather than drawn at the origin -- if the canvas is bigger
    // than the zone (any terminal above the documented minimum size), a
    // thin border frames it and the real surrounding overworld terrain
    // (sampled from `grid` around the player's own preserved overworld
    // position -- see GameState::x/y's own doc comment) fills the rest as
    // a purely decorative backdrop; `grid` is never consulted for
    // movement/collision, which still checks only Zone::tileCodeAt/poiAt
    // in zone-local coordinates exactly as before this milestone. At the
    // absolute minimum terminal size the inset is exactly 0 and this
    // renders identically to the old wall-padded behavior it replaces.
    // "Standing On" is always the zone's own name (Zone::name()); the
    // status panel's mode label reads "INDOORS". As of Milestone 30, a
    // POI's name/description and any TIMELINE_ANCHOR presence are pushed
    // to `log` once on arrival by GameLoop::announceZoneTile rather than
    // redrawn here every frame -- see docs/ARCHITECTURE.md.
    static void drawZoneFrame(const world::OverworldGrid& grid, const world::Zone& zone,
                               const game::GameState& state, const std::vector<std::string>& log);

    // Renders the full character sheet as its own frame. GameLoop shows
    // this on demand ('c'), blocks for one keypress to dismiss it, then
    // resumes normal rendering -- see docs/ARCHITECTURE.md. `companions`
    // (Milestone 116 Phase 1's one slot; a real roster as of Milestone 118)
    // appends a short, terse "Companions:" block, one entry per recruited
    // companion (identity + HP/AC/THAC0 only) -- see docs/COMBAT_NOTES.md's
    // "Extending this later".
    static void drawCharacterSheet(const character::Character& character,
                                    const std::vector<character::Character>& companions = {});

    // Renders the full spell roster for the character's class
    // (character::spellListFor), grouped by level up to
    // character::maxAccessibleSpellLevel -- the detail drawCharacterSheet's
    // own terse "Spells memorized: ..." line leaves out. Each spell still
    // memorized-and-uncast is marked. Reached from the character sheet
    // ('s', only offered to a caster -- see game::GameLoop::
    // showCharacterSheet/showSpellbook). Same one-keypress-blocks shape as
    // drawCharacterSheet.
    static void drawSpellbookFrame(const character::Character& character);

    // The tactical combat grid's fixed size (Milestone 114, bumped at
    // Milestone 129 -- see docs/COMBAT_NOTES.md's "Positional combat grid"
    // section) -- small enough to read comfortably inside the existing
    // "organic" combat box (each cell renders 3 columns wide, " X "/"[X]",
    // so a 15-wide grid is 47 visible columns including its own border --
    // well under kProseWrapWidth's floor of 71 even on the smallest
    // supported console, so a grid row can never wrap), big enough for a
    // roomier Gold-Box-style battlefield than the original 11x7. Width
    // MUST stay odd -- GameLoop::runCombat centers the player and spreads
    // monster instances symmetrically around kCombatGridWidth / 2.
    // GameLoop::runCombat reads these same constants for placement/
    // bounds-checking -- no duplicated numbers.
    static constexpr int kCombatGridWidth = 15;
    static constexpr int kCombatGridHeight = 9;

    // One monster instance's presentation state, built by GameLoop::runCombat
    // from its own std::vector<MonsterInstance> -- MapRenderer never sees
    // combat::Monster's other fields (special-ability flags, terrain data,
    // etc.), same "presentation stays ignorant of the domain type" pattern
    // JournalEntry/DialogueLine already establish. `name` already carries
    // the letter suffix ("Goblin A") when the encounter is a group of more
    // than one -- a solo encounter's `name` is just the plain monster name,
    // identical to every combat screen before Milestone 113. `pos` is new
    // as of Milestone 114's positional grid.
    struct CombatMonsterView {
        std::string name;
        int hp = 0;
        int maxHp = 0;
        int armorClass = 0;
        bool alive = true;
        combat::GridPos pos;
        // The single character drawn on the grid at `pos` -- always a
        // letter ('A' + instance index), even for a solo fight where
        // `name` itself carries no letter suffix (its HP-list line just
        // reads "Goblin"). Kept as its own field rather than derived from
        // `name` in MapRenderer, which would be ambiguous for that solo
        // case.
        char glyph = '?';
    };

    // Drives drawCombatFrame's in-frame choosers (Milestone 115, see
    // docs/COMBAT_NOTES.md's "In-frame combat actions" section) -- built
    // fresh by GameLoop::runCombat each redraw, never stored. Three
    // states, distinguished by `title`/`options`:
    //   - `title` empty: no chooser is open. The footer becomes the
    //     command row (ATTACK/MOVE/CAST/USE/FLEE) instead of a picker
    //     hint, and `gridCursorIndex` is ignored.
    //   - `title` set, `options` empty: target picking. The grid itself
    //     is the picker -- `gridCursorIndex` (an index into the
    //     `monsters` vector passed to drawCombatFrame) draws that
    //     instance's cell as "[X]" instead of " X ", and its HP-roster
    //     line gets the usual "> " cursor prefix. Enter/up/down are
    //     handled by the caller (GameLoop::runCombat's pickTarget), same
    //     as before -- this struct only carries what to draw.
    //   - `title` set, `options` non-empty: an in-frame list chooser
    //     (spell or item selection), which has no grid representation --
    //     `options` render as their own cursor list under the grid, same
    //     visual shape drawPickerFrame used before this milestone,
    //     `selected` marks the cursor row.
    struct CombatPrompt {
        std::string title;
        std::vector<std::string> options;
        int selected = 0;
        int gridCursorIndex = -1;
        std::string footer;
    };

    // Renders one combat frame: a small bordered tactical grid (Milestone
    // 114 -- empty cells are a uniform '.' and `floorTerrain`'s `name` is
    // printed once as a "Battlefield: forest" label above the grid rather
    // than its glyph being tiled across every cell (Milestone 120), still
    // matching DQoK.pdf's own manual describing the combat map as "a
    // detailed view of the terrain the party was in"; `playerPos` and each
    // alive monster's `CombatMonsterView::pos` place the `@`/lettered
    // glyphs, and each alive entry in `companions` -- Milestone 117/118 --
    // its own glyph), then the player's HP/AC, one HP/AC line per companion
    // ("(knocked out)" once its HP reaches 0, same convention as a
    // defeated monster below), every monster instance's HP/AC (defeated
    // ones marked, still shown so the roster visibly shrinks rather than
    // vanishing), a scrolling combat log (most recent entries last -- only
    // the tail that fits is shown), and the available actions. Monster HP
    // is tracked by GameLoop::runCombat, not the combat::Monster struct
    // itself (which is static content shared by every encounter with that
    // monster type) -- see docs/COMBAT_NOTES.md. `currentDay`
    // (hoursElapsed/24, same convention as drawCharacterSheet above) is
    // used both to decide the command row's "USE: ..." hint
    // (character::availableCombatItems) and, as of Milestone 115, is what
    // an idle `prompt` (default-constructed CombatPrompt) renders instead
    // of the old fixed footer hints -- see CombatPrompt above for the
    // chooser states GameLoop::runCombat can request instead. `companions`
    // reuses CombatMonsterView purely as a presentation-state carrier (not
    // because a companion IS a monster) -- built fresh each redraw by
    // GameLoop::runCombat's own companionViews() helper from
    // GameState::companions; never a target for the player's own
    // pickTarget, which only ever indexes into `monsters`.
    static void drawCombatFrame(const character::Character& character,
                                 const std::vector<CombatMonsterView>& monsters,
                                 const std::vector<std::string>& log, long long currentDay,
                                 const world::TerrainInfo& floorTerrain, combat::GridPos playerPos,
                                 const CombatPrompt& prompt = {},
                                 const std::vector<CombatMonsterView>& companions = {});

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

    // Draws the "ask about something else..." free-text prompt: a boxed
    // hint line listing `hints` (the askable subjects' display labels, so
    // the player isn't guessing blind -- see docs/TIMELINE_NOTES.md's "Ask
    // about anything"), an instruction line naming `npcName`, then a plain
    // (unboxed) "> " prompt printed immediately below with no trailing
    // newline, so the cursor is left right after it --
    // render::Console::readLine's raw per-keystroke echo prints directly
    // there. See game::GameLoop::talkTo.
    static void drawAskInputFrame(const std::string& npcName, const std::vector<std::string>& hints);

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

    // Renders the read-only "World Map" overview screen ('o'): the whole
    // 480x320 overworld grid downsampled to kWorldMapColumns x
    // kWorldMapRows (box-majority-vote terrain sampling, same
    // world::terrainFor colors drawOverworldFrame uses), every
    // world::Location overlaid as its glyph (a footprint of 1/2/2x2 cells
    // depending on world::MapSize -- see docs/MAP_NOTES.md), the
    // player's own position overlaid last as '@' so it's never hidden by
    // an overlapping location marker, and a side legend panel (glyph +
    // name, alphabetical) listing every location -- the map itself never
    // draws text, since roughly half the location set sits too close
    // together at this resolution for inline labels to stay legible (see
    // docs/MAP_NOTES.md). Not a live/scrolling view -- GameLoop::
    // showWorldMap draws this once and blocks for a single keypress to
    // dismiss it, same shape as drawHelpFrame/drawSpellbookFrame; no
    // travel/fast-travel action is offered here, this is reference-only.
    static void drawWorldMapFrame(const world::OverworldGrid& grid, const world::World& world,
                                   const game::GameState& state);

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
