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
    // `regionsPath` is data/overworld_regions.grid -- a second, much
    // coarser classification of the same finished terrain (a large
    // sliding-window majority vote, see tools/generate_overworld.py's
    // compute_region_layer), used only by MapRenderer to decide where to
    // draw a region-boundary highlight (Milestone 148) -- never to pick a
    // tile's own displayed glyph/color, which always comes from the real
    // grid via terrainCodeAt. Must have identical dimensions to the terrain
    // grid; throws otherwise.
    static OverworldGrid loadFromFile(const std::string& path, const std::string& regionsPath);

    int width() const { return width_; }
    int height() const { return height_; }

    // Returns '?' (uncharted/impassable) for any out-of-bounds coordinate
    // rather than throwing -- both movement and rendering need "off the
    // edge of the map" to be a normal, handleable case, not an error.
    char terrainCodeAt(int x, int y) const;

    // Same out-of-bounds convention as terrainCodeAt. See the constructor
    // comment above for what this coarse layer is (and isn't) for.
    char regionCodeAt(int x, int y) const;

private:
    OverworldGrid(std::vector<std::string> rows, std::vector<std::string> regionRows, int width, int height);

    std::vector<std::string> rows_;
    std::vector<std::string> regionRows_;
    int width_ = 0;
    int height_ = 0;
};

} // namespace world
