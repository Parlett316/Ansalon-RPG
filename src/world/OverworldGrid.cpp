#include "world/OverworldGrid.h"

#include <fstream>
#include <stdexcept>

namespace world {

OverworldGrid::OverworldGrid(std::vector<std::string> rows, int width, int height)
    : rows_(std::move(rows)), width_(width), height_(height) {}

OverworldGrid OverworldGrid::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open overworld grid file: " + path);
    }

    std::vector<std::string> rows;
    std::string line;
    int width = -1;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back(); // tolerate CRLF
        if (line.empty()) continue;
        if (width == -1) {
            width = static_cast<int>(line.size());
        } else if (static_cast<int>(line.size()) != width) {
            throw std::runtime_error("Overworld grid file has a ragged row (expected width " +
                                      std::to_string(width) + ", got " + std::to_string(line.size()) + ")");
        }
        rows.push_back(std::move(line));
    }
    if (rows.empty()) {
        throw std::runtime_error("Overworld grid file is empty: " + path);
    }

    return OverworldGrid(std::move(rows), width, static_cast<int>(rows.size()));
}

char OverworldGrid::terrainCodeAt(int x, int y) const {
    if (x < 0 || y < 0 || y >= height_ || x >= width_) return '?';
    return rows_[static_cast<size_t>(y)][static_cast<size_t>(x)];
}

} // namespace world
