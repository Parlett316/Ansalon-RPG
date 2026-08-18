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
    Flee,  // retreat from combat -- see game::GameLoop::runCombat
    Cast,  // cast the character's one known spell in combat -- see game::GameLoop::runCombat
    Quit,
    Unknown,
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

    // Blocks until a key is pressed and returns what it means. Bindings:
    // arrows / hjkl / wasd for the 4 cardinal directions, yubn for the 4
    // diagonals (vi/roguelike convention), ';' to look around, 't' to talk
    // to a present NPC/canon character, Enter to step into/out of a
    // walkable interior (or attack, during combat -- see
    // game::GameLoop::runCombat), 'c' for the character sheet, 'p' to
    // browse/buy at a shop, 'i' for the inventory/equip screen, 'f' to
    // flee combat, 'm' to cast in combat, 'q'/Esc to quit. See
    // docs/GOTCHAS.md for the Windows arrow-key
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
