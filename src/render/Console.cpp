#include "render/Console.h"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <iostream>
#include <string>
#endif

#include <cstdio>

namespace render {

Console::Console() {
#ifdef _WIN32
    // Windows consoles do not interpret ANSI/VT100 escape sequences (used
    // here for screen-clearing and terrain color) unless
    // ENABLE_VIRTUAL_TERMINAL_PROCESSING is explicitly turned on. Without
    // this, every escape code we print shows up as literal garbage like
    // "^[[2J" instead of doing anything. This is a per-console-mode
    // setting, not a per-process one, so we also restore the original mode
    // in the destructor rather than leaving it changed for whatever
    // process uses this console window next.
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    outHandle_ = handle;
    DWORD mode = 0;
    if (handle != INVALID_HANDLE_VALUE && handle != nullptr && GetConsoleMode(handle, &mode)) {
        originalOutMode_ = mode;
        if (SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            modeChanged_ = true;
        }
    }
    // Deliberately NOT switching the output code page to UTF-8: the game
    // sticks to plain 7-bit ASCII glyphs everywhere so rendering looks
    // identical regardless of the console's code page or font. See
    // docs/GOTCHAS.md.
#endif
}

Console::~Console() {
#ifdef _WIN32
    if (modeChanged_ && outHandle_ != nullptr) {
        SetConsoleMode(static_cast<HANDLE>(outHandle_), originalOutMode_);
    }
#endif
}

void Console::clearScreen() {
    // \x1b[2J clears the visible screen, \x1b[H moves the cursor back to
    // the top-left. Requires ENABLE_VIRTUAL_TERMINAL_PROCESSING on Windows
    // (see constructor above) -- other platforms' terminals honor this by
    // default.
    std::fputs("\x1b[2J\x1b[H", stdout);
}

Key Console::readKey() {
#ifdef _WIN32
    int c = _getch();
    if (c == 0 || c == 0xE0) {
        // Arrow keys (and other extended keys) are reported as TWO bytes on
        // Windows: a prefix (0x00 or 0xE0 depending on keyboard/driver)
        // followed by a scan code. The prefix alone is not a real key -- a
        // SECOND _getch() call is required to get the actual scan code.
        // Forgetting this is the classic conio.h bug: the scan code gets
        // silently read as if it were the *next* keypress instead.
        switch (_getch()) {
            case 72: return Key::North; // up arrow
            case 80: return Key::South; // down arrow
            case 75: return Key::West;  // left arrow
            case 77: return Key::East;  // right arrow
            default: return Key::Unknown;
        }
    }
    switch (c) {
        case 'w': case 'W': case 'k': case 'K': return Key::North;
        case 's': case 'S': case 'j': case 'J': return Key::South;
        case 'a': case 'A': case 'h': case 'H': return Key::West;
        case 'd': case 'D': case 'l': case 'L': return Key::East;
        case 'y': case 'Y': return Key::NorthWest;
        case 'u': case 'U': return Key::NorthEast;
        case 'b': case 'B': return Key::SouthWest;
        case 'n': case 'N': return Key::SouthEast;
        case ';': return Key::Look;
        case 13: return Key::Enter; // Enter/Return
        case 'q': case 'Q': case 27: return Key::Quit; // 27 = Esc
        default: return Key::Unknown;
    }
#else
    // No raw-keypress implementation for non-Windows yet -- see the
    // Console section of docs/ARCHITECTURE.md. Falling back to a blocking
    // line read keeps this compiling on other platforms without pretending
    // to support them properly; a POSIX termios-based readKey (raw mode,
    // single-character reads) is future work for an actual Linux/Mac port.
    std::string line;
    if (!std::getline(std::cin, line)) return Key::Unknown;
    if (line.empty()) return Key::Enter; // an empty line means the user just pressed Enter
    switch (line[0]) {
        case 'w': case 'k': return Key::North;
        case 's': case 'j': return Key::South;
        case 'a': case 'h': return Key::West;
        case 'd': case 'l': return Key::East;
        case 'y': return Key::NorthWest;
        case 'u': return Key::NorthEast;
        case 'b': return Key::SouthWest;
        case 'n': return Key::SouthEast;
        case ';': return Key::Look;
        case 'q': return Key::Quit;
        default: return Key::Unknown;
    }
#endif
}

} // namespace render
