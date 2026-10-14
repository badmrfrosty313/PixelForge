#include "document.hpp"
#include <algorithm>
#include <cmath>
#include <deque>
#include <stdexcept>
#include <unordered_map>
#include <utility>
namespace pf {
namespace {
std::size_t layer_bytes(const std::vector<Layer> &layers) {
  std::size_t size = 0;
  for (const auto &layer : layers)
    size +=
        sizeof(Layer) + layer.name.size() + layer.tiles.size() * sizeof(int);
  return size;
}
std::uint64_t key(int layer, int x, int y) {
  return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(layer)) << 40) |
         (static_cast<std::uint64_t>(static_cast<std::uint32_t>(y)) << 20) |
         static_cast<std::uint32_t>(x);
}
template <class Fn> void bresenham(int x0, int y0, int x1, int y1, Fn fn) {
  int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;
  for (;;) {
    fn(x0, y0);
    if (x0 == x1 && y0 == y1)
      break;
    const int twice = 2 * error;
    if (twice >= dy) {
      error += dy;
      x0 += sx;
    }
    if (twice <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}
} // namespace
Document::Document(Level level) : level_(std::move(level)) {
  level_.validate();
  for (const auto &entity : level_.entities)
    if (entity.id >= next_entity_id_)
      next_entity_id_ = entity.id == UINT64_MAX ? 0 : entity.id + 1;
}
void Document::commit(Command command) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Cannot start another edit during a stroke");
  if (cursor_ < history_.size()) {
    for (std::size_t i = cursor_; i < history_.size(); ++i)
      history_bytes_ -= history_[i].bytes;
    history_.resize(cursor_);
  }
  command.before_state = state_;
  command.after_state = next_state_++;
  state_ = command.after_state;
  history_bytes_ += command.bytes;
  history_.push_back(std::move(command));
  cursor_ = history_.size();
  while (history_.size() > max_history_commands ||
         history_bytes_ > max_history_bytes) {
    if (history_.size() == 1) {
      history_bytes_ = 0;
      history_.clear();
      cursor_ = 0;
      saved_state_ = state_;
      break;
    }
    history_bytes_ -= history_.front().bytes;
    history_.erase(history_.begin());
    --cursor_;
  }
}
void Document::commit_tiles(std::vector<TileChange> changes) {
  if (changes.empty())
    return;
  Command command;
  command.tiles = std::move(changes);
  command.bytes = command.tiles.size() * sizeof(TileChange);
  commit(std::move(command));
}
void Document::commit_layers(std::vector<Layer> before) {
  if (before == level_.layers)
    return;
  Command command;
  command.changes_layers = true;
  command.before_layers = std::move(before);
  command.after_layers = level_.layers;
  command.bytes =
      layer_bytes(command.before_layers) + layer_bytes(command.after_layers);
  commit(std::move(command));
}
void Document::commit_entities(std::vector<Entity> before) {
  if (before == level_.entities)
    return;
  Command command;
  command.changes_entities = true;
  command.before_entities = std::move(before);
  command.after_entities = level_.entities;
  command.bytes =
      (command.before_entities.size() + command.after_entities.size()) *
      sizeof(Entity);
  for (const auto &entity : command.before_entities)
    for (const auto &[name, value] : entity.properties)
      command.bytes += name.size() + value.dump().size();
  for (const auto &entity : command.after_entities)
    for (const auto &[name, value] : entity.properties)
      command.bytes += name.size() + value.dump().size();
  commit(std::move(command));
}
void Document::commit_asset_settings(const Level &before) {
  Command command;
  command.changes_assets = true;
  command.before_tileset = before.tileset_path;
  command.after_tileset = level_.tileset_path;
  command.before_tile_width = before.tile_width;
  command.before_tile_height = before.tile_height;
  command.before_atlas_width = before.atlas_tile_width;
  command.before_atlas_height = before.atlas_tile_height;
  command.after_tile_width = level_.tile_width;
  command.after_tile_height = level_.tile_height;
  command.after_atlas_width = level_.atlas_tile_width;
  command.after_atlas_height = level_.atlas_tile_height;
  if (command.before_tileset == command.after_tileset &&
      command.before_tile_width == command.after_tile_width &&
      command.before_tile_height == command.after_tile_height &&
      command.before_atlas_width == command.after_atlas_width &&
      command.before_atlas_height == command.after_atlas_height)
    return;
  command.bytes = command.before_tileset.size() + command.after_tileset.size() +
                  sizeof(int) * 8;
  commit(std::move(command));
}
void Document::undo() {
  if (!can_undo())
    return;
  const auto &c = history_[cursor_ - 1];
  if (c.changes_layers)
    level_.layers = c.before_layers;
  else if (c.changes_collision)
    for (const auto &cell : c.collision)
      level_.set_solid(cell.x, cell.y, cell.before);
  else if (c.changes_entities)
    level_.entities = c.before_entities;
  else if (c.changes_assets) {
    level_.tileset_path = c.before_tileset;
    level_.tile_width = c.before_tile_width;
    level_.tile_height = c.before_tile_height;
    level_.atlas_tile_width = c.before_atlas_width;
    level_.atlas_tile_height = c.before_atlas_height;
  } else
    for (auto i = c.tiles.rbegin(); i != c.tiles.rend(); ++i)
      level_.paint(i->layer, i->x, i->y, i->before);
  state_ = c.before_state;
  --cursor_;
}
void Document::redo() {
  if (!can_redo())
    return;
  const auto &c = history_[cursor_];
  if (c.changes_layers)
    level_.layers = c.after_layers;
  else if (c.changes_collision)
    for (const auto &cell : c.collision)
      level_.set_solid(cell.x, cell.y, cell.after);
  else if (c.changes_entities)
    level_.entities = c.after_entities;
  else if (c.changes_assets) {
    level_.tileset_path = c.after_tileset;
    level_.tile_width = c.after_tile_width;
    level_.tile_height = c.after_tile_height;
    level_.atlas_tile_width = c.after_atlas_width;
    level_.atlas_tile_height = c.after_atlas_height;
  } else
    for (const auto &t : c.tiles)
      level_.paint(t.layer, t.x, t.y, t.after);
  state_ = c.after_state;
  ++cursor_;
}
void Document::begin_stroke() {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Stroke already active");
  recording_stroke_ = true;
  stroke_.clear();
  stroke_indices_.clear();
}
void Document::paint_line(int layer, int x0, int y0, int x1, int y1,
                          int value) {
  if (!recording_stroke_)
    throw std::logic_error("Stroke is not active");
  if (layer < 0 || layer >= static_cast<int>(level_.layers.size()))
    throw std::out_of_range("Layer index");
  if (level_.layers[layer].locked)
    return;
  if (value < 0 || value > 255)
    throw std::invalid_argument("Tile ID must be 0..255");
  bresenham(x0, y0, x1, y1, [&](int x, int y) {
    if (x < 0 || y < 0 || x >= level_.width || y >= level_.height)
      return;
    const int old = level_.tile(layer, x, y);
    const auto tile_key = key(layer, x, y);
    auto it = stroke_indices_.find(tile_key);
    if (it == stroke_indices_.end()) {
      if (old == value)
        return;
      stroke_indices_.emplace(tile_key, stroke_.size());
      stroke_.push_back({layer, x, y, old, value});
    } else if (stroke_[it->second].before == value) {
      const auto removed = it->second;
      stroke_indices_.erase(it);
      if (removed + 1 < stroke_.size()) {
        stroke_[removed] = stroke_.back();
        stroke_indices_[key(layer, stroke_[removed].x, stroke_[removed].y)] =
            removed;
      }
      stroke_.pop_back();
    } else
      stroke_[it->second].after = value;
    level_.paint(layer, x, y, value);
  });
}
void Document::end_stroke() {
  if (!recording_stroke_)
    return;
  recording_stroke_ = false;
  std::erase_if(stroke_, [](const auto &c) { return c.before == c.after; });
  auto changes = std::move(stroke_);
  stroke_.clear();
  stroke_indices_.clear();
  if (!changes.empty()) {
    Command command;
    command.tiles = std::move(changes);
    command.bytes = command.tiles.size() * sizeof(TileChange);
    commit(std::move(command));
  }
}
void Document::paint(int layer, int x, int y, int value) {
  begin_stroke();
  paint_line(layer, x, y, x, y, value);
  end_stroke();
}
void Document::line(int layer, int x0, int y0, int x1, int y1, int value) {
  begin_stroke();
  paint_line(layer, x0, y0, x1, y1, value);
  end_stroke();
}
void Document::rectangle(int layer, int x0, int y0, int x1, int y1, int value) {
  if (value < 0 || value > 255)
    throw std::invalid_argument("Tile ID must be 0..255");
  if (layer < 0 || layer >= static_cast<int>(level_.layers.size()))
    throw std::out_of_range("Layer index");
  const int left = std::min(x0, x1), right = std::max(x0, x1),
            top = std::min(y0, y1), bottom = std::max(y0, y1);
  std::vector<TileChange> changes;
  for (int y = std::max(0, top); y <= std::min(level_.height - 1, bottom); ++y)
    for (int x = std::max(0, left); x <= std::min(level_.width - 1, right);
         ++x) {
      const auto old = level_.tile(layer, x, y);
      if (old != value)
        changes.push_back({layer, x, y, old, value});
    }
  if (level_.layers[layer].locked)
    return;
  for (const auto &c : changes)
    level_.paint(c.layer, c.x, c.y, c.after);
  commit_tiles(std::move(changes));
}
void Document::flood_fill(int layer, int x, int y, int value) {
  if (value < 0 || value > 255)
    throw std::invalid_argument("Tile ID must be 0..255");
  if (layer < 0 || layer >= static_cast<int>(level_.layers.size()))
    throw std::out_of_range("Layer index");
  if (x < 0 || y < 0 || x >= level_.width || y >= level_.height)
    throw std::out_of_range("Tile coordinates");
  if (level_.layers[layer].locked)
    return;
  const int target = level_.tile(layer, x, y);
  if (target == value)
    return;
  std::vector<unsigned char> seen(
      static_cast<std::size_t>(level_.width * level_.height));
  std::deque<std::pair<int, int>> queue{{x, y}};
  std::vector<TileChange> changes;
  while (!queue.empty()) {
    auto [cx, cy] = queue.front();
    queue.pop_front();
    auto index = static_cast<std::size_t>(cy * level_.width + cx);
    if (seen[index])
      continue;
    seen[index] = 1;
    if (level_.tile(layer, cx, cy) != target)
      continue;
    changes.push_back({layer, cx, cy, target, value});
    if (cx > 0)
      queue.emplace_back(cx - 1, cy);
    if (cx + 1 < level_.width)
      queue.emplace_back(cx + 1, cy);
    if (cy > 0)
      queue.emplace_back(cx, cy - 1);
    if (cy + 1 < level_.height)
      queue.emplace_back(cx, cy + 1);
  }
  for (const auto &c : changes)
    level_.paint(c.layer, c.x, c.y, c.after);
  commit_tiles(std::move(changes));
}
void Document::create_layer(std::string name) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  auto before = level_.layers;
  level_.layers.push_back(
      {std::move(name),
       std::vector<int>(static_cast<std::size_t>(level_.width * level_.height),
                        0)});
  try {
    level_.validate();
  } catch (...) {
    level_.layers = std::move(before);
    throw;
  }
  commit_layers(std::move(before));
}
void Document::rename_layer(int i, std::string name) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  auto before = level_.layers;
  level_.layers.at(i).name = std::move(name);
  try {
    level_.validate();
  } catch (...) {
    level_.layers = std::move(before);
    throw;
  }
  commit_layers(std::move(before));
}
void Document::delete_layer(int i) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  if (i < 0 || i >= static_cast<int>(level_.layers.size()))
    throw std::out_of_range("Layer index");
  if (level_.layers.size() <= 1)
    throw std::invalid_argument("A level must keep at least one layer");
  auto before = level_.layers;
  level_.layers.erase(level_.layers.begin() + i);
  commit_layers(std::move(before));
}
void Document::move_layer(int from, int to) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  if (from < 0 || to < 0 || from >= static_cast<int>(level_.layers.size()) ||
      to >= static_cast<int>(level_.layers.size()))
    throw std::out_of_range("Layer index");
  if (from == to)
    return;
  auto before = level_.layers;
  auto item = std::move(level_.layers[from]);
  level_.layers.erase(level_.layers.begin() + from);
  level_.layers.insert(level_.layers.begin() + to, std::move(item));
  commit_layers(std::move(before));
}
void Document::set_layer_visible(int i, bool value) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  auto before = level_.layers;
  level_.layers.at(i).visible = value;
  commit_layers(std::move(before));
}
void Document::set_layer_locked(int i, bool value) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  auto before = level_.layers;
  level_.layers.at(i).locked = value;
  commit_layers(std::move(before));
}
void Document::set_solid(int x, int y, bool value) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  const bool before = level_.solid(x, y);
  if (before == value)
    return;
  level_.set_solid(x, y, value);
  Command command;
  command.changes_collision = true;
  command.collision.push_back({x, y, before, value});
  command.bytes = sizeof(CollisionChange);
  commit(std::move(command));
}
void Document::begin_collision_stroke() {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Another edit is active");
  recording_collision_stroke_ = true;
  collision_stroke_.clear();
  collision_indices_.clear();
}
void Document::paint_collision_line(int x0, int y0, int x1, int y1,
                                    bool value) {
  if (!recording_collision_stroke_)
    throw std::logic_error("Collision stroke is not active");
  bresenham(x0, y0, x1, y1, [&](int x, int y) {
    if (x < 0 || y < 0 || x >= level_.width || y >= level_.height)
      return;
    const bool old = level_.solid(x, y);
    const auto tile_key = key(0, x, y);
    auto found = collision_indices_.find(tile_key);
    if (found == collision_indices_.end()) {
      if (old == value)
        return;
      collision_indices_.emplace(tile_key, collision_stroke_.size());
      collision_stroke_.push_back({x, y, old, value});
    } else if (collision_stroke_[found->second].before == value) {
      const auto removed = found->second;
      collision_indices_.erase(found);
      if (removed + 1 < collision_stroke_.size()) {
        collision_stroke_[removed] = collision_stroke_.back();
        collision_indices_[key(0, collision_stroke_[removed].x,
                               collision_stroke_[removed].y)] = removed;
      }
      collision_stroke_.pop_back();
    } else
      collision_stroke_[found->second].after = value;
    level_.set_solid(x, y, value);
  });
}
void Document::end_collision_stroke() {
  if (!recording_collision_stroke_)
    return;
  recording_collision_stroke_ = false;
  collision_indices_.clear();
  if (collision_stroke_.empty())
    return;
  Command command;
  command.changes_collision = true;
  command.collision = std::move(collision_stroke_);
  collision_stroke_.clear();
  command.bytes = command.collision.size() * sizeof(CollisionChange);
  commit(std::move(command));
}
void Document::set_collision_line(int x0, int y0, int x1, int y1, bool value) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  std::unordered_map<std::uint64_t, std::size_t> indices;
  std::vector<CollisionChange> changes;
  bresenham(x0, y0, x1, y1, [&](int x, int y) {
    if (x < 0 || y < 0 || x >= level_.width || y >= level_.height)
      return;
    const bool before = level_.solid(x, y);
    if (before == value)
      return;
    const auto tile_key = key(0, x, y);
    if (indices.emplace(tile_key, changes.size()).second)
      changes.push_back({x, y, before, value});
  });
  if (changes.empty())
    return;
  for (const auto &cell : changes)
    level_.set_solid(cell.x, cell.y, cell.after);
  Command command;
  command.changes_collision = true;
  command.collision = std::move(changes);
  command.bytes = command.collision.size() * sizeof(CollisionChange);
  commit(std::move(command));
}
std::uint64_t Document::add_entity(EntityType type, float x, float y) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  if (next_entity_id_ == 0)
    throw std::overflow_error("No entity IDs remain");
  const auto before = level_.entities;
  Entity entity{next_entity_id_, type, x, y, {}};
  level_.entities.push_back(std::move(entity));
  try {
    level_.validate();
  } catch (...) {
    level_.entities = before;
    throw;
  }
  const auto id = next_entity_id_;
  next_entity_id_ = id == UINT64_MAX ? 0 : id + 1;
  commit_entities(before);
  return id;
}
void Document::update_entity(Entity entity) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  auto it = std::find_if(
      level_.entities.begin(), level_.entities.end(),
      [&](const auto &current) { return current.id == entity.id; });
  if (it == level_.entities.end())
    throw std::out_of_range("Entity ID");
  const auto before = level_.entities;
  *it = std::move(entity);
  try {
    level_.validate();
  } catch (...) {
    level_.entities = before;
    throw;
  }
  commit_entities(before);
}
void Document::delete_entity(std::uint64_t id) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  const auto found =
      std::find_if(level_.entities.begin(), level_.entities.end(),
                   [&](const auto &entity) { return entity.id == id; });
  if (found == level_.entities.end())
    throw std::out_of_range("Entity ID");
  auto before = level_.entities;
  level_.entities.erase(level_.entities.begin() +
                        std::distance(level_.entities.begin(), found));
  commit_entities(std::move(before));
}
void Document::set_asset_settings(std::string path, int tile_width,
                                  int tile_height, int atlas_width,
                                  int atlas_height) {
  if (recording_stroke_ || recording_collision_stroke_)
    throw std::logic_error("Finish the current stroke first");
  const auto before = level_;
  level_.tileset_path = std::move(path);
  level_.tile_width = tile_width;
  level_.tile_height = tile_height;
  level_.atlas_tile_width = atlas_width;
  level_.atlas_tile_height = atlas_height;
  try {
    level_.validate();
  } catch (...) {
    level_ = before;
    throw;
  }
  commit_asset_settings(before);
}
} // namespace pf
