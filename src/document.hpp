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
class Document {
  struct Command {
    std::vector<TileChange> tiles;
    std::vector<Layer> before_layers;
    std::vector<Layer> after_layers;
    std::uint64_t before_state = 0, after_state = 0;
    std::size_t bytes = 0;
    bool changes_layers = false;
  };
  Level level_;
  std::vector<Command> history_;
  std::size_t cursor_ = 0, history_bytes_ = 0;
  static constexpr std::size_t max_history_commands = 64;
  static constexpr std::size_t max_history_bytes = 16 * 1024 * 1024;
  std::uint64_t state_ = 0, saved_state_ = 0, next_state_ = 1;
  bool recording_stroke_ = false;
  std::vector<TileChange> stroke_;
  std::unordered_map<std::uint64_t, std::size_t> stroke_indices_;
  void commit_tiles(std::vector<TileChange> changes);
  void commit_layers(std::vector<Layer> before);
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
};
} // namespace pf
