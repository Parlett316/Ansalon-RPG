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

WindowSize Console::currentWindowSize() {
    constexpr WindowSize kFallback{80, 24};
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (handle == INVALID_HANDLE_VALUE || handle == nullptr || !GetConsoleScreenBufferInfo(handle, &info)) {
        return kFallback; // e.g. stdout redirected to a file/pipe -- not a real console
    }
    // srWindow is the VISIBLE window rectangle -- deliberately not
    // dwSize, which is the scrollback buffer's size and can be far
    // taller than what's actually on screen without scrolling. Using
    // dwSize here would silently reintroduce the exact "bigger than my
    // console" bug this function exists to fix -- see docs/GOTCHAS.md.
    int columns = info.srWindow.Right - info.srWindow.Left + 1;
    int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    return WindowSize{columns, rows};
#else
    return kFallback;
#endif
}

std::string Console::executableDirectory() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        return ""; // failed or truncated -- caller falls back to the compile-time path
    }
    std::string path(buffer, len);
    std::size_t slash = path.find_last_of("\\/");
    return slash == std::string::npos ? "" : path.substr(0, slash);
#else
    return "";
#endif
}

Key Console::readKey() {
#ifdef _WIN32
    int c = _getch();
    if (c == 0 || c == 0xE0) {
        // Extended keys (arrows, function keys, etc.) are reported as TWO
        // bytes on Windows: a prefix (0x00 or 0xE0 depending on keyboard/
        // driver) followed by a scan code that a SECOND _getch() call is
        // required to consume -- forgetting it causes the scan code to be
        // silently read as if it were the *next* keypress. See
        // docs/GOTCHAS.md. Arrow keys are a deliberately undocumented
        // silent alias for wasd (see Milestone 63's follow-up) -- every
        // other extended key still falls through to Unknown.
        switch (_getch()) {
            case 72: return Key::North; // up arrow
            case 80: return Key::South; // down arrow
            case 75: return Key::West;  // left arrow
            case 77: return Key::East;  // right arrow
            default: return Key::Unknown;
        }
    }
    switch (c) {
        case 'w': case 'W': return Key::North;
        case 's': case 'S': return Key::South;
        case 'a': case 'A': return Key::West;
        case 'd': case 'D': return Key::East;
        case 'l': case 'L': return Key::Look;
        case 't': case 'T': return Key::Talk;
        case 13: return Key::Enter; // Enter/Return
        case 'c': case 'C': return Key::Sheet;
        case 'p': case 'P': return Key::Shop;
        case 'i': case 'I': return Key::Inventory;
        case 'v': case 'V': return Key::Log;
        case 'g': case 'G': return Key::Journal;
        case 'f': case 'F': return Key::Flee;
        case 'm': case 'M': return Key::Cast;
        case 'r': case 'R': return Key::Rest;
        case 'z': case 'Z': return Key::BedRest;
        case '?': return Key::Help;
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
        case 'w': return Key::North;
        case 's': return Key::South;
        case 'a': return Key::West;
        case 'd': return Key::East;
        case 'l': return Key::Look;
        case 't': return Key::Talk;
        case 'c': return Key::Sheet;
        case 'p': return Key::Shop;
        case 'i': return Key::Inventory;
        case 'v': return Key::Log;
        case 'g': return Key::Journal;
        case 'f': return Key::Flee;
        case 'm': return Key::Cast;
        case 'r': return Key::Rest;
        case 'z': return Key::BedRest;
        case '?': return Key::Help;
        case 'q': return Key::Quit;
        default: return Key::Unknown;
    }
#endif
}

std::string Console::readLine(std::size_t maxLength) {
#ifdef _WIN32
    std::string buffer;
    for (;;) {
        int c = _getch();
        if (c == 0 || c == 0xE0) {
            _getch(); // eat the extended-key second byte -- see readKey's own comment above
            continue;  // arrows/function keys have no meaning while typing free text
        }
        if (c == 13) { // Enter
            std::fputc('\n', stdout);
            return buffer;
        }
        if (c == 27) { // Esc -- cancel; deliberately not 'q', see the header doc comment
            std::fputc('\n', stdout);
            return "";
        }
        if (c == 8) { // Backspace
            if (!buffer.empty()) {
                buffer.pop_back();
                std::fputs("\b \b", stdout);
                std::fflush(stdout);
            }
            continue;
        }
        // True 7-bit ASCII only (see docs/GOTCHAS.md) -- anything else is
        // silently ignored rather than echoed as garbage.
        if (c >= 0x20 && c < 0x7F && buffer.size() < maxLength) {
            buffer.push_back(static_cast<char>(c));
            std::fputc(c, stdout);
            std::fflush(stdout);
        }
    }
#else
    std::string line;
    if (!std::getline(std::cin, line)) return "";
    if (line.size() > maxLength) line.resize(maxLength);
    return line;
#endif
}

void Console::flushInput() {
#ifdef _WIN32
    // _kbhit() reports whether a keypress is waiting without consuming it;
    // draining with _getch() until it's false empties the buffer. Same
    // conio.h primitives as readKey/readLine, not a console-handle API, so
    // this needs no new Windows dependency. No-op on non-Windows -- see the
    // header doc comment.
    while (_kbhit()) {
        _getch();
    }
#endif
}

} // namespace render
