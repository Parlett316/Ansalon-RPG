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
    Enter, // enter/exit a walkable interior -- see game::GameLoop
    Sheet, // view the character sheet -- see game::GameLoop
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
    // diagonals (vi/roguelike convention), ';' to look around, Enter to
    // step into/out of a walkable interior, 'c' for the character sheet,
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
