#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace world {

// A single point of interest inside a zone -- a specific tile that shows a
// description when the player stands on it, the overworld-Location
// equivalent for interior scenes.
struct PointOfInterest {
    char code = '?';
    std::string name;
    std::string description;
};

// A loaded walkable interior (e.g. Solace's town square), hand-authored in
// data/zones/<id>.txt -- see docs/ZONE_NOTES.md for the file grammar. Small
// by convention (must fit inside MapRenderer's viewport -- see
// docs/ARCHITECTURE.md): unlike OverworldGrid, Zone has no camera/scrolling.
class Zone {
public:
    // Built by ZoneLoader from a parsed data/zones/<id>.txt file. `rows`
    // must be rectangular (every row the same length) -- ZoneLoader
    // enforces this before constructing.
    Zone(std::string name, std::vector<std::string> rows, int entryX, int entryY,
         std::unordered_map<char, PointOfInterest> pois);

    const std::string& name() const { return name_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int entryX() const { return entryX_; }
    int entryY() const { return entryY_; }

    // Returns '#' (wall/impassable) for any out-of-bounds coordinate, so a
    // zone's edges are always a hard boundary without needing walls drawn
    // explicitly around the whole grid.
    char tileCodeAt(int x, int y) const;

    // Returns the point of interest at (x, y), or nullptr if the tile there
    // isn't a POI.
    const PointOfInterest* poiAt(int x, int y) const;

private:
    std::string name_;
    int width_ = 0;
    int height_ = 0;
    int entryX_ = 0;
    int entryY_ = 0;
    std::vector<std::string> rows_;
    std::unordered_map<char, PointOfInterest> pois_;
};

} // namespace world
