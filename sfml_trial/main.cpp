// Round 3 SFML spike -- see docs/CURRENT_WORK.md and the plan this was
// built from. Throwaway, isolated proof that the real map image
// (References/dragonlancemap2.png, used with the artist's permission --
// see docs/MAP_NOTES.md) can be rendered directly as the overworld, with a
// scrolling camera and location markers, instead of a hand-drawn tile
// atlas (round 2's approach). Not part of the real game: reuses
// world::OverworldGrid/World/WorldLoader directly (the actual loading
// code) but touches nothing else in render/ or game/.

#include "world/OverworldGrid.h"
#include "world/Terrain.h"
#include "world/World.h"
#include "world/WorldLoader.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace {

int runTrial() {
    const unsigned windowW = 1280;
    const unsigned windowH = 800;

    std::cout << "step 0: starting" << std::endl;

    world::OverworldGrid grid =
        world::OverworldGrid::loadFromFile("data/overworld.grid", "data/overworld_regions.grid");
    std::cout << "step 1: grid loaded " << grid.width() << "x" << grid.height() << std::endl;

    world::World world;
    world::WorldLoader::loadFromFile("data/locations.txt", world);
    std::cout << "step 2: world loaded, " << world.allLocations().size() << " locations" << std::endl;

    const world::Location* solace = world.getLocation("solace");
    if (!solace) {
        throw std::runtime_error("data/locations.txt: no 'solace' location found (needed as camera start)");
    }

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(windowW, windowH)),
                             "Ansalon SFML Trial 3 -- Real Map Overworld (spike)");
    window.setFramerateLimit(60);
    std::cout << "step 3: window created; GPU max texture size = " << sf::Texture::getMaximumSize() << std::endl;

    sf::Texture mapTexture;
    if (!mapTexture.loadFromFile("References/dragonlancemap2.png")) {
        std::cerr << "Failed to load References/dragonlancemap2.png "
                     "(see GPU max texture size logged above -- image is 8192x5461)\n";
        return 1;
    }
    const sf::Vector2u mapSize = mapTexture.getSize();
    std::cout << "step 4: map texture loaded, size " << mapSize.x << "x" << mapSize.y << std::endl;

    // Scale factors derived from the *actually loaded* texture divided by
    // the real grid dimensions -- not hardcoded -- so this stays correct if
    // the source image is ever swapped for a different resolution.
    const float pxPerTileX = static_cast<float>(mapSize.x) / static_cast<float>(grid.width());
    const float pxPerTileY = static_cast<float>(mapSize.y) / static_cast<float>(grid.height());
    std::cout << "step 5: scale " << pxPerTileX << "x" << pxPerTileY << " px/tile" << std::endl;

    // Player state: a real grid position, moved one tile at a time and
    // collision-checked against world::terrainFor -- the same passability
    // table game::GameLoop::tryMoveOverworld uses in the real game (see
    // src/world/Terrain.h). Starts at Solace, same as the camera always has.
    int playerGridX = solace->x;
    int playerGridY = solace->y;

    float camX = (static_cast<float>(playerGridX) + 0.5f) * pxPerTileX;
    float camY = (static_cast<float>(playerGridY) + 0.5f) * pxPerTileY;

    sf::View view(sf::Vector2f(camX, camY), sf::Vector2f(static_cast<float>(windowW), static_cast<float>(windowH)));
    window.setView(view);

    const float worldW = static_cast<float>(mapSize.x);
    const float worldH = static_cast<float>(mapSize.y);

    sf::Sprite mapSprite(mapTexture);

    sf::CircleShape locationMarker(6.f);
    locationMarker.setOrigin(sf::Vector2f(6.f, 6.f));

    sf::CircleShape playerMarker(9.f);
    playerMarker.setOrigin(sf::Vector2f(9.f, 9.f));
    playerMarker.setFillColor(sf::Color(255, 215, 0)); // gold -- live player position
    playerMarker.setPosition(sf::Vector2f(camX, camY));

    std::cout << "init ok; world pixel size " << worldW << "x" << worldH
              << ", " << world.allLocations().size() << " location markers" << std::endl;

    while (window.isOpen()) {
        try {
            while (const std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                    std::cout << "resize event: " << resized->size.x << "x" << resized->size.y << std::endl;
                    sf::Vector2f newSize(static_cast<float>(resized->size.x), static_cast<float>(resized->size.y));
                    newSize.x = std::max(newSize.x, 1.f);
                    newSize.y = std::max(newSize.y, 1.f);
                    view.setSize(newSize);
                    window.setView(view);
                } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    // One grid tile per keypress (relying on OS key-repeat
                    // for holding a direction down) rather than continuous
                    // pixel panning -- matches the real game's turn-based
                    // movement feel instead of a free-scrolling camera.
                    int dx = 0;
                    int dy = 0;
                    switch (keyPressed->code) {
                        case sf::Keyboard::Key::Left:  dx = -1; break;
                        case sf::Keyboard::Key::Right: dx = 1;  break;
                        case sf::Keyboard::Key::Up:    dy = -1; break;
                        case sf::Keyboard::Key::Down:  dy = 1;  break;
                        default: break;
                    }
                    if (dx != 0 || dy != 0) {
                        const int nx = playerGridX + dx;
                        const int ny = playerGridY + dy;
                        const world::TerrainInfo& terrain = world::terrainFor(grid.terrainCodeAt(nx, ny));
                        if (terrain.passable) {
                            playerGridX = nx;
                            playerGridY = ny;
                            if (const world::Location* here = world.locationAt(playerGridX, playerGridY)) {
                                std::cout << "arrived at " << here->name << std::endl;
                            }
                        } else {
                            std::cout << "blocked: cannot walk onto " << terrain.name << std::endl;
                        }
                    }
                }
            }

            const float playerPxX = (static_cast<float>(playerGridX) + 0.5f) * pxPerTileX;
            const float playerPxY = (static_cast<float>(playerGridY) + 0.5f) * pxPerTileY;

            const sf::Vector2f viewSize = view.getSize();
            const float halfW = viewSize.x / 2.f;
            const float halfH = viewSize.y / 2.f;
            camX = std::clamp(playerPxX, halfW, std::max(halfW, worldW - halfW));
            camY = std::clamp(playerPxY, halfH, std::max(halfH, worldH - halfH));
            view.setCenter(sf::Vector2f(camX, camY));
            window.setView(view);

            window.clear(sf::Color::Black);
            window.draw(mapSprite);

            for (const world::Location& loc : world.allLocations()) {
                const float px = (static_cast<float>(loc.x) + 0.5f) * pxPerTileX;
                const float py = (static_cast<float>(loc.y) + 0.5f) * pxPerTileY;
                locationMarker.setFillColor(loc.isTown ? sf::Color(60, 220, 90) : sf::Color(220, 60, 60));
                locationMarker.setPosition(sf::Vector2f(px, py));
                window.draw(locationMarker);
            }
            playerMarker.setPosition(sf::Vector2f(playerPxX, playerPxY));
            window.draw(playerMarker);

            window.display();

            // Self-capture for remote review: this session runs headless
            // relative to the interactive desktop, so external screen/
            // PrintWindow capture can't see OpenGL content -- only the
            // process's own framebuffer read-back can. Kept for as long as
            // this trial is being iterated on remotely; drop it if/when the
            // spike graduates into the real engine (see docs/CURRENT_WORK.md).
            static bool captured = false;
            static sf::Clock captureClock;
            if (!captured && captureClock.getElapsedTime().asSeconds() > 1.0f) {
                sf::Texture snap;
                if (snap.resize(window.getSize())) {
                    snap.update(window);
                    if (snap.copyToImage().saveToFile("sfml_trial3_capture.png")) {
                        std::cout << "captured screenshot to sfml_trial3_capture.png" << std::endl;
                    } else {
                        std::cerr << "failed to save capture" << std::endl;
                    }
                }
                captured = true;
            }
        } catch (const std::exception& e) {
            std::cerr << "EXCEPTION: " << e.what() << std::endl;
            window.close();
        } catch (...) {
            std::cerr << "UNKNOWN EXCEPTION" << std::endl;
            window.close();
        }
    }

    return 0;
}

} // namespace

int main() {
    try {
        return runTrial();
    } catch (const std::exception& e) {
        std::cerr << "FATAL EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "FATAL UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
}
