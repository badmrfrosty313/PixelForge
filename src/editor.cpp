#include "editor.hpp"
#include "rendering.hpp"
#include "serialization.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <imgui.h>
#include <utility>
namespace pf {
namespace {
const char *tool_name(Tool t) {
  switch (t) {
  case Tool::pencil:
    return "Pencil";
  case Tool::eraser:
    return "Eraser";
  case Tool::rectangle:
    return "Rectangle";
  case Tool::line:
    return "Line";
  case Tool::fill:
    return "Fill";
  case Tool::eyedropper:
    return "Eyedropper";
  }
  return "Tool";
}
} // namespace
Editor::Editor(SDL_Renderer *renderer) : renderer_(renderer) {
  std::filesystem::path default_path =
      std::filesystem::current_path() / "assets/studio_tiles.png";
  if (!std::filesystem::exists(default_path)) {
    const char *executable_base = SDL_GetBasePath();
    if (executable_base && *executable_base) {
      const auto executable_path =
          std::filesystem::path(executable_base) / "assets/studio_tiles.png";
      if (std::filesystem::exists(executable_path))
        default_path = executable_path;
    }
    if (!std::filesystem::exists(default_path))
      default_path = std::filesystem::path(PIXELFORGE_SOURCE_ASSET_DIR) /
                     "studio_tiles.png";
  }
  const auto text = default_path.string();
  std::snprintf(tileset_path_, sizeof(tileset_path_), "%s", text.c_str());
  try {
    tileset_.load(renderer_, default_path, tile_width_, tile_height_);
    status_ = "Loaded original Studio tileset";
  } catch (const std::exception &e) {
    status_ = e.what();
  }
}
void Editor::set_active_layer(int index) {
  active_layer_ = std::clamp(
      index, 0, static_cast<int>(document_.level().layers.size()) - 1);
}
void Editor::mark_dirty_status() {
  if (document_.dirty())
    status_ = "Modified";
}
void Editor::save_level() {
  try {
    const auto target =
        document_path_.empty() ? std::filesystem::path(path_) : document_path_;
    save(document_.level(), target);
    document_path_ = target;
    const auto text = target.string();
    std::snprintf(path_, sizeof(path_), "%s", text.c_str());
    document_.mark_saved();
    status_ = "Saved";
  } catch (const std::exception &e) {
    status_ = e.what();
  }
}
void Editor::open_path() {
  try {
    if (stroke_active_) {
      document_.end_stroke();
      stroke_active_ = false;
    }
    auto loaded = load(path_);
    document_ = Document(std::move(loaded));
    document_.mark_saved();
    document_path_ = path_;
    genre_ = document_.level().genre == Genre::top_down_rpg ? 0 : 1;
    width_ = document_.level().width;
    height_ = document_.level().height;
    set_active_layer(0);
    status_ = "Opened level";
  } catch (const std::exception &e) {
    status_ = e.what();
  }
}
void Editor::new_level() {
  try {
    if (stroke_active_) {
      document_.end_stroke();
      stroke_active_ = false;
    }
    document_ =
        Document(Level(genre_ == 0 ? Genre::top_down_rpg : Genre::platformer,
                       width_, height_));
    document_path_.clear();
    std::snprintf(path_, sizeof(path_), "level.json");
    set_active_layer(0);
    status_ = "Created new level";
  } catch (const std::exception &e) {
    status_ = e.what();
  }
}
void Editor::toolbar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New", "Ctrl+N")) {
        if (document_.dirty())
          confirm_new_ = true;
        else
          new_level();
      }
      if (ImGui::MenuItem("Open...", "Ctrl+O")) {
        if (document_.dirty())
          confirm_open_ = true;
        else
          open_path();
      }
      if (ImGui::MenuItem("Save", "Ctrl+S"))
        save_level();
      ImGui::Separator();
      if (ImGui::MenuItem("Exit")) {
        SDL_Event event{};
        event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&event);
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z", false, document_.can_undo()))
        document_.undo();
      if (ImGui::MenuItem("Redo", "Ctrl+Y", false, document_.can_redo()))
        document_.redo();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Show grid", nullptr, &grid_visible_)) {
      }
      if (ImGui::MenuItem("Fit level", "F"))
        fit_requested_ = true;
      if (ImGui::MenuItem("Reset zoom", "0")) {
        zoom_ = 1;
        pan_x_ = 20;
        pan_y_ = 20;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
  ImGui::SetNextWindowPos(ImVec2(8, 28), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(1264, 52), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Toolbar")) {
    if (ImGui::Button(document_.can_undo() ? "Undo" : "Undo"))
      document_.undo();
    ImGui::SameLine();
    if (ImGui::Button("Redo"))
      document_.redo();
    ImGui::SameLine();
    for (auto tool : {Tool::pencil, Tool::eraser, Tool::rectangle, Tool::line,
                      Tool::fill, Tool::eyedropper}) {
      if (tool != Tool::pencil)
        ImGui::SameLine();
      if (tool_ == tool)
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
      if (ImGui::Button(tool_name(tool)))
        tool_ = tool;
      if (tool_ == tool)
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Grid", &grid_visible_);
    ImGui::SameLine();
    if (ImGui::Button("Fit"))
      fit_requested_ = true;
    ImGui::SameLine();
    if (ImGui::Button("100%")) {
      zoom_ = 1;
      pan_x_ = 20;
      pan_y_ = 20;
    }
    ImGui::SameLine();
    ImGui::Text("%s%s | %dx%d | %.0f%%",
                document_path_.empty()
                    ? "Untitled"
                    : document_path_.filename().string().c_str(),
                document_.dirty() ? " *" : "", document_.level().width,
                document_.level().height, zoom_ * 100.0f);
  }
  ImGui::End();
  if (confirm_new_) {
    ImGui::OpenPopup("Discard edits before New?");
    confirm_new_ = false;
  }
  if (ImGui::BeginPopupModal("Discard edits before New?", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("The current document has unsaved changes.");
    if (ImGui::Button("Discard and create")) {
      new_level();
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
  if (confirm_open_) {
    ImGui::OpenPopup("Discard edits before Open?");
    confirm_open_ = false;
  }
  if (ImGui::BeginPopupModal("Discard edits before Open?", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("The current document has unsaved changes.");
    if (ImGui::Button("Discard and open")) {
      open_path();
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
}
void Editor::layers_panel() {
  ImGui::SetNextWindowPos(ImVec2(1016, 88), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(256, 680), ImGuiCond_FirstUseEver);
  ImGui::Begin("Layers");
  if (ImGui::Button("+ Layer")) {
    try {
      document_.create_layer(layer_name_);
      set_active_layer(static_cast<int>(document_.level().layers.size()) - 1);
      mark_dirty_status();
    } catch (const std::exception &e) {
      status_ = e.what();
    }
  }
  ImGui::SameLine();
  ImGui::InputText("##new-layer-name", layer_name_, sizeof(layer_name_));
  for (int i = static_cast<int>(document_.level().layers.size()) - 1; i >= 0;
       --i) {
    const auto &layer = document_.level().layers[static_cast<std::size_t>(i)];
    const std::string layer_name = layer.name;
    ImGui::PushID(i);
    if (ImGui::Selectable(layer.name.c_str(), active_layer_ == i,
                          ImGuiSelectableFlags_AllowDoubleClick,
                          ImVec2(112, 0)))
      set_active_layer(i);
    ImGui::SameLine();
    bool visible = layer.visible;
    if (ImGui::Checkbox("V##visible", &visible)) {
      try {
        document_.set_layer_visible(i, visible);
        mark_dirty_status();
      } catch (const std::exception &e) {
        status_ = e.what();
      }
    }
    ImGui::SameLine();
    bool locked = layer.locked;
    if (ImGui::Checkbox("L##locked", &locked)) {
      try {
        document_.set_layer_locked(i, locked);
        mark_dirty_status();
      } catch (const std::exception &e) {
        status_ = e.what();
      }
    }
    if (active_layer_ == i) {
      if (ImGui::SmallButton("Move up") &&
          i + 1 < static_cast<int>(document_.level().layers.size())) {
        document_.move_layer(i, i + 1);
        set_active_layer(i + 1);
        mark_dirty_status();
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("Move down") && i > 0) {
        document_.move_layer(i, i - 1);
        set_active_layer(i - 1);
        mark_dirty_status();
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("Rename"))
        ImGui::OpenPopup("Rename layer");
      if (ImGui::BeginPopup("Rename layer")) {
        static char renamed[128];
        if (ImGui::IsWindowAppearing())
          std::snprintf(renamed, sizeof(renamed), "%s", layer_name.c_str());
        ImGui::InputText("Name", renamed, sizeof(renamed));
        if (ImGui::Button("Apply")) {
          try {
            document_.rename_layer(i, renamed);
            mark_dirty_status();
          } catch (const std::exception &e) {
            status_ = e.what();
          }
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("Delete")) {
        try {
          document_.delete_layer(i);
          set_active_layer(std::min(
              i, static_cast<int>(document_.level().layers.size()) - 1));
          mark_dirty_status();
        } catch (const std::exception &e) {
          status_ = e.what();
        }
      }
    }
    ImGui::PopID();
  }
  ImGui::TextWrapped(
      "Top rows draw above lower rows. V toggles visibility; L locks editing.");
  ImGui::End();
}
void Editor::canvas() {
  ImGui::SetNextWindowPos(ImVec2(278, 88), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(730, 680), ImGuiCond_FirstUseEver);
  ImGui::Begin("Level Canvas");
  ImVec2 pos = ImGui::GetCursorScreenPos(),
         size = ImGui::GetContentRegionAvail();
  size.x = std::max(size.x, 1.0f);
  size.y = std::max(size.y, 1.0f);
  ImGui::InvisibleButton("canvas", size,
                         ImGuiButtonFlags_MouseButtonLeft |
                             ImGuiButtonFlags_MouseButtonRight |
                             ImGuiButtonFlags_MouseButtonMiddle);
  const bool hovered = ImGui::IsItemHovered(), focused = ImGui::IsItemFocused();
  auto &io = ImGui::GetIO();
  auto *draw = ImGui::GetWindowDrawList();
  if (focused && ImGui::IsKeyPressed(ImGuiKey_Z) && io.KeyCtrl) {
    if (stroke_active_) {
      document_.end_stroke();
      stroke_active_ = false;
    }
    if (io.KeyShift)
      document_.redo();
    else
      document_.undo();
  }
  if (focused && ImGui::IsKeyPressed(ImGuiKey_Y) && io.KeyCtrl) {
    if (stroke_active_) {
      document_.end_stroke();
      stroke_active_ = false;
    }
    document_.redo();
  }
  if (focused && ImGui::IsKeyPressed(ImGuiKey_S) && io.KeyCtrl)
    save_level();
  if (focused && ImGui::IsKeyPressed(ImGuiKey_N) && io.KeyCtrl) {
    if (document_.dirty())
      confirm_new_ = true;
    else
      new_level();
  }
  if (focused && ImGui::IsKeyPressed(ImGuiKey_O) && io.KeyCtrl) {
    if (document_.dirty())
      confirm_open_ = true;
    else
      open_path();
  }
  if (focused && ImGui::IsKeyPressed(ImGuiKey_F))
    fit_requested_ = true;
  if (focused && ImGui::IsKeyPressed(ImGuiKey_0)) {
    zoom_ = 1;
    pan_x_ = 20;
    pan_y_ = 20;
  }
  if (hovered && io.MouseWheel != 0) {
    float next = std::clamp(zoom_ * std::pow(1.15f, io.MouseWheel), 0.2f, 6.0f);
    pan_x_ = (pan_x_ - (io.MousePos.x - pos.x)) * next / zoom_ +
             (io.MousePos.x - pos.x);
    pan_y_ = (pan_y_ - (io.MousePos.y - pos.y)) * next / zoom_ +
             (io.MousePos.y - pos.y);
    zoom_ = next;
  }
  if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
    pan_x_ += io.MouseDelta.x;
    pan_y_ += io.MouseDelta.y;
  }
  float cell = 24 * zoom_;
  ImVec2 origin(pos.x + pan_x_, pos.y + pan_y_);
  auto cursor_cell = [&]() {
    int x = static_cast<int>(std::floor((io.MousePos.x - origin.x) / cell)),
        y = static_cast<int>(std::floor((io.MousePos.y - origin.y) / cell));
    return std::pair{x, y};
  };
  auto [mx, my] = cursor_cell();
  const bool inside = mx >= 0 && my >= 0 && mx < document_.level().width &&
                      my < document_.level().height;
  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && inside) {
    if (tool_ == Tool::pencil || tool_ == Tool::eraser) {
      stroke_value_ = tool_ == Tool::eraser ? 0 : selected_tile_;
      stroke_layer_ = active_layer_;
      document_.begin_stroke();
      stroke_active_ = true;
      previous_x_ = mx;
      previous_y_ = my;
      document_.paint_line(stroke_layer_, mx, my, mx, my, stroke_value_);
    } else if (tool_ == Tool::rectangle || tool_ == Tool::line) {
      gesture_active_ = true;
      gesture_tool_ = tool_;
      gesture_layer_ = active_layer_;
      gesture_x_ = mx;
      gesture_y_ = my;
    } else if (tool_ == Tool::fill) {
      try {
        document_.flood_fill(active_layer_, mx, my, selected_tile_);
        mark_dirty_status();
      } catch (const std::exception &e) {
        status_ = e.what();
      }
    } else if (tool_ == Tool::eyedropper) {
      selected_tile_ = document_.tile(active_layer_, mx, my);
      tool_ = Tool::pencil;
    }
  }
  if (stroke_active_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    auto [x, y] = cursor_cell();
    x = std::clamp(x, 0, document_.level().width - 1);
    y = std::clamp(y, 0, document_.level().height - 1);
    document_.paint_line(stroke_layer_, previous_x_, previous_y_, x, y,
                         stroke_value_);
    previous_x_ = x;
    previous_y_ = y;
  }
  if (stroke_active_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    auto [x, y] = cursor_cell();
    x = std::clamp(x, 0, document_.level().width - 1);
    y = std::clamp(y, 0, document_.level().height - 1);
    document_.paint_line(stroke_layer_, previous_x_, previous_y_, x, y,
                         stroke_value_);
  }
  if (gesture_active_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    auto [x, y] = cursor_cell();
    x = std::clamp(x, 0, document_.level().width - 1);
    y = std::clamp(y, 0, document_.level().height - 1);
    if (gesture_tool_ == Tool::line)
      document_.line(gesture_layer_, gesture_x_, gesture_y_, x, y,
                     selected_tile_);
    else
      document_.rectangle(gesture_layer_, gesture_x_, gesture_y_, x, y,
                          selected_tile_);
    gesture_active_ = false;
    mark_dirty_status();
  }
  if (stroke_active_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    document_.end_stroke();
    stroke_active_ = false;
    mark_dirty_status();
  }
  draw->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
  draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      IM_COL32(25, 28, 34, 255));
  if (fit_requested_) {
    zoom_ = std::clamp(std::min(size.x / (document_.level().width * 24.0f),
                                size.y / (document_.level().height * 24.0f)),
                       0.2f, 6.0f);
    cell = 24 * zoom_;
    pan_x_ = (size.x - document_.level().width * cell) * 0.5f;
    pan_y_ = (size.y - document_.level().height * cell) * 0.5f;
    origin = ImVec2(pos.x + pan_x_, pos.y + pan_y_);
    fit_requested_ = false;
  }
  const int x0 = std::clamp(static_cast<int>(std::floor(-pan_x_ / cell)), 0,
                            document_.level().width),
            y0 = std::clamp(static_cast<int>(std::floor(-pan_y_ / cell)), 0,
                            document_.level().height);
  const int x1 = std::clamp(
                static_cast<int>(std::ceil((size.x - pan_x_) / cell)), 0,
                document_.level().width),
            y1 = std::clamp(
                static_cast<int>(std::ceil((size.y - pan_y_) / cell)), 0,
                document_.level().height);
  for (int y = y0; y < y1; ++y)
    for (int x = x0; x < x1; ++x) {
      ImVec2 a(origin.x + x * cell, origin.y + y * cell),
          b(a.x + cell, a.y + cell);
      for (const auto &layer : document_.level().layers)
        if (layer.visible) {
          int id = layer.tiles[static_cast<std::size_t>(
              y * document_.level().width + x)];
          if (id)
            draw_tile(draw, tileset_, id, a, b);
        }
      if (grid_visible_)
        draw->AddRect(a, b, IM_COL32(40, 53, 59, 160));
    }
  if (gesture_active_) {
    auto [x, y] = cursor_cell();
    ImVec2 a(origin.x + std::min(gesture_x_, x) * cell,
             origin.y + std::min(gesture_y_, y) * cell),
        b(origin.x + (std::max(gesture_x_, x) + 1) * cell,
          origin.y + (std::max(gesture_y_, y) + 1) * cell);
    if (gesture_tool_ == Tool::line)
      draw->AddLine(
          ImVec2(origin.x + (gesture_x_ + 0.5f) * cell,
                 origin.y + (gesture_y_ + 0.5f) * cell),
          ImVec2(origin.x + (x + 0.5f) * cell, origin.y + (y + 0.5f) * cell),
          IM_COL32(255, 230, 125, 255), 2.0f);
    else
      draw->AddRect(a, b, IM_COL32(255, 230, 125, 255), 0, 0, 2.0f);
  }
  draw->PopClipRect();
  ImGui::End();
}
void Editor::draw() {
  ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
  toolbar();
  ImGui::SetNextWindowPos(ImVec2(8, 88), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(262, 680), ImGuiCond_FirstUseEver);
  ImGui::Begin("Tileset Palette");
  ImGui::Combo("New genre", &genre_, "Top-down RPG\0Platformer\0");
  ImGui::InputInt("New width", &width_);
  ImGui::InputInt("New height", &height_);
  if (ImGui::Button("New level")) {
    if (document_.dirty())
      confirm_new_ = true;
    else
      new_level();
  }
  ImGui::InputText("Level JSON", path_, sizeof(path_));
  ImGui::SameLine();
  if (ImGui::Button("Open")) {
    if (document_.dirty())
      confirm_open_ = true;
    else
      open_path();
  }
  ImGui::SameLine();
  if (ImGui::Button("Save"))
    save_level();
  ImGui::Separator();
  ImGui::InputText("PNG path", tileset_path_, sizeof(tileset_path_));
  ImGui::InputInt("Tile width", &tile_width_);
  ImGui::InputInt("Tile height", &tile_height_);
  if (ImGui::Button("Load PNG")) {
    try {
      tileset_.load(renderer_, tileset_path_, tile_width_, tile_height_);
      selected_tile_ = 1;
      status_ = "Loaded PNG tileset";
    } catch (const std::exception &e) {
      status_ = e.what();
    }
  }
  ImGui::Text("%s", tileset_.loaded() ? "PNG atlas ready" : "No atlas loaded");
  if (tileset_.loaded()) {
    const int count = std::min(tileset_.tile_count(), 255);
    const int per_row = std::max(
        1, static_cast<int>((ImGui::GetContentRegionAvail().x) / 42.0f));
    ImGui::BeginChild("atlas tiles", ImVec2(0, 0), ImGuiChildFlags_Borders);
    auto *list = ImGui::GetWindowDrawList();
    for (int i = 1; i <= count; ++i) {
      ImGui::PushID(i);
      ImVec2 at = ImGui::GetCursorScreenPos();
      ImGui::InvisibleButton("tile", ImVec2(38, 38));
      if (ImGui::IsItemClicked())
        selected_tile_ = i;
      ImVec2 a(at.x + 3, at.y + 3), b(at.x + 35, at.y + 35);
      if (i == selected_tile_)
        list->AddRect(a, b, IM_COL32(255, 220, 96, 255), 0, 0, 2);
      draw_tile(list, tileset_, i, a, b);
      ImGui::SameLine();
      if (i % per_row == 0)
        ImGui::NewLine();
      ImGui::PopID();
    }
    ImGui::EndChild();
  }
  ImGui::Text("Selected tile: %d", selected_tile_);
  if (tileset_.loaded())
    ImGui::SliderInt("Tile ID", &selected_tile_, 0,
                     std::min(tileset_.tile_count(), 255));
  ImGui::End();
  layers_panel();
  canvas();
  const ImVec2 vp = ImGui::GetMainViewport()->Pos,
               vsz = ImGui::GetMainViewport()->Size;
  ImGui::SetNextWindowPos(ImVec2(vp.x, vp.y + vsz.y - 26), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(vsz.x, 26), ImGuiCond_Always);
  ImGui::Begin("Status", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
  ImGui::TextWrapped("%s | %s", document_.dirty() ? "Modified" : "Saved",
                     status_.c_str());
  ImGui::End();
}
} // namespace pf
