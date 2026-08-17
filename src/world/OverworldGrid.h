#pragma once

#include <string>
#include <vector>

namespace world {

// Owns the overworld terrain grid loaded from data/overworld.grid
// (generated offline by tools/generate_overworld.py -- see
// docs/MAP_NOTES.md). Each character is a terrain code understood by
// world::terrainFor().
class OverworldGrid {
public:
    static OverworldGrid loadFromFile(const std::string& path);

    int width() const { return width_; }
    int height() const { return height_; }

    // Returns '?' (uncharted/impassable) for any out-of-bounds coordinate
    // rather than throwing -- both movement and rendering need "off the
    // edge of the map" to be a normal, handleable case, not an error.
    char terrainCodeAt(int x, int y) const;

private:
    OverworldGrid(std::vector<std::string> rows, int width, int height);

    std::vector<std::string> rows_;
    int width_ = 0;
    int height_ = 0;
};

} // namespace world
