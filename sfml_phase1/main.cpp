// Full-migration Phase 1 -- see docs/CURRENT_WORK.md and the plan this was
// built from. A real, pixel-space overworld screen driven by real save
// data: proves the rendering/collision approach end-to-end before any of
// zones/combat/menus are attempted. Deliberately standalone rather than
// reusing game::GameLoop::run() -- see this file's CMakeLists.txt comment
// for why. Loads a save file read-only via game::SaveGame -- never writes
// back, never touches the real ansalon_rpg target's code path.

#include "character/CharClass.h"
#include "character/Race.h"
#include "game/GameState.h"
#include "game/SaveGame.h"
#include "world/OverworldGrid.h"
#include "world/Terrain.h"
#include "world/World.h"
#include "world/WorldLoader.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <deque>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

constexpr std::size_t kMaxLogLines = 14;
constexpr unsigned kSidebarCharSize = 16;
// Rough monospace advance width for kSidebarCharSize in Consolas -- used
// only to wrap placeholder log text to the sidebar's pixel width, not for
// precise layout. Recalibrate if the font or character size changes.
constexpr float kSidebarCharWidth = 9.5f;

std::string formatDayTime(long long hoursElapsed) {
    const long long day = hoursElapsed / 24;
    const long long hour = hoursElapsed % 24;
    std::ostringstream oss;
    oss << "Day " << day << ", " << (hour < 10 ? "0" : "") << hour << ":00";
    return oss.str();
}

// Greedy word-wrap so sidebar log lines don't run off the panel -- the
// sidebar is placeholder-styled (per the approved plan), but unwrapped
// overflowing text would undermine even that, so this one bit of layout
// care is worth it.
std::vector<std::string> wrapToWidth(const std::string& text, std::size_t maxChars) {
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word;
    std::string current;
    while (words >> word) {
        const std::string candidate = current.empty() ? word : current + " " + word;
        if (candidate.size() > maxChars && !current.empty()) {
            lines.push_back(current);
            current = word;
        } else {
            current = candidate;
        }
    }
    if (!current.empty()) {
        lines.push_back(current);
    }
    if (lines.empty()) {
        lines.push_back(std::string());
    }
    return lines;
}

