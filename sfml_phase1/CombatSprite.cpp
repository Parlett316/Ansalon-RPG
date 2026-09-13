#include "CombatSprite.h"

namespace sfml_phase1 {

std::optional<std::pair<sf::IntRect, sf::IntRect>> computeSpriteFrameRects(unsigned width, unsigned height) {
    if (width == 0 || height == 0 || width % 2 != 0) return std::nullopt;
    const int frameWidth = static_cast<int>(width / 2);
    const int frameHeight = static_cast<int>(height);
    const sf::IntRect idleRect({0, 0}, {frameWidth, frameHeight});
    const sf::IntRect attackRect({frameWidth, 0}, {frameWidth, frameHeight});
    return std::make_pair(idleRect, attackRect);
}

bool spriteShouldFaceLeft(combat::GridPos self, combat::GridPos target) { return target.x < self.x; }

std::optional<CombatSpriteFrames> loadCombatSprite(const std::string& id) {
    struct Variant {
        const char* suffix;
        int renderWidth;
        int renderHeight;
    };
    static constexpr Variant kVariants[] = {
        {"", 1, 1},
        {"-wide", 2, 1},
        {"-tall", 1, 2},
        {"-four", 2, 2},
    };
    sf::Texture texture;
    const Variant* matched = nullptr;
    for (const Variant& variant : kVariants) {
        if (texture.loadFromFile("assets/sprites/" + id + variant.suffix + ".png")) {
            matched = &variant;
            break;
        }
    }
    if (matched == nullptr) return std::nullopt;
    texture.setSmooth(false);  // keep pixel art crisp when scaled up, not blurred
    const auto rects = computeSpriteFrameRects(texture.getSize().x, texture.getSize().y);
    if (!rects.has_value()) return std::nullopt;
    CombatSpriteFrames frames;
    frames.texture = std::move(texture);
    frames.idleRect = rects->first;
    frames.attackRect = rects->second;
    frames.renderWidth = matched->renderWidth;
    frames.renderHeight = matched->renderHeight;
    return frames;
}

}  // namespace sfml_phase1
