#include "game/GameLoop.h"
#include "game/Command.h"
#include "render/MapRenderer.h"

#include <iostream>

namespace game {

GameLoop::GameLoop(const world::World& world, GameState initialState)
    : world_(world), state_(std::move(initialState)) {}

void GameLoop::run() {
    render::MapRenderer::drawLocationScene(world_, state_);
    std::cout << "\nType 'help' for a list of commands.\n";

    std::string line;
    for (;;) {
        std::cout << "\n> ";
        if (!std::getline(std::cin, line)) {
            // EOF (input redirected/piped and exhausted) ends the game
            // gracefully instead of spinning on a failed read.
            std::cout << "\n";
            break;
        }

        ParsedCommand cmd = parseCommand(line);
        switch (cmd.verb) {
            case Verb::Look:
                render::MapRenderer::drawLocationScene(world_, state_);
                break;
            case Verb::Map:
                render::MapRenderer::drawSchematic(world_, state_);
                break;
            case Verb::Go:
                handleGo(cmd.argument);
                break;
            case Verb::Help:
                printHelp();
                break;
            case Verb::Quit:
                std::cout << "Farewell, traveler.\n";
                return;
            case Verb::Unknown:
            default:
                std::cout << "I don't understand that. Type 'help' for commands.\n";
                break;
        }
    }
}

void GameLoop::handleGo(const std::string& destinationName) {
    if (destinationName.empty()) {
        std::cout << "Go where? Try: go <place name>\n";
        return;
    }

    // Safe to dereference without a null check: state_.currentLocationId is
    // seeded in main.cpp from an id validated to exist in World, and is only
    // ever reassigned below to dest->id, which World::findByName guarantees
    // is a real location. See docs/GOTCHAS.md.
    const world::Location* current = world_.getLocation(state_.currentLocationId);

    const world::Location* dest = world_.findByName(destinationName);
    if (!dest) {
        std::cout << "You've never heard of a place called \"" << destinationName << "\".\n";
        return;
    }

    const world::Connection* road = nullptr;
    for (const auto& conn : current->connections) {
        if (conn.targetId == dest->id) {
            road = &conn;
            break;
        }
    }
    if (!road) {
        std::cout << "There is no direct road from " << current->name << " to " << dest->name << ".\n";
        return;
    }

    std::cout << "\nYou travel " << road->roadDescription << ".\n";
    state_.dayCount += road->travelDays;
    state_.currentLocationId = dest->id;
    state_.visitedLocations.insert(dest->id);
    std::cout << "(" << road->travelDays << " day" << (road->travelDays == 1 ? "" : "s")
               << " pass" << (road->travelDays == 1 ? "es" : "") << ". It is now day "
               << state_.dayCount << ".)\n";

    render::MapRenderer::drawLocationScene(world_, state_);
}

void GameLoop::printHelp() const {
    std::cout <<
        "\nCommands:\n"
        "  look            - describe where you are and list roads onward\n"
        "  map             - show the schematic map of Ansalon\n"
        "  go <place>      - travel to a directly connected place, e.g. 'go tarsis'\n"
        "  help            - show this list\n"
        "  quit            - leave the game\n";
}

} // namespace game
