// Full-migration Phase 1+2 -- see docs/CURRENT_WORK.md and the plans this
// was built from. A real, pixel-space overworld + zone-interior screen
// driven by real save data: proves the rendering/collision approach end to
// end before combat/menus are attempted. Deliberately standalone rather
// than reusing game::GameLoop::run() -- see this file's CMakeLists.txt
// comment for why. Loads a save file read-only via game::SaveGame --
// never writes back, never touches the real ansalon_rpg target's code
// path.

#include "character/CharClass.h"
#include "character/Race.h"
#include "game/GameState.h"
#include "game/SaveGame.h"
#include "world/OverworldGrid.h"
#include "world/Terrain.h"
#include "world/World.h"
#include "world/WorldLoader.h"
#include "world/Zone.h"
#include "world/ZoneCatalog.h"
#include "world/ZoneTile.h"

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
// Placeholder pixel-per-tile scale for zone interiors -- there's no real
// reference image for these like the overworld has, so this is a plain
// colored-tile placeholder, not art. Zones are capped at 44x16
// (docs/ZONE_NOTES.md), so at this scale every zone fits the map viewport
// with no scrolling needed.
constexpr float kZoneTilePx = 20.f;

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

// Placeholder fill color per zone tile code -- plain colored rectangles,
// not art (see kZoneTilePx above). POI/portal tiles hold their own POI
// letter code in the grid (not '.'), so they fall through to default here
// too; ZoneLoader::loadFromFile already fails fast unless every grid
// character is either one of the codes below or a declared POI, so
// `default` is unreachable for any zone that loaded at all -- just a
// plain-floor fallback, not a "spot the gap" signal. The POI's own icon
// (see PoiKind/drawPoiIcon below) is what actually distinguishes it.
sf::Color colorForZoneTile(char code) {
    switch (code) {
        case '.': return sf::Color(200, 190, 160); // open ground
        case '%': return sf::Color(55, 120, 55);   // vallenwood / dense growth
        case '#': return sf::Color(70, 70, 78);    // wall
        case '~': return sf::Color(60, 100, 180);  // water
        case '+': return sf::Color(120, 85, 50);   // doorway
        default: return sf::Color(200, 190, 160);  // POI/portal tile -- same as open ground
    }
}

// What a POI tile visually is, for icon selection -- computed at draw
// time from data already on world::PointOfInterest/world::Zone, not a
// stored field. Order matters only in the (currently never-hit, per a
// grep across every data/zones/*.txt) case where a POI is more than one
// of these at once.
enum class PoiKind { Door, Shop, Bed, Person, Landmark };

PoiKind poiKindFor(bool isPortal, const world::PointOfInterest& poi) {
    if (isPortal) return PoiKind::Door;
    if (poi.isShop) return PoiKind::Shop;
    if (poi.isBed) return PoiKind::Bed;
    if (!poi.dialogue.empty()) return PoiKind::Person;
    return PoiKind::Landmark;
}

// Small reusable shape pool for drawPoiIcon -- declared once and mutated
// per cell, same "declare outside the loop" convention as zoneTileShape/
// locationMarker below. Each icon is 1-2 primitives so the placeholder
// reads as a distinct shape, not just a distinct color.
struct PoiIconShapes {
    sf::RectangleShape rectA;
    sf::RectangleShape rectB;
    sf::ConvexShape triangle;
    sf::ConvexShape diamond;
    sf::CircleShape circle;

    explicit PoiIconShapes(float tilePx) : circle(tilePx * 0.22f) {
        triangle.setPointCount(3);
        diamond.setPointCount(4);
        circle.setOrigin(sf::Vector2f(tilePx * 0.22f, tilePx * 0.22f));
    }
};

