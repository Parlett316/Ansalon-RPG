#pragma once

namespace render {

enum class Key {
    North,
    South,
    East,
    West,
    NorthEast,
    NorthWest,
    SouthEast,
    SouthWest,
    Look,
    Talk,  // talk to a present NPC/canon character -- see game::GameLoop::handleTalk
    Enter, // enter/exit a walkable interior -- see game::GameLoop
    Sheet, // view the character sheet -- see game::GameLoop
    Shop,  // browse/buy at a shop POI -- see game::GameLoop::handleShop
    Inventory, // carried items / equip-unequip -- see game::GameLoop::handleInventory
    Log,   // dedicated scrollable full-history log screen -- see game::GameLoop::handleLog
    Flee,  // retreat from combat -- see game::GameLoop::runCombat
    Cast,  // cast the character's one known spell in combat -- see game::GameLoop::runCombat
    Rest,  // rest to heal and (re)memorize spells -- see game::GameLoop::handleRest
    BedRest, // fully heal at a bed POI -- see game::GameLoop::handleBedRest
    Quit,
    Unknown,
};

// The real console window's visible size, in character cells -- see
// Console::currentWindowSize().
struct WindowSize {
    int columns;
    int rows;
};

// Owns terminal setup/teardown and raw keyboard input. On Windows this
// enables ANSI/VT100 escape sequence processing for the lifetime of the
// object (restoring the original console mode on destruction -- see
// docs/GOTCHAS.md) and reads single keypresses without waiting for Enter,
// which is what makes movement feel immediate rather than command-based.
// All platform-specific console handling lives in this one class -- it is
// the single file a future Linux/Mac port would need to reimplement.
class Console {
public:
    Console();
    ~Console();

    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    static void clearScreen();

    // Queries the real, currently visible console window size (not the
    // scrollback buffer, which can be much taller -- see docs/GOTCHAS.md
    // for why that distinction matters). Used once at startup by main.cpp
    // to size MapRenderer's layout to what's actually on screen -- see
    // MapRenderer::configureLayout and docs/ARCHITECTURE.md. Falls back to
    // a conservative {80, 24} if the query fails (stdout redirected, e.g.
    // a piped smoke test or the throwaway self-test pattern) or on
    // non-Windows, where no real implementation exists yet -- same
    // honesty precedent as readKey's non-Windows fallback below.
    static WindowSize currentWindowSize();

    // Blocks until a key is pressed and returns what it means. Bindings:
    // arrows / hjkl / wasd for the 4 cardinal directions, yubn for the 4
    // diagonals (vi/roguelike convention), ';' to look around, 't' to talk
    // to a present NPC/canon character, Enter to step into/out of a
    // walkable interior (or attack, during combat -- see
    // game::GameLoop::runCombat), 'c' for the character sheet, 'p' to
    // browse/buy at a shop, 'i' for the inventory/equip screen, 'v' for
    // the scrollable full log-history screen, 'f' to flee combat, 'm' to
    // cast in combat, 'r' to rest, 'z' to fully heal at a bed POI (not 'b'
    // -- that's already SouthWest in the yubn diagonal-movement scheme),
    // 'q'/Esc to quit. See docs/GOTCHAS.md for the Windows arrow-key
    // decoding quirk.
    static Key readKey();

private:
#ifdef _WIN32
    void* outHandle_ = nullptr;
    unsigned long originalOutMode_ = 0;
    bool modeChanged_ = false;
#endif
};

} // namespace render