int runPhase1(const std::string& savePath) {
    const unsigned windowW = 1280;
    const unsigned windowH = 800;
    const float sidebarWidth = 320.f;
    const float mapWidth = static_cast<float>(windowW) - sidebarWidth;

    std::cout << "step 0: starting, save = " << savePath << std::endl;

    world::OverworldGrid grid =
        world::OverworldGrid::loadFromFile("data/overworld.grid", "data/overworld_regions.grid");
    std::cout << "step 1: grid loaded " << grid.width() << "x" << grid.height() << std::endl;

    world::World world;
    world::WorldLoader::loadFromFile("data/locations.txt", world);
    std::cout << "step 2: world loaded, " << world.allLocations().size() << " locations" << std::endl;

    game::GameState state = game::SaveGame::load(savePath);
    std::cout << "step 3: save loaded -- " << state.character.name << ", level "
              << state.character.level << " " << character::raceInfo(state.character.race).name << " "
              << character::classInfo(state.character.charClass).name << ", at (" << state.x << ", "
              << state.y << ")" << std::endl;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(windowW, windowH)),
                             "Ansalon SFML Phase 1 -- Real Overworld (WIP)");
    window.setFramerateLimit(60);

    sf::Texture mapTexture;
    if (!mapTexture.loadFromFile("References/dragonlancemap2.png")) {
        std::cerr << "Failed to load References/dragonlancemap2.png\n";
        return 1;
    }
    const sf::Vector2u mapSize = mapTexture.getSize();
    std::cout << "step 4: map texture loaded, size " << mapSize.x << "x" << mapSize.y << std::endl;

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/consola.ttf")) {
        std::cerr << "Failed to load placeholder UI font (C:/Windows/Fonts/consola.ttf) -- "
                     "this build uses a system font as a stand-in until this project has its own.\n";
        return 1;
    }

    const float pxPerTileX = static_cast<float>(mapSize.x) / static_cast<float>(grid.width());
    const float pxPerTileY = static_cast<float>(mapSize.y) / static_cast<float>(grid.height());
    const float worldW = static_cast<float>(mapSize.x);
    const float worldH = static_cast<float>(mapSize.y);

    int playerGridX = state.x;
    int playerGridY = state.y;

    sf::View mapView(sf::Vector2f((static_cast<float>(playerGridX) + 0.5f) * pxPerTileX,
                                   (static_cast<float>(playerGridY) + 0.5f) * pxPerTileY),
                      sf::Vector2f(mapWidth, static_cast<float>(windowH)));
    mapView.setViewport(sf::FloatRect({0.f, 0.f}, {mapWidth / static_cast<float>(windowW), 1.f}));

    const sf::View uiView = window.getDefaultView();

    sf::Sprite mapSprite(mapTexture);

    sf::CircleShape locationMarker(6.f);
    locationMarker.setOrigin(sf::Vector2f(6.f, 6.f));

    sf::CircleShape playerMarker(9.f);
    playerMarker.setOrigin(sf::Vector2f(9.f, 9.f));
    playerMarker.setFillColor(sf::Color(255, 215, 0));

    sf::RectangleShape sidebarBg(sf::Vector2f(sidebarWidth, static_cast<float>(windowH)));
    sidebarBg.setPosition(sf::Vector2f(mapWidth, 0.f));
    sidebarBg.setFillColor(sf::Color(20, 20, 28));

    std::deque<std::string> log;
    auto pushLog = [&log](const std::string& text) {
        log.push_back(text);
        while (log.size() > kMaxLogLines) {
            log.pop_front();
        }
    };

    pushLog("Loaded " + state.character.name + ".");
    if (state.mode == game::Mode::Zone) {
        pushLog("Note: save is mid-zone (" + state.currentZoneId +
                "); showing last overworld position -- zones aren't in this build yet.");
    }
    pushLog("Movement only. Other keys are placeholders for now.");

    std::cout << "init ok; world pixel size " << worldW << "x" << worldH << std::endl;

    while (window.isOpen()) {
        try {
            while (const std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    const sf::Keyboard::Key key = keyPressed->code;
                    int dx = 0;
                    int dy = 0;
                    std::string placeholder;
                    switch (key) {
                        case sf::Keyboard::Key::W:
                        case sf::Keyboard::Key::Up: dy = -1; break;
                        case sf::Keyboard::Key::S:
                        case sf::Keyboard::Key::Down: dy = 1; break;
                        case sf::Keyboard::Key::A:
                        case sf::Keyboard::Key::Left: dx = -1; break;
                        case sf::Keyboard::Key::D:
                        case sf::Keyboard::Key::Right: dx = 1; break;
                        case sf::Keyboard::Key::Q:
                        case sf::Keyboard::Key::Escape: window.close(); break;
                        case sf::Keyboard::Key::L: placeholder = "Look: not yet implemented in this build."; break;
                        case sf::Keyboard::Key::T: placeholder = "Talk: not yet implemented in this build."; break;
                        case sf::Keyboard::Key::Enter:
                            placeholder = "Enter: zones aren't in this build yet.";
                            break;
                        case sf::Keyboard::Key::C:
                            placeholder = "Character sheet: not yet implemented in this build.";
                            break;
                        case sf::Keyboard::Key::P: placeholder = "Shop: not yet implemented in this build."; break;
                        case sf::Keyboard::Key::I:
                            placeholder = "Inventory: not yet implemented in this build.";
                            break;
                        case sf::Keyboard::Key::V:
                            placeholder = "Full log view: not yet implemented in this build.";
                            break;
                        case sf::Keyboard::Key::G:
                            placeholder = "Journal: not yet implemented in this build.";
                            break;
                        case sf::Keyboard::Key::F: placeholder = "Flee: not available outside combat."; break;
                        case sf::Keyboard::Key::M: placeholder = "Cast: not available outside combat."; break;
                        case sf::Keyboard::Key::R: placeholder = "Rest: not yet implemented in this build."; break;
                        case sf::Keyboard::Key::Z:
                            placeholder = "Bed rest: not yet implemented in this build.";
                            break;
                        default: break;
                    }
                    if (!placeholder.empty()) {
                        pushLog(placeholder);
                    } else if (dx != 0 || dy != 0) {
                        const int nx = playerGridX + dx;
                        const int ny = playerGridY + dy;
                        const world::TerrainInfo& terrain = world::terrainFor(grid.terrainCodeAt(nx, ny));
                        if (terrain.passable) {
                            playerGridX = nx;
                            playerGridY = ny;
                            if (const world::Location* here = world.locationAt(playerGridX, playerGridY)) {
                                pushLog("Arrived at " + here->name + ".");
                            }
                        } else {
                            pushLog("Blocked: cannot walk onto " + std::string(terrain.name) + ".");
                        }
                    }
                }
            }

            const float playerPxX = (static_cast<float>(playerGridX) + 0.5f) * pxPerTileX;
            const float playerPxY = (static_cast<float>(playerGridY) + 0.5f) * pxPerTileY;

            const sf::Vector2f viewSize = mapView.getSize();
            const float halfW = viewSize.x / 2.f;
            const float halfH = viewSize.y / 2.f;
            const float camX = std::clamp(playerPxX, halfW, std::max(halfW, worldW - halfW));
            const float camY = std::clamp(playerPxY, halfH, std::max(halfH, worldH - halfH));
            mapView.setCenter(sf::Vector2f(camX, camY));

            window.clear(sf::Color::Black);

            window.setView(mapView);
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

            window.setView(uiView);
            window.draw(sidebarBg);

            float lineY = 16.f;
            const float lineX = mapWidth + 16.f;
            const float lineHeight = static_cast<float>(kSidebarCharSize) + 6.f;
            auto drawLine = [&](const std::string& text, sf::Color color) {
                sf::Text sfText(font, text, kSidebarCharSize);
                sfText.setFillColor(color);
                sfText.setPosition(sf::Vector2f(lineX, lineY));
                window.draw(sfText);
                lineY += lineHeight;
            };

            drawLine(state.character.name + ", level " + std::to_string(state.character.level) + " " +
                          std::string(character::raceInfo(state.character.race).name) + " " +
                          std::string(character::classInfo(state.character.charClass).name),
                      sf::Color::White);
            drawLine("HP: " + std::to_string(state.character.currentHp) + "/" +
                          std::to_string(state.character.maxHp),
                      sf::Color(220, 90, 90));
            drawLine(formatDayTime(state.hoursElapsed), sf::Color(200, 200, 140));
            lineY += lineHeight * 0.5f;
            drawLine("-- Log --", sf::Color(140, 140, 160));

            const std::size_t maxLineChars =
                static_cast<std::size_t>((sidebarWidth - 32.f) / kSidebarCharWidth);
            for (const std::string& entry : log) {
                for (const std::string& wrapped : wrapToWidth(entry, maxLineChars)) {
                    drawLine(wrapped, sf::Color(190, 190, 200));
                }
            }

            window.display();
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

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: ansalon_sfml_phase1 <path-to-save-file>\n"
                     "  run from the repo root, e.g.:\n"
                     "  .\\build\\Debug\\ansalon_sfml_phase1.exe build\\Debug\\save1.txt\n"
                     "Read-only -- never writes back to the save file.\n";
        return 1;
    }
    try {
        return runPhase1(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "FATAL EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "FATAL UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
}
