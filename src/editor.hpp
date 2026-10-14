#pragma once
#include "document.hpp"
#include "tileset.hpp"
#include <SDL3/SDL.h>
#include <filesystem>
namespace pf {
enum class Tool { pencil, eraser, rectangle, line, fill, eyedropper };
class Editor {
  Document document_;
  Tileset tileset_;
  SDL_Renderer *renderer_;
  std::filesystem::path document_path_;
  char path_[512] = "level.json";
  char tileset_path_[512] = "assets/studio_tiles.png";
  char layer_name_[128] = "Layer";
  int genre_ = 0, width_ = 32, height_ = 24, selected_tile_ = 1,
      active_layer_ = 0, tile_width_ = 16, tile_height_ = 16;
  float zoom_ = 1.0f, pan_x_ = 20, pan_y_ = 20;
  bool grid_visible_ = true, stroke_active_ = false, gesture_active_ = false,
       fit_requested_ = false;
  int gesture_x_ = 0, gesture_y_ = 0, previous_x_ = 0, previous_y_ = 0;
  int stroke_layer_ = 0, gesture_layer_ = 0;
  int stroke_value_ = 0;
  Tool gesture_tool_ = Tool::rectangle;
  Tool tool_ = Tool::pencil;
  std::string status_ = "Ready";
  bool confirm_new_ = false, confirm_open_ = false;
  void toolbar();
  void layers_panel();
  void canvas();
  void set_active_layer(int index);
  void open_path();
  void new_level();
  void save_level();
  void mark_dirty_status();

public:
  explicit Editor(SDL_Renderer *renderer);
  bool assets_ready() const noexcept { return tileset_.loaded(); }
  void draw();
};
} // namespace pf
