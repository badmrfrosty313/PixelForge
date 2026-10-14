#pragma once
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
namespace pf {
enum class Genre { top_down_rpg, platformer };
std::string genre_name(Genre genre);
Genre parse_genre(const std::string &name);
enum class EntityType {
  player_spawn,
  npc,
  enemy,
  collectible,
  trigger,
  checkpoint,
  hazard
};
std::string entity_type_name(EntityType type);
EntityType parse_entity_type(const std::string &name);
struct Entity {
  std::uint64_t id = 0;
  EntityType type = EntityType::npc;
  float x = 0, y = 0;
  std::map<std::string, nlohmann::json> properties;
  bool operator==(const Entity &) const = default;
};
struct Layer {
  std::string name;
  std::vector<int> tiles;
  bool visible = true;
  bool locked = false;
  bool operator==(const Layer &) const = default;
};
class Level {
public:
  static constexpr int max_dimension = 256;
  Genre genre;
  int width, height;
  int tile_width = 24, tile_height = 24;
  std::string tileset_path = "assets/studio_tiles.png";
  int atlas_tile_width = 16, atlas_tile_height = 16;
  std::vector<Layer> layers;
  std::vector<std::uint8_t> collision;
  std::vector<Entity> entities;
  Level(Genre genre = Genre::top_down_rpg, int width = 32, int height = 24);
  int tile(int layer, int x, int y) const;
  void paint(int layer, int x, int y, int value);
  bool solid(int x, int y) const;
  void set_solid(int x, int y, bool value);
  void validate() const;
  bool operator==(const Level &) const = default;

private:
  std::size_t index(int layer, int x, int y) const;
};
} // namespace pf
