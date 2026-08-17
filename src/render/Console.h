#pragma once

namespace render {

// Owns terminal setup/teardown. On Windows this enables ANSI/VT100 escape
// sequence processing for the lifetime of the object and restores the
// console's original mode on destruction (see docs/GOTCHAS.md for why this
// is necessary at all). All platform-specific console handling lives in
// this one class -- it is the single file a future Linux/Mac port would
// need to reimplement, since real terminals there already interpret ANSI
// codes by default.
class Console {
public:
    Console();
    ~Console();

    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    static void clearScreen();

private:
#ifdef _WIN32
    void* outHandle_ = nullptr;
    unsigned long originalOutMode_ = 0;
    bool modeChanged_ = false;
#endif
};

} // namespace render
