#pragma once
#include "document.hpp"
#include "gameplay.hpp"
#include "tileset.hpp"
#include <SDL3/SDL.h>
#include <chrono>
#include <filesystem>
#include <memory>
namespace pf {
enum class Tool {
  pencil,
  eraser,
  rectangle,
  line,
  fill,
  eyedropper,
  collision,
  entity
};
class Editor {
  Document document_;
  Tileset tileset_;
  std::unique_ptr<gameplay::Session> playtest_;
  std::chrono::steady_clock::time_point last_playtest_frame_;
  SDL_Renderer *renderer_;
  std::filesystem::path document_path_;
  char path_[512] = "level.json";
  char tileset_path_[512] = "assets/studio_tiles.png";
  char layer_name_[128] = "Layer";
  int genre_ = 0, width_ = 32, height_ = 24, selected_tile_ = 1,
      active_layer_ = 0, tile_width_ = 16, tile_height_ = 16;
  int level_tile_width_ = 24, level_tile_height_ = 24, entity_type_index_ = 1;
  float zoom_ = 1.0f, pan_x_ = 20, pan_y_ = 20;
  bool grid_visible_ = true, stroke_active_ = false, gesture_active_ = false,
       fit_requested_ = false;
  bool collision_visible_ = false, collision_stroke_active_ = false,
       entity_drag_active_ = false;
  bool collision_value_ = true;
  std::uint64_t selected_entity_id_ = 0, entity_drag_id_ = 0;
  float entity_x_ = 0, entity_y_ = 0, entity_drag_x_ = 0, entity_drag_y_ = 0;
  char entity_name_[128] = "", entity_message_[256] = "";
  int gesture_x_ = 0, gesture_y_ = 0, previous_x_ = 0, previous_y_ = 0;
  int stroke_layer_ = 0, gesture_layer_ = 0;
  int stroke_value_ = 0;
  Tool gesture_tool_ = Tool::rectangle;
  int gesture_value_ = 1;
  Tool tool_ = Tool::pencil;
  std::string status_ = "Ready";
  bool confirm_new_ = false, confirm_open_ = false, confirm_close_ = false;
  bool close_requested_ = false;
  void toolbar();
  void layers_panel();
  void canvas();
  void playtest_canvas();
  void entity_inspector();
  void sync_entity_fields();
  void set_active_layer(int index);
  void open_path();
  void new_level();
  bool save_level();
  bool save_level_as();
  void mark_dirty_status();

public:
  explicit Editor(SDL_Renderer *renderer);
  bool assets_ready() const noexcept { return tileset_.loaded(); }
  void request_close();
  bool should_close() const noexcept { return close_requested_; }
  void draw();
};
} // namespace pf
