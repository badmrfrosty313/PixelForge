#pragma once
#include "document.hpp"
#include "gameplay.hpp"
#include "tileset.hpp"
#include <SDL3/SDL.h>
#include <chrono>
#include <filesystem>
#include <imgui.h>
#include <memory>
namespace pf {
struct DialogMailbox;
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
  ImGuiStyle base_style_;
  std::filesystem::path document_path_;
  std::filesystem::path resource_root_;
  std::filesystem::path pending_open_path_, pending_tileset_source_;
  char tileset_path_[512] = "assets/studio_tiles.png";
  char layer_name_[128] = "Layer";
  int genre_ = 0, width_ = 32, height_ = 24, selected_tile_ = 1,
      active_layer_ = 0, tile_width_ = 16, tile_height_ = 16;
  int level_tile_width_ = 24, level_tile_height_ = 24, entity_type_index_ = 1;
  float zoom_ = 1.0f, pan_x_ = 20, pan_y_ = 20;
  int ui_scale_percent_ = 100;
  int cursor_cell_x_ = -1, cursor_cell_y_ = -1;
  bool cursor_on_level_ = false;
  bool grid_visible_ = true, stroke_active_ = false, gesture_active_ = false,
       fit_requested_ = false;
  bool collision_visible_ = false, collision_stroke_active_ = false,
       entity_drag_active_ = false, debug_overlay_ = false;
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
  std::string error_message_;
  std::shared_ptr<DialogMailbox> dialog_mailbox_;
  enum class SaveIntent { none, open_level, create_new, close_application };
  SaveIntent save_intent_ = SaveIntent::none;
  bool dialog_pending_ = false, close_new_popup_ = false,
       close_open_popup_ = false;
  bool error_popup_pending_ = false;
  bool show_welcome_ = true;
  bool confirm_new_ = false, confirm_open_ = false, confirm_close_ = false;
  bool close_requested_ = false;
  bool reset_layout_requested_ = false, focus_editor_canvas_ = false;
  void apply_ui_scale(int percent);
  void set_default_layout(ImGuiID dockspace_id, bool force);
  void request_open_dialog();
  void request_save_dialog(SaveIntent intent = SaveIntent::none);
  void request_tileset_dialog();
  void poll_dialog_result();
  void toolbar();
  void layers_panel();
  void canvas();
  void playtest_canvas();
  void entity_inspector();
  void sync_entity_fields();
  void set_active_layer(int index);
  void open_path(const std::filesystem::path &path);
  void import_tileset(const std::filesystem::path &path);
  void new_level();
  bool save_level();
  bool save_level_to(const std::filesystem::path &path);
  void report_error(const std::string &message);
  void mark_dirty_status();

public:
  explicit Editor(SDL_Renderer *renderer, float display_scale = 1.0f);
  void set_display_scale(float scale);
  bool assets_ready() const noexcept { return tileset_.loaded(); }
  void request_close();
  bool should_close() const noexcept { return close_requested_; }
  void draw();
};
} // namespace pf
