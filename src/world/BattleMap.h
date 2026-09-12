#pragma once

#include <string>
#include <vector>

namespace world {

// A hand-authored wall layout for one overworld terrain's combat encounters
// -- data/battlemaps/<terrain-name>.txt, loaded by BattleMapLoader. Milestone
// 188's answer to the DQoK-screenshot request for "impassable walls forming
// rooms/cover" (see docs/COMBAT_NOTES.md's own Milestone 188 section).
//
// Deliberately a much smaller sibling of Zone (world/Zone.h), not a reuse of
// it: a battlemap has no POIs, no entry point, no NPCs, nothing to look at --
// just a passable/impassable grid the size of the SFML combat screen's own
// tactical grid (ansalon_sfml_phase1 only; ansalon_rpg's 15x9 console combat
// has no wall concept and never will -- see CLAUDE.md's SFML-only precedent
// for this whole milestone chain). No dependency on combat:: here either --
// world:: stays decoupled from combat::/character:: (see docs/ZONE_NOTES.md's
// own note on Zone/PointOfInterest doing the same); sfml_phase1/main.cpp is
// the one place both namespaces meet.
class BattleMap {
public:
    // `rows` must already be rectangular and exactly width x height --
    // BattleMapLoader enforces this before constructing, same division of
    // responsibility as Zone/ZoneLoader.
    BattleMap(std::vector<std::string> rows, int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    // True for '#' at (x, y), and true for ANY out-of-bounds coordinate too
    // -- mirrors Zone::tileCodeAt's "edges are a hard boundary" convention,
    // so a caller never needs its own separate bounds check before asking.
    bool isWall(int x, int y) const;

private:
    std::vector<std::string> rows_;
    int width_ = 0;
    int height_ = 0;
};

} // namespace world
