#include "world/OverworldGrid.h"

#include <fstream>
#include <stdexcept>

namespace world {

namespace {

// Shared row-file parsing for both the terrain grid and the region-layer
// grid -- same format, same ragged-row/empty-file fail-fast checks.
std::vector<std::string> loadRows(const std::string& path) {
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
                                      std::to_string(width) + ", got " + std::to_string(line.size()) + "): " + path);
        }
        rows.push_back(std::move(line));
    }
    if (rows.empty()) {
        throw std::runtime_error("Overworld grid file is empty: " + path);
    }
    return rows;
}

} // namespace

OverworldGrid::OverworldGrid(std::vector<std::string> rows, std::vector<std::string> regionRows, int width,
                              int height)
    : rows_(std::move(rows)), regionRows_(std::move(regionRows)), width_(width), height_(height) {}

OverworldGrid OverworldGrid::loadFromFile(const std::string& path, const std::string& regionsPath) {
    std::vector<std::string> rows = loadRows(path);
    std::vector<std::string> regionRows = loadRows(regionsPath);

    int width = static_cast<int>(rows[0].size());
    int height = static_cast<int>(rows.size());
    if (regionRows.size() != rows.size() || regionRows[0].size() != rows[0].size()) {
        throw std::runtime_error("Overworld region grid dimensions (" + std::to_string(regionRows[0].size()) + "x" +
                                  std::to_string(regionRows.size()) + ") don't match the terrain grid (" +
                                  std::to_string(width) + "x" + std::to_string(height) + "): " + regionsPath);
    }

    return OverworldGrid(std::move(rows), std::move(regionRows), width, height);
}

char OverworldGrid::terrainCodeAt(int x, int y) const {
    if (x < 0 || y < 0 || y >= height_ || x >= width_) return '?';
    return rows_[static_cast<size_t>(y)][static_cast<size_t>(x)];
}

char OverworldGrid::regionCodeAt(int x, int y) const {
    if (x < 0 || y < 0 || y >= height_ || x >= width_) return '?';
    return regionRows_[static_cast<size_t>(y)][static_cast<size_t>(x)];
}

} // namespace world
