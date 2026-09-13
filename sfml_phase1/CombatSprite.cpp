#include "CombatSprite.h"

namespace sfml_phase1 {

namespace {

bool columnFullyOpaqueBlack(const sf::Image& image, unsigned x, unsigned topY, unsigned bottomY) {
    for (unsigned y = topY; y < bottomY; ++y) {
        const sf::Color c = image.getPixel({x, y});
        if (c.a == 0 || c.r > 10 || c.g > 10 || c.b > 10) return false;
    }
    return true;
}

bool rowFullyOpaqueBlack(const sf::Image& image, unsigned y, unsigned width) {
    for (unsigned x = 0; x < width; ++x) {
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

    // Trim a real 1px-or-more opaque-black border touching any of the
    // four edges -- present on every sampled file (see this function's
    // header comment) but harmless to look for even on art that lacks
    // it. Top/bottom trimmed first (uniform across both frames, no
    // idle|attack split on this axis) so the left/right divider search
    // below only looks at real content rows.
    unsigned topContent = 0;
    while (topContent < height && rowFullyOpaqueBlack(image, topContent, width)) ++topContent;
    unsigned bottomContent = height;  // exclusive
    while (bottomContent > topContent && rowFullyOpaqueBlack(image, bottomContent - 1, width)) --bottomContent;
    if (topContent >= bottomContent) return std::nullopt;

    unsigned leftContent = 0;
    while (leftContent < width && columnFullyOpaqueBlack(image, leftContent, topContent, bottomContent)) {
        ++leftContent;
    }
    unsigned rightContent = width;  // exclusive
    while (rightContent > leftContent &&
           columnFullyOpaqueBlack(image, rightContent - 1, topContent, bottomContent)) {
        --rightContent;
    }
    if (leftContent >= rightContent) return std::nullopt;

    // Within the border-trimmed content, look for exactly one interior
    // run of fully-opaque-black columns -- the idle|attack divider. Zero
    // or more than one such run is treated as "no divider convention
    // here" and falls back to a plain half-width split below, rather
    // than guessing wrong.
    unsigned dividerStart = 0;
    unsigned dividerEnd = 0;  // exclusive
    int dividerRuns = 0;
    for (unsigned x = leftContent; x < rightContent;) {
        if (!columnFullyOpaqueBlack(image, x, topContent, bottomContent)) {
            ++x;
            continue;
        }
        const unsigned runStart = x;
        while (x < rightContent && columnFullyOpaqueBlack(image, x, topContent, bottomContent)) ++x;
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

    const int contentTop = static_cast<int>(topContent);
    const int contentHeight = static_cast<int>(bottomContent - topContent);
    const sf::IntRect idleRect({static_cast<int>(idleLeft), contentTop},
                                {static_cast<int>(idleRight - idleLeft), contentHeight});
    const sf::IntRect attackRect({static_cast<int>(attackLeft), contentTop},
                                  {static_cast<int>(attackRight - attackLeft), contentHeight});
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
