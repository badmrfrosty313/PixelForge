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
}
void Document::commit(Command command) {
  if (recording_stroke_)
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
void Document::undo() {
  if (!can_undo())
    return;
  const auto &c = history_[cursor_ - 1];
  if (c.changes_layers)
    level_.layers = c.before_layers;
  else
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
  else
    for (const auto &t : c.tiles)
      level_.paint(t.layer, t.x, t.y, t.after);
  state_ = c.after_state;
  ++cursor_;
}
void Document::begin_stroke() {
  if (recording_stroke_)
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
  if (recording_stroke_)
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
  if (recording_stroke_)
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
  if (recording_stroke_)
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
  if (recording_stroke_)
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
  if (recording_stroke_)
    throw std::logic_error("Finish the current stroke first");
  auto before = level_.layers;
  level_.layers.at(i).visible = value;
  commit_layers(std::move(before));
}
void Document::set_layer_locked(int i, bool value) {
  if (recording_stroke_)
    throw std::logic_error("Finish the current stroke first");
  auto before = level_.layers;
  level_.layers.at(i).locked = value;
  commit_layers(std::move(before));
}
} // namespace pf
