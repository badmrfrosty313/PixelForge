#include "level.hpp"
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <unordered_set>
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
std::string entity_type_name(EntityType t) {
  switch (t) {
  case EntityType::player_spawn:
    return "player_spawn";
  case EntityType::npc:
    return "npc";
  case EntityType::enemy:
    return "enemy";
  case EntityType::collectible:
    return "collectible";
  case EntityType::trigger:
    return "trigger";
  case EntityType::checkpoint:
    return "checkpoint";
  case EntityType::hazard:
    return "hazard";
  }
  throw std::invalid_argument("Unknown entity type");
}
EntityType parse_entity_type(const std::string &s) {
  if (s == "player_spawn")
    return EntityType::player_spawn;
  if (s == "npc")
    return EntityType::npc;
  if (s == "enemy")
    return EntityType::enemy;
  if (s == "collectible")
    return EntityType::collectible;
  if (s == "trigger")
    return EntityType::trigger;
  if (s == "checkpoint")
    return EntityType::checkpoint;
  if (s == "hazard")
    return EntityType::hazard;
  throw std::invalid_argument("Unknown entity type: " + s);
}
Level::Level(Genre g, int w, int h) : genre(g), width(w), height(h) {
  if (w < 1 || h < 1 || w > max_dimension || h > max_dimension)
    throw std::invalid_argument("Dimensions must be 1..256");
  layers.push_back({"Ground", std::vector<int>(w * h, 0)});
  collision.resize(static_cast<std::size_t>(w * h), 0);
  validate();
}
bool Level::solid(int x, int y) const {
  if (x < 0 || y < 0 || x >= width || y >= height)
    return true;
  return collision.at(static_cast<std::size_t>(y * width + x)) != 0;
}
void Level::set_solid(int x, int y, bool value) {
  if (x < 0 || y < 0 || x >= width || y >= height)
    throw std::out_of_range("Collision coordinates");
  collision.at(static_cast<std::size_t>(y * width + x)) = value ? 1 : 0;
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
  if (tile_width < 1 || tile_width > 512 || tile_height < 1 ||
      tile_height > 512)
    throw std::invalid_argument("Tile size must be 1..512 pixels");
  if (atlas_tile_width < 1 || atlas_tile_width > 512 || atlas_tile_height < 1 ||
      atlas_tile_height > 512)
    throw std::invalid_argument("Tileset tile size must be 1..512 pixels");
  if (tileset_path.empty() || tileset_path.size() > 260 ||
      tileset_path.find('\0') != std::string::npos ||
      tileset_path.find('\\') != std::string::npos ||
      tileset_path.find(':') != std::string::npos)
    throw std::invalid_argument(
        "Tileset reference must be a safe relative path");
  const std::filesystem::path asset(tileset_path);
  // is_absolute() is platform-specific: on Windows a POSIX-rooted path such
  // as /tmp/atlas.png is rooted but not absolute. Reject every rooted form.
  if (asset.has_root_path() || asset.is_absolute())
    throw std::invalid_argument("Tileset reference must be relative");
  for (const auto &part : asset)
    if (part == ".." || part == ".")
      throw std::invalid_argument(
          "Tileset reference cannot traverse directories");
  if (collision.size() != static_cast<std::size_t>(width * height))
    throw std::invalid_argument("Collision cell count mismatch");
  for (auto value : collision)
    if (value > 1)
      throw std::invalid_argument("Collision values must be zero or one");
  if (entities.size() > 4096)
    throw std::invalid_argument("Entity limit is 4096");
  std::unordered_set<std::uint64_t> ids;
  for (const auto &entity : entities) {
    entity_type_name(entity.type);
    if (entity.id == 0 || !ids.insert(entity.id).second)
      throw std::invalid_argument("Entity IDs must be nonzero and unique");
    if (!std::isfinite(entity.x) || !std::isfinite(entity.y) || entity.x < 0 ||
        entity.y < 0 || entity.x >= width || entity.y >= height)
      throw std::invalid_argument("Entity position is outside the level");
    if (entity.properties.size() > 32)
      throw std::invalid_argument("Entity property limit is 32");
    for (const auto &[name, value] : entity.properties) {
      if (name.empty() || name.size() > 64)
        throw std::invalid_argument("Invalid entity property name");
      const auto encoded = value.dump();
      if (encoded.size() > 4096)
        throw std::invalid_argument("Entity property exceeds 4 KiB");
    }
  }
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
