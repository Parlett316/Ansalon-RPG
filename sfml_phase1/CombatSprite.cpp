#include "CombatSprite.h"

namespace sfml_phase1 {

namespace {

bool columnFullyOpaqueBlack(const sf::Image& image, unsigned x, unsigned height) {
    for (unsigned y = 0; y < height; ++y) {
        const sf::Color c = image.getPixel({x, y});
        if (c.a == 0 || c.r > 10 || c.g > 10 || c.b > 10) return false;
    }
    return true;
}

}  // namespace

std::optional<std::pair<sf::IntRect, sf::IntRect>> computeSpriteFrameRects(const sf::Image& image) {
    const sf::Vector2u size = image.getSize();
    const unsigned width = size.x;
    const unsigned height = size.y;
    if (width == 0 || height == 0) return std::nullopt;

    // Trim a real 1px-or-more opaque-black border touching either edge --
    // present on every sampled file (see this function's header comment)
    // but harmless to look for even on art that lacks it.
    unsigned leftContent = 0;
    while (leftContent < width && columnFullyOpaqueBlack(image, leftContent, height)) ++leftContent;
    unsigned rightContent = width;  // exclusive
    while (rightContent > leftContent && columnFullyOpaqueBlack(image, rightContent - 1, height)) --rightContent;
    if (leftContent >= rightContent) return std::nullopt;

    // Within the border-trimmed content, look for exactly one interior
    // run of fully-opaque-black, full-height columns -- the idle|attack
    // divider. Zero or more than one such run is treated as "no divider
    // convention here" and falls back to a plain half-width split below,
    // rather than guessing wrong.
    unsigned dividerStart = 0;
    unsigned dividerEnd = 0;  // exclusive
    int dividerRuns = 0;
    for (unsigned x = leftContent; x < rightContent;) {
        if (!columnFullyOpaqueBlack(image, x, height)) {
            ++x;
            continue;
        }
        const unsigned runStart = x;
        while (x < rightContent && columnFullyOpaqueBlack(image, x, height)) ++x;
        ++dividerRuns;
        dividerStart = runStart;
        dividerEnd = x;
    }

    unsigned idleLeft, idleRight, attackLeft, attackRight;
    if (dividerRuns == 1) {
        idleLeft = leftContent;
        idleRight = dividerStart;
        attackLeft = dividerEnd;
        attackRight = rightContent;
    } else {
        const unsigned contentWidth = rightContent - leftContent;
        if (contentWidth == 0 || contentWidth % 2 != 0) return std::nullopt;
        idleLeft = leftContent;
        idleRight = leftContent + contentWidth / 2;
        attackLeft = idleRight;
        attackRight = rightContent;
    }
    if (idleLeft >= idleRight || attackLeft >= attackRight) return std::nullopt;

    const sf::IntRect idleRect({static_cast<int>(idleLeft), 0},
                                {static_cast<int>(idleRight - idleLeft), static_cast<int>(height)});
    const sf::IntRect attackRect({static_cast<int>(attackLeft), 0},
                                  {static_cast<int>(attackRight - attackLeft), static_cast<int>(height)});
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
    sf::Image image;
    const Variant* matched = nullptr;
    for (const Variant& variant : kVariants) {
        if (image.loadFromFile("assets/sprites/" + id + variant.suffix + ".png")) {
            matched = &variant;
            break;
        }
    }
    if (matched == nullptr) return std::nullopt;
    // Inspected as a CPU-side sf::Image (border/divider detection needs
    // per-pixel reads) before handing the same pixels to the GPU texture
    // sprites actually draw from.
    const auto rects = computeSpriteFrameRects(image);
    if (!rects.has_value()) return std::nullopt;
    sf::Texture texture;
    if (!texture.loadFromImage(image)) return std::nullopt;
    texture.setSmooth(false);  // keep pixel art crisp when scaled up, not blurred
    CombatSpriteFrames frames;
    frames.texture = std::move(texture);
    frames.idleRect = rects->first;
    frames.attackRect = rects->second;
    frames.renderWidth = matched->renderWidth;
    frames.renderHeight = matched->renderHeight;
    return frames;
}

}  // namespace sfml_phase1
