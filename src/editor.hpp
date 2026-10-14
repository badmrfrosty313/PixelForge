#pragma once
#include "document.hpp"
#include "gameplay.hpp"
#include "tileset.hpp"
#include <SDL3/SDL.h>
#include <chrono>
#include <array>
#include <filesystem>
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>
namespace pf {
struct DialogMailbox;
struct VisualControlGeometry {
  std::string window, label;
  std::array<float, 4> bounds{}, clip_bounds{};
  bool visible = false, clipped_horizontally = false,
       clipped_vertically = false, vertically_scrollable = false;
};
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
struct VisualGeometry {
  float canvas_x = 0, canvas_y = 0, canvas_width = 0, canvas_height = 0;
  float map_x = 0, map_y = 0, map_width = 0, map_height = 0;
  float zoom = 1;
  int map_width_tiles = 0, map_height_tiles = 0, ui_scale_percent = 100;
};
class Editor {
  Document document_;
  Tileset tileset_;
  std::unique_ptr<gameplay::Session> playtest_;
  std::chrono::steady_clock::time_point last_playtest_frame_;
  SDL_Renderer *renderer_;
  ImGuiStyle base_style_;
  ImFont *status_font_ = nullptr;
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
  VisualGeometry visual_geometry_;
  std::vector<VisualControlGeometry> visual_controls_;
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
  bool confirm_new_ = false, confirm_open_ = false, confirm_close_ = false,
       defer_close_prompt_ = false;
  bool close_requested_ = false;
  bool reset_layout_requested_ = false, focus_canvas_ = false;
  bool interaction_test_mode_ = false;
  bool exit_menu_activated_this_frame_ = false;
  void apply_ui_scale(int percent);
  void set_default_layout(ImGuiID dockspace_id, bool force);
  void request_open_dialog();
  void request_save_dialog(SaveIntent intent = SaveIntent::none);
  void request_tileset_dialog();
  void poll_dialog_result();
  void toolbar();
  void layers_panel();
  void canvas();
  void playtest_canvas(bool request_focus);
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
  void record_visual_control(const char *label);

public:
  explicit Editor(SDL_Renderer *renderer, float display_scale = 1.0f);
  void set_display_scale(float scale);
  void start_smoke_playtest(Genre genre);
  void stop_playtest();
  void visual_load_demo(Genre genre, bool playtest);
  void visual_create_large_map();
  void visual_select_entity(std::uint64_t id);
  void visual_step_gameplay(const gameplay::Input &input, int steps);
  void visual_load_path(const std::filesystem::path &path);
  void visual_enable_interaction_test() noexcept {
    interaction_test_mode_ = true;
  }
  void visual_cancel_pending_dialog_for_test();
  bool visual_playtest_active() const noexcept { return !!playtest_; }
  bool visual_document_dirty() const noexcept { return document_.dirty(); }
  bool visual_dialog_pending() const noexcept { return dialog_pending_; }
  bool visual_exit_menu_activated() const noexcept {
    return exit_menu_activated_this_frame_;
  }
  const gameplay::Player &visual_player() const;
  const std::string &visual_gameplay_message() const;
  void visual_show_welcome(bool visible) { show_welcome_ = visible; }
  bool assets_ready() const noexcept { return tileset_.loaded(); }
  const VisualGeometry &visual_geometry() const noexcept {
    return visual_geometry_;
  }
  const std::vector<VisualControlGeometry> &visual_controls() const noexcept {
    return visual_controls_;
  }
  static bool visual_overflow_probe() noexcept;
  void request_close();
  bool should_close() const noexcept { return close_requested_; }
  void draw();
};
} // namespace pf
