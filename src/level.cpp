#include "level.hpp"
#include <stdexcept>
namespace pf {
std::string genre_name(Genre g) {
  switch (g) {
  case Genre::top_down_rpg:
    return "top_down_rpg";
  case Genre::platformer:
    return "platformer";
  }
  throw std::invalid_argument("Unknown genre");
}
Genre parse_genre(const std::string &s) {
  if (s == "top_down_rpg")
    return Genre::top_down_rpg;
  if (s == "platformer")
    return Genre::platformer;
  throw std::invalid_argument("Unknown genre: " + s);
}
Level::Level(Genre g, int w, int h) : genre(g), width(w), height(h) {
  if (w < 1 || h < 1 || w > max_dimension || h > max_dimension)
    throw std::invalid_argument("Dimensions must be 1..256");
  layers.push_back({"Ground", std::vector<int>(w * h, 0)});
  validate();
}
std::size_t Level::index(int l, int x, int y) const {
  if (l < 0 || l >= static_cast<int>(layers.size()) || x < 0 || y < 0 ||
      x >= width || y >= height)
    throw std::out_of_range("Tile coordinates");
  return static_cast<std::size_t>(y * width + x);
}
int Level::tile(int l, int x, int y) const {
  return layers.at(l).tiles.at(index(l, x, y));
}
void Level::paint(int l, int x, int y, int v) {
  if (v < 0 || v > 255)
    throw std::invalid_argument("Tile ID must be 0..255");
  layers.at(l).tiles.at(index(l, x, y)) = v;
}
void Level::validate() const {
  genre_name(genre);
  if (width < 1 || height < 1 || width > max_dimension ||
      height > max_dimension || layers.empty() || layers.size() > 16)
    throw std::invalid_argument("Invalid dimensions/layers");
  for (const auto &l : layers) {
    if (l.name.empty() || l.name.size() > 128 ||
        l.tiles.size() != static_cast<std::size_t>(width * height))
      throw std::invalid_argument("Invalid layer");
    for (int t : l.tiles)
      if (t < 0 || t > 255)
        throw std::invalid_argument("Invalid tile ID");
  }
}
} // namespace pf
