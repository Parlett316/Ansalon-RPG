#include "render/Console.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdio>

namespace render {

Console::Console() {
#ifdef _WIN32
    // Windows consoles do not interpret ANSI/VT100 escape sequences (used
    // here for screen-clearing and, later, color) unless
    // ENABLE_VIRTUAL_TERMINAL_PROCESSING is explicitly turned on. Without
    // this, every escape code we print shows up as literal garbage like
    // "^[[2J" instead of doing anything. This is a per-console-mode setting,
    // not a per-process one, so we also restore the original mode in the
    // destructor rather than leaving it changed for whatever process uses
    // this console window next.
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    outHandle_ = handle;
    DWORD mode = 0;
    if (handle != INVALID_HANDLE_VALUE && handle != nullptr && GetConsoleMode(handle, &mode)) {
        originalOutMode_ = mode;
        if (SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            modeChanged_ = true;
        }
    }
    // Deliberately NOT switching the output code page to UTF-8 here: the
    // game sticks to plain 7-bit ASCII glyphs everywhere specifically so
    // map rendering looks identical regardless of the console's code page
    // or font. See docs/GOTCHAS.md.
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
    // (see constructor above) -- on other platforms terminals honor this by
    // default.
    std::fputs("\x1b[2J\x1b[H", stdout);
}

} // namespace render
