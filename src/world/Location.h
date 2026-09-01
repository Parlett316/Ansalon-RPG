#pragma once

#include <string>

namespace world {

// Rough on-screen footprint for the World Map overview (see
// render::MapRenderer::drawWorldMapFrame and docs/MAP_NOTES.md) --
// Small (the default) draws a single cell, same as every location has
// always occupied; Medium/Large widen that footprint to signal relative
// importance. Unused by the real-time walking viewport, which always
// draws a location as its single glyph regardless of this value.
enum class MapSize { Small, Medium, Large };

// A single named place on the overworld map. Plain data -- all behavior
// (loading, rendering, movement) lives elsewhere. x/y are tile coordinates
// in the SAME coordinate space as OverworldGrid (see docs/MAP_NOTES.md for
// how these were chosen: approximate, from relative geography, not
// pixel-measured off the reference image's tiny labels).
struct Location {
    std::string id;        // stable key, e.g. "solace"
    std::string name;      // display name, e.g. "Solace"
    std::string region;    // e.g. "Abanasinia" -- unused by logic today, reserved for the
                            // future timeline engine to scope canon events by region
    std::string terrain;   // free-form descriptive tag (informational -- actual walkable
                            // terrain under the location comes from OverworldGrid)
    char glyph = '?';      // single ASCII character drawn on the overworld viewport
    int x = 0;
    int y = 0;
    std::string description;
    bool isTown = false;   // civilian settlement a knocked-out player can wake up in --
                            // see game::GameLoop::nearestRefuge and docs/COMBAT_NOTES.md
    bool seaLocked = false; // reachable only by a one-time scripted BOAT voyage (see
                            // world::BoatVoyage), so it's also a valid knockout wake-up
                            // point even though it isn't a TOWN -- otherwise a knockout
                            // near a sea-locked location strands the player somewhere
                            // they have no way back from (see game::GameLoop::
                            // nearestRefuge and docs/COMBAT_NOTES.md)
    MapSize mapSize = MapSize::Small; // World Map footprint -- see MapSize above
};

} // namespace world
