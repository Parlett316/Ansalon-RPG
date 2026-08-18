#pragma once

#include "character/Equipment.h"
#include "combat/Monster.h"
#include "game/GameState.h"
#include "timeline/Timeline.h"
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
    static constexpr int kViewportWidth = 78;
    static constexpr int kViewportHeight = 20;
    // Fixed default size, not queried from the actual console -- dynamic
    // resize handling is explicitly deferred (see docs/ARCHITECTURE.md).
    // Assumes a terminal at least this large; README.md documents that
    // assumption. Zones are authored to fit within this size (see
    // docs/ZONE_NOTES.md) so drawZoneFrame needs no camera/scrolling.

    // Renders one full overworld frame: a scrolling colored viewport of the
    // overworld grid centered on the player (clamped at map edges),
    // location glyphs and the player's '@' overlaid, a status line, and
    // (when standing on a Location) any canon characters the timeline
    // places there on the current in-game day -- see
    // docs/TIMELINE_NOTES.md. `message` is an optional transient line shown
    // once, below the status.
    static void drawOverworldFrame(const world::OverworldGrid& grid, const world::World& world,
                                    const timeline::Timeline& timeline, const game::GameState& state,
                                    const std::string& message);

    // Renders one full zone (interior) frame: the whole zone grid (no
    // camera -- see kViewportWidth/Height above), the entry/exit tile
    // marked, POI glyphs and the player's '@' overlaid, and a status line.
    // When standing exactly on the zone's TIMELINE_ANCHOR POI (if it has
    // one), also appends any canon characters the timeline places there
    // today -- the zone-interior counterpart to drawOverworldFrame's own
    // Timeline query, see docs/TIMELINE_NOTES.md.
    static void drawZoneFrame(const world::Zone& zone, const timeline::Timeline& timeline,
                               const game::GameState& state, const std::string& message);

    // Renders the full character sheet as its own frame. GameLoop shows
    // this on demand ('c'), blocks for one keypress to dismiss it, then
    // resumes normal rendering -- see docs/ARCHITECTURE.md. `currentDay`
    // (hoursElapsed/24) is only used to display remaining spell slots
    // correctly without mutating `character` -- see
    // character::hasSpellSlotAvailable.
    static void drawCharacterSheet(const character::Character& character, long long currentDay);

    // Renders one combat frame: both combatants' HP/AC, a scrolling combat
    // log (most recent entries last -- only the tail that fits is shown),
    // and the available actions. `monsterHp` is tracked by
    // GameLoop::runCombat, not the Monster struct itself (which is static
    // content shared by every encounter with that monster type) -- see
    // docs/COMBAT_NOTES.md.
    static void drawCombatFrame(const character::Character& character, const combat::Monster& monster,
                                 int monsterHp, int monsterMaxHp, const std::vector<std::string>& log);

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

    // Renders the carried-items screen: the three equipped slots (Armor/
    // Shield/Weapon) at top, then a cursor list of character.inventory
    // (label format matches ShopItem::label) with `selectedIndex` marked.
    // GameLoop::handleInventory() takes over input itself while this is
    // showing -- same nested-loop shape as drawShopFrame (North/South move
    // the cursor, Enter equips, Quit leaves).
    static void drawInventoryFrame(const character::Character& character, int selectedIndex);
};

} // namespace render
