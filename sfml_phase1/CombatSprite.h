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
};

// Tries assets/sprites/<id>.png (plain relative literal, same convention
// sfml_phase1/main.cpp already uses for data/ and References/ -- see
// docs/ARCHITECTURE.md's SFML section). Returns std::nullopt on any
// failure (missing file, odd width) -- never throws; this is optional
// presentation, unlike the fail-fast data loaders elsewhere in this
// project.
std::optional<CombatSpriteFrames> loadCombatSprite(const std::string& id);

}  // namespace sfml_phase1
