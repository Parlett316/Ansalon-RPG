#pragma once

// Combat sprite art: idle/attack pose swap for `ansalon_sfml_phase1`'s
// battle grid tokens (see docs/ARCHITECTURE.md's SFML section, "Combat
// sprite art"). Every combatant still defaults to the pre-existing plain
// marker + letter glyph; this only applies when a sprite sheet actually
// exists for a given id under `assets/sprites/<id>.png`. "player" and
// "bren_alder" have art as of this writing -- `combat::Monster::id` and
// the rest of `game::RecruitedCompanion::id` are the natural future keys,
// needing no further code changes here when that art arrives.

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

#include "combat/CombatGrid.h"

namespace sfml_phase1 {

// Pure, unit-testable: splits a sheet's pixel width into two equal
// side-by-side frames (idle | attack), same full height. A sheet with an
// odd width can't be split evenly -- returns std::nullopt so the caller
// falls back to the plain marker rather than mis-slicing a frame.
std::optional<std::pair<sf::IntRect, sf::IntRect>> computeSpriteFrameRects(unsigned width, unsigned height);

// Pure, unit-testable: the cosmetic facing check -- true when `target` is
// to the left of `self` on the grid. This never feeds backstab (which
// stays combat::oppositeSide's own position-only check, see
// docs/COMBAT_NOTES.md) -- it only decides which way the sprite is
// mirrored to face.
bool spriteShouldFaceLeft(combat::GridPos self, combat::GridPos target);

struct CombatSpriteFrames {
    sf::Texture texture;
    sf::IntRect idleRect;
    sf::IntRect attackRect;
    // Grid cells this art should render across, from which filename suffix
    // matched (plain = 1x1, "-wide" = 2x1, "-tall" = 1x2, "-four" = 2x2).
    // Purely a rendering size -- the caller combines this with the
    // monster's real gameplay footprint (which may still be 1x1, e.g. a
    // pack monster like a wolf using "-wide" art for better proportions)
    // rather than ever shrinking a genuinely-bigger creature's footprint.
    int renderWidth = 1;
    int renderHeight = 1;
};

// Tries assets/sprites/<id>.png, then <id>-wide.png / <id>-tall.png /
// <id>-four.png (plain relative literals, same convention
// sfml_phase1/main.cpp already uses for data/ and References/ -- see
// docs/ARCHITECTURE.md's SFML section), and records which one matched as
// CombatSpriteFrames::renderWidth/Height. The suffixed variants render
// across that many grid cells (still just an idle|attack pair split down
// the middle, same as the plain case) -- this never touches occupancy/
// grouping, since drawCombatSpriteToken combines it with (not in place of)
// the monster's real gameplay footprint: a 1x1 pack monster (e.g. a wolf)
// can use "-wide" art and render visibly bigger on screen without becoming
// a real multi-cell creature for targeting/pathing/solo-grouping purposes.
// Returns std::nullopt on any failure (no file under any candidate name,
// odd width) -- never throws; this is optional presentation, unlike the
// fail-fast data loaders elsewhere in this project.
std::optional<CombatSpriteFrames> loadCombatSprite(const std::string& id);

}  // namespace sfml_phase1
