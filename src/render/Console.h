#pragma once

#include <cstddef>
#include <string>

namespace render {

enum class Key {
    North,
    South,
    East,
    West,
    Look,
    Talk,  // talk to a present NPC/canon character -- see game::GameLoop::handleTalk
    Enter, // enter/exit a walkable interior -- see game::GameLoop
    Sheet, // view the character sheet -- see game::GameLoop
    Shop,  // browse/buy at a shop POI -- see game::GameLoop::handleShop
    Inventory, // carried items / equip-unequip -- see game::GameLoop::handleInventory
    Log,   // dedicated scrollable full-history log screen -- see game::GameLoop::handleLog
    Journal, // quest journal -- see game::GameLoop::showJournal
    WorldMap, // read-only zoomed-out continent overview -- see game::GameLoop::showWorldMap
    Flee,  // retreat from combat -- see game::GameLoop::runCombat
    Cast,  // cast the character's one known spell in combat -- see game::GameLoop::runCombat
    Rest,  // rest to heal and (re)memorize spells -- see game::GameLoop::handleRest
    BedRest, // fully heal at a bed POI -- see game::GameLoop::handleBedRest
    Help,  // show the command help screen -- see game::GameLoop::showHelp
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
    // wasd for the 4 cardinal directions (no diagonals -- see
    // docs/GOTCHAS.md; arrow keys are also accepted as a silent, on-screen-
    // undocumented alias, Windows only), 'l' to look around, 't' to talk
    // to a present NPC/
    // canon character, Enter to step into/out of a walkable interior (or
    // attack, during combat -- see game::GameLoop::runCombat), 'c' for the
    // character sheet, 'p' to browse/buy at a shop, 'i' for the inventory/
    // equip screen, 'v' for the scrollable full log-history screen, 'g' for
    // the quest journal, 'f' to flee combat, 'm' to cast in combat, 'r' to
    // rest, 'z' to fully heal at a bed POI, 'q'/Esc to quit.
    static Key readKey();

    // Blocks reading a free-typed line of text, echoing it as the player
    // types (used by the "ask about something else..." free-text prompt --
    // see game::GameLoop::talkTo and docs/TIMELINE_NOTES.md's "Ask about
    // anything"). Deliberately reads via the same raw _getch() primitive
    // readKey() uses, one character at a time, rather than std::cin/
    // std::getline -- those are only ever used pre-GameLoop, by
    // CharacterCreator, and mixing them with a live _getch() loop is
    // unproven territory this project has never needed before (see
    // docs/GOTCHAS.md). Enter submits and returns the typed text (capped at
    // `maxLength`, extra keystrokes past the cap are ignored); Backspace
    // edits; Esc cancels and returns an empty string. **Esc, not 'q', is
    // cancel here** -- a deliberate deviation from this game's usual Quit
    // convention, since 'q' is a perfectly ordinary character to type in a
    // free-text question (see docs/GOTCHAS.md).
    static std::string readLine(std::size_t maxLength);

    // Discards any keypresses already waiting in the OS input buffer, so a
    // key pressed for one input mode (e.g. a direction key mashed while
    // still travelling the overworld) can never be silently replayed as the
    // first input of a mode transition where the same key means something
    // very different (e.g. combat, where a direction key is a full round
    // action) -- see docs/GOTCHAS.md. No-op on non-Windows, where no real
    // raw-keypress implementation exists yet (same honesty precedent as
    // readKey/executableDirectory above).
    static void flushInput();

    // Directory containing the running executable (no trailing slash), used
    // to locate data/ and save.txt next to a distributed build instead of a
    // baked-in source-tree path -- see docs/GOTCHAS.md. Empty string if it
    // can't be determined (non-Windows, where no real implementation exists
    // yet -- same honesty precedent as readKey/currentWindowSize above).
    static std::string executableDirectory();

private:
#ifdef _WIN32
    void* outHandle_ = nullptr;
    unsigned long originalOutMode_ = 0;
    bool modeChanged_ = false;
#endif
};

} // namespace render
