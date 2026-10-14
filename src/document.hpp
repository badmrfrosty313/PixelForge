#pragma once
#include "level.hpp"
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>
namespace pf {
struct TileChange {
  int layer;
  int x;
  int y;
  int before;
  int after;
};
struct CollisionChange {
  int x, y;
  bool before, after;
};
class Document {
  struct Command {
    std::vector<TileChange> tiles;
    std::vector<Layer> before_layers;
    std::vector<Layer> after_layers;
    std::vector<CollisionChange> collision;
    std::vector<Entity> before_entities, after_entities;
    std::string before_tileset, after_tileset;
    int before_tile_width = 24, before_tile_height = 24,
        before_atlas_width = 16, before_atlas_height = 16;
    int after_tile_width = 24, after_tile_height = 24, after_atlas_width = 16,
        after_atlas_height = 16;
    std::uint64_t before_state = 0, after_state = 0;
    std::size_t bytes = 0;
    bool changes_layers = false;
    bool changes_collision = false, changes_entities = false,
         changes_assets = false;
  };
  Level level_;
  std::vector<Command> history_;
  std::size_t cursor_ = 0, history_bytes_ = 0;
  static constexpr std::size_t max_history_commands = 64;
  static constexpr std::size_t max_history_bytes = 16 * 1024 * 1024;
  std::uint64_t state_ = 0, saved_state_ = 0, next_state_ = 1;
  std::uint64_t next_entity_id_ = 1;
  bool recording_stroke_ = false;
  std::vector<TileChange> stroke_;
  std::unordered_map<std::uint64_t, std::size_t> stroke_indices_;
  bool recording_collision_stroke_ = false;
  std::vector<CollisionChange> collision_stroke_;
  std::unordered_map<std::uint64_t, std::size_t> collision_indices_;
  void commit_tiles(std::vector<TileChange> changes);
  void commit_layers(std::vector<Layer> before);
  void commit_entities(std::vector<Entity> before);
  void commit_asset_settings(const Level &before);
  void commit(Command command);

public:
  explicit Document(Level level = Level{});
  const Level &level() const noexcept { return level_; }
  bool dirty() const noexcept { return state_ != saved_state_; }
  void mark_saved() noexcept { saved_state_ = state_; }
  bool can_undo() const noexcept { return cursor_ > 0; }
  bool can_redo() const noexcept { return cursor_ < history_.size(); }
  void undo();
  void redo();
  void begin_stroke();
  void paint_line(int layer, int x0, int y0, int x1, int y1, int tile);
  void end_stroke();
  void paint(int layer, int x, int y, int tile);
  void rectangle(int layer, int x0, int y0, int x1, int y1, int tile);
  void line(int layer, int x0, int y0, int x1, int y1, int tile);
  void flood_fill(int layer, int x, int y, int tile);
  int tile(int layer, int x, int y) const { return level_.tile(layer, x, y); }
  void create_layer(std::string name);
  void rename_layer(int index, std::string name);
  void delete_layer(int index);
  void move_layer(int from, int to);
  void set_layer_visible(int index, bool visible);
  void set_layer_locked(int index, bool locked);
  bool solid(int x, int y) const { return level_.solid(x, y); }
  void set_solid(int x, int y, bool value);
  void begin_collision_stroke();
  void paint_collision_line(int x0, int y0, int x1, int y1, bool value);
  void end_collision_stroke();
  void set_collision_line(int x0, int y0, int x1, int y1, bool value);
  std::uint64_t add_entity(EntityType type, float x, float y);
  void update_entity(Entity entity);
  void delete_entity(std::uint64_t id);
  void set_asset_settings(std::string path, int tile_width, int tile_height,
                          int atlas_width, int atlas_height);
};
} // namespace pf