// Draws a 1-2 primitive placeholder icon for `kind`, centered on
// (centerX, centerY). No image assets exist for zone interiors yet (see
// docs/ARCHITECTURE.md's SFML section) -- this is placeholder shape/color
// vocabulary, ready to swap for real sprite art later.
void drawPoiIcon(sf::RenderWindow& window, PoiIconShapes& shapes, PoiKind kind, float tilePx, float centerX,
                  float centerY) {
    switch (kind) {
        case PoiKind::Door: {
            const float frameW = tilePx * 0.5f;
            const float frameH = tilePx * 0.8f;
            shapes.rectA.setSize(sf::Vector2f(frameW, frameH));
            shapes.rectA.setOrigin(sf::Vector2f(frameW / 2.f, frameH / 2.f));
            shapes.rectA.setPosition(sf::Vector2f(centerX, centerY));
            shapes.rectA.setFillColor(sf::Color(101, 67, 33)); // wood frame
            window.draw(shapes.rectA);

            const float openW = frameW * 0.55f;
            const float openH = frameH * 0.7f;
            shapes.rectB.setSize(sf::Vector2f(openW, openH));
            shapes.rectB.setOrigin(sf::Vector2f(openW / 2.f, openH / 2.f));
            shapes.rectB.setPosition(sf::Vector2f(centerX, centerY + frameH * 0.06f));
            shapes.rectB.setFillColor(sf::Color(235, 205, 150)); // lit opening
            window.draw(shapes.rectB);
            break;
        }
        case PoiKind::Shop: {
            const float counterW = tilePx * 0.7f;
            const float counterH = tilePx * 0.35f;
            shapes.rectA.setSize(sf::Vector2f(counterW, counterH));
            shapes.rectA.setOrigin(sf::Vector2f(counterW / 2.f, counterH / 2.f));
            shapes.rectA.setPosition(sf::Vector2f(centerX, centerY + tilePx * 0.2f));
            shapes.rectA.setFillColor(sf::Color(150, 130, 100)); // counter
            window.draw(shapes.rectA);

            const float awningW = tilePx * 0.8f;
            const float awningH = tilePx * 0.3f;
            shapes.triangle.setPoint(0, sf::Vector2f(-awningW / 2.f, 0.f));
            shapes.triangle.setPoint(1, sf::Vector2f(awningW / 2.f, 0.f));
            shapes.triangle.setPoint(2, sf::Vector2f(0.f, -awningH));
            shapes.triangle.setPosition(sf::Vector2f(centerX, centerY - tilePx * 0.05f));
            shapes.triangle.setFillColor(sf::Color(70, 150, 150)); // awning
            window.draw(shapes.triangle);
            break;
        }
        case PoiKind::Bed: {
            const float mattressW = tilePx * 0.75f;
            const float mattressH = tilePx * 0.4f;
            shapes.rectA.setSize(sf::Vector2f(mattressW, mattressH));
            shapes.rectA.setOrigin(sf::Vector2f(mattressW / 2.f, mattressH / 2.f));
            shapes.rectA.setPosition(sf::Vector2f(centerX, centerY));
            shapes.rectA.setFillColor(sf::Color(180, 160, 210)); // mattress
            window.draw(shapes.rectA);

            const float pillowW = mattressW * 0.3f;
            const float pillowH = mattressH * 0.7f;
            shapes.rectB.setSize(sf::Vector2f(pillowW, pillowH));
            shapes.rectB.setOrigin(sf::Vector2f(pillowW / 2.f, pillowH / 2.f));
            shapes.rectB.setPosition(sf::Vector2f(centerX - mattressW / 2.f + pillowW / 2.f + 2.f, centerY));
            shapes.rectB.setFillColor(sf::Color(230, 225, 240)); // pillow
            window.draw(shapes.rectB);
            break;
        }
        case PoiKind::Person: {
            shapes.circle.setPosition(sf::Vector2f(centerX, centerY - tilePx * 0.2f));
            shapes.circle.setFillColor(sf::Color(230, 180, 90)); // head
            window.draw(shapes.circle);

            const float bodyBottomW = tilePx * 0.32f;
            const float bodyH = tilePx * 0.4f;
            shapes.triangle.setPoint(0, sf::Vector2f(0.f, 0.f));
            shapes.triangle.setPoint(1, sf::Vector2f(-bodyBottomW, bodyH));
            shapes.triangle.setPoint(2, sf::Vector2f(bodyBottomW, bodyH));
            shapes.triangle.setPosition(sf::Vector2f(centerX, centerY));
            shapes.triangle.setFillColor(sf::Color(190, 130, 60)); // body/robe
            window.draw(shapes.triangle);
            break;
        }
        case PoiKind::Landmark: {
            const float r = tilePx * 0.28f;
            shapes.diamond.setPoint(0, sf::Vector2f(0.f, -r));
            shapes.diamond.setPoint(1, sf::Vector2f(r, 0.f));
            shapes.diamond.setPoint(2, sf::Vector2f(0.f, r));
            shapes.diamond.setPoint(3, sf::Vector2f(-r, 0.f));
            shapes.diamond.setPosition(sf::Vector2f(centerX, centerY));
            shapes.diamond.setFillColor(sf::Color(110, 130, 90)); // quiet scenery marker
            window.draw(shapes.diamond);
            break;
        }
    }
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

    world::ZoneCatalog zones = world::ZoneCatalog::loadForWorld(world, "data/zones");
    std::cout << "step 2b: zones loaded, " << zones.allZones().size() << " zones" << std::endl;

    game::GameState state = game::SaveGame::load(savePath);
    std::cout << "step 3: save loaded -- " << state.character.name << ", level "
              << state.character.level << " " << character::raceInfo(state.character.race).name << " "
              << character::classInfo(state.character.charClass).name << ", at (" << state.x << ", "
              << state.y << ")" << std::endl;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(windowW, windowH)),
                             "Ansalon SFML Phase 1+2 -- Real Overworld + Zones (WIP)");
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

    // Real GameState fields drive position in both modes (state.x/y for
    // Overworld, state.zoneX/zoneY for Zone) -- never written back to
    // savePath, but mutated in memory exactly like game::GameLoop does.
    const world::Zone* currentZone = nullptr;
    if (state.mode == game::Mode::Zone) {
        currentZone = zones.getZone(state.currentZoneId);
        if (!currentZone) {
            std::cerr << "Warning: save's zone '" << state.currentZoneId
                      << "' not found -- falling back to overworld.\n";
            state.mode = game::Mode::Overworld;
        }
    }

    sf::View mapView(sf::Vector2f(0.f, 0.f), sf::Vector2f(mapWidth, static_cast<float>(windowH)));
    mapView.setViewport(sf::FloatRect({0.f, 0.f}, {mapWidth / static_cast<float>(windowW), 1.f}));

    const sf::View uiView = window.getDefaultView();

    sf::Sprite mapSprite(mapTexture);

    sf::CircleShape locationMarker(6.f);
    locationMarker.setOrigin(sf::Vector2f(6.f, 6.f));

    sf::CircleShape playerMarker(9.f);
    playerMarker.setOrigin(sf::Vector2f(9.f, 9.f));
    playerMarker.setFillColor(sf::Color(255, 215, 0));

    sf::RectangleShape zoneTileShape(sf::Vector2f(kZoneTilePx, kZoneTilePx));

    PoiIconShapes poiIconShapes(kZoneTilePx);

    sf::CircleShape entryMarker(kZoneTilePx * 0.35f);
    entryMarker.setOrigin(sf::Vector2f(kZoneTilePx * 0.35f, kZoneTilePx * 0.35f));
    entryMarker.setFillColor(sf::Color(60, 200, 220));

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
    if (currentZone) {
        pushLog("Resumed inside " + currentZone->name() + ".");
    }
    pushLog("Movement + Enter (zones) work. Other keys are placeholders for now.");

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
                    bool handleEnter = false;
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
                        case sf::Keyboard::Key::Enter: handleEnter = true; break;
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

                    if (handleEnter) {
                        if (state.mode == game::Mode::Overworld) {
                            const world::Location* here = world.locationAt(state.x, state.y);
                            const world::Zone* zone = here ? zones.getZone(here->id) : nullptr;
                            if (zone) {
                                state.mode = game::Mode::Zone;
                                state.currentZoneId = here->id;
                                state.zoneX = zone->entryX();
                                state.zoneY = zone->entryY();
                                currentZone = zone;
                                pushLog("You step into " + zone->name() + ".");
                            } else {
                                pushLog("There's nothing to enter here.");
                            }
                        } else if (currentZone) {
                            if (const std::string* portalTarget = currentZone->portalAt(state.zoneX, state.zoneY)) {
                                const world::Zone* target = zones.getZone(*portalTarget);
                                if (target) {
                                    state.zoneStack.push_back({state.currentZoneId, state.zoneX, state.zoneY});
                                    state.currentZoneId = *portalTarget;
                                    state.zoneX = target->entryX();
                                    state.zoneY = target->entryY();
                                    currentZone = target;
                                    pushLog("You step into " + target->name() + ".");
                                } else {
                                    pushLog("That doorway doesn't lead anywhere in this build.");
                                }
                            } else if (state.zoneX == currentZone->entryX() &&
                                       state.zoneY == currentZone->entryY()) {
                                if (!state.zoneStack.empty()) {
                                    const game::ZoneReturnPoint back = state.zoneStack.back();
                                    state.zoneStack.pop_back();
                                    state.currentZoneId = back.zoneId;
                                    state.zoneX = back.x;
                                    state.zoneY = back.y;
                                    currentZone = zones.getZone(state.currentZoneId);
                                    pushLog("You step back out into " +
                                            (currentZone ? currentZone->name() : back.zoneId) + ".");
                                } else {
                                    pushLog("You step back outside.");
                                    state.mode = game::Mode::Overworld;
                                    currentZone = nullptr;
                                }
                            } else {
                                pushLog("Nothing to step through here.");
                            }
                        }
                    } else if (!placeholder.empty()) {
                        pushLog(placeholder);
                    } else if (dx != 0 || dy != 0) {
                        if (state.mode == game::Mode::Overworld) {
                            const int nx = state.x + dx;
                            const int ny = state.y + dy;
                            const world::TerrainInfo& terrain = world::terrainFor(grid.terrainCodeAt(nx, ny));
                            if (terrain.passable) {
                                state.x = nx;
                                state.y = ny;
                                if (const world::Location* here = world.locationAt(state.x, state.y)) {
                                    pushLog("Arrived at " + here->name + ".");
                                }
                            } else {
                                pushLog("Blocked: cannot walk onto " + std::string(terrain.name) + ".");
                            }
                        } else if (currentZone) {
                            const int nx = state.zoneX + dx;
                            const int ny = state.zoneY + dy;
                            const bool isPoi = currentZone->poiAt(nx, ny) != nullptr;
                            const world::ZoneTileInfo& tile = world::zoneTileFor(currentZone->tileCodeAt(nx, ny));
                            if (isPoi || tile.passable) {
                                state.zoneX = nx;
                                state.zoneY = ny;
                                if (const world::PointOfInterest* poi = currentZone->poiAt(state.zoneX, state.zoneY)) {
                                    pushLog("Here: " + poi->name + ".");
                                }
                            } else {
                                pushLog("Blocked: cannot walk onto " + std::string(tile.name) + ".");
                            }
                        }
                    }
                }
            }

            window.clear(sf::Color::Black);
            window.setView(mapView);

            if (state.mode == game::Mode::Overworld) {
                const float playerPxX = (static_cast<float>(state.x) + 0.5f) * pxPerTileX;
                const float playerPxY = (static_cast<float>(state.y) + 0.5f) * pxPerTileY;

                const sf::Vector2f viewSize = mapView.getSize();
                const float halfW = viewSize.x / 2.f;
                const float halfH = viewSize.y / 2.f;
                const float camX = std::clamp(playerPxX, halfW, std::max(halfW, worldW - halfW));
                const float camY = std::clamp(playerPxY, halfH, std::max(halfH, worldH - halfH));
                mapView.setCenter(sf::Vector2f(camX, camY));
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
            } else if (currentZone) {
                // Whole zone always fits the viewport (see kZoneTilePx) --
                // centered, not scrolled, unlike the overworld.
                const float zonePxW = static_cast<float>(currentZone->width()) * kZoneTilePx;
                const float zonePxH = static_cast<float>(currentZone->height()) * kZoneTilePx;
                mapView.setCenter(sf::Vector2f(zonePxW / 2.f, zonePxH / 2.f));
                window.setView(mapView);

                constexpr unsigned kPoiLabelCharSize = 11;
                for (int zy = 0; zy < currentZone->height(); ++zy) {
                    for (int zx = 0; zx < currentZone->width(); ++zx) {
                        zoneTileShape.setFillColor(colorForZoneTile(currentZone->tileCodeAt(zx, zy)));
                        zoneTileShape.setPosition(
                            sf::Vector2f(static_cast<float>(zx) * kZoneTilePx, static_cast<float>(zy) * kZoneTilePx));
                        window.draw(zoneTileShape);

                        const float centerX = (static_cast<float>(zx) + 0.5f) * kZoneTilePx;
                        const float centerY = (static_cast<float>(zy) + 0.5f) * kZoneTilePx;
                        const std::string* portalTarget = currentZone->portalAt(zx, zy);
                        const world::PointOfInterest* poi = currentZone->poiAt(zx, zy);
                        if (poi) {
                            const PoiKind kind = poiKindFor(portalTarget != nullptr, *poi);
                            drawPoiIcon(window, poiIconShapes, kind, kZoneTilePx, centerX, centerY);
                        }
                        if (poi && !poi->name.empty()) {
                            sf::Text label(font, poi->name, kPoiLabelCharSize);
                            label.setFillColor(sf::Color(240, 240, 220));
                            label.setPosition(sf::Vector2f(centerX + kZoneTilePx * 0.4f, centerY - kZoneTilePx * 0.5f));
                            window.draw(label);
                        }
                        if (zx == currentZone->entryX() && zy == currentZone->entryY()) {
                            entryMarker.setPosition(sf::Vector2f(centerX, centerY));
                            window.draw(entryMarker);
                        }
                    }
                }

                const float playerPxX = (static_cast<float>(state.zoneX) + 0.5f) * kZoneTilePx;
                const float playerPxY = (static_cast<float>(state.zoneY) + 0.5f) * kZoneTilePx;
                playerMarker.setPosition(sf::Vector2f(playerPxX, playerPxY));
                window.draw(playerMarker);
            }

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
            if (state.mode == game::Mode::Zone && currentZone) {
                drawLine("Indoors -- " + currentZone->name(), sf::Color(150, 200, 230));
            }
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
