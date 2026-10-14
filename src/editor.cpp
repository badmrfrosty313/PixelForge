#include "editor.hpp"
#include "coordinates.hpp"
#include "rendering.hpp"
#include "serialization.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <imgui.h>
#include <stdexcept>
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
  case Tool::collision:
    return "Collision";
  case Tool::entity:
    return "Entity";
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
bool Editor::save_level() {
  try {
    const auto target =
        document_path_.empty() ? std::filesystem::path(path_) : document_path_;
    if (target.empty())
      throw std::invalid_argument("Level path cannot be empty");
    save(document_.level(), target);
    document_path_ = target;
    const auto text = target.string();
    std::snprintf(path_, sizeof(path_), "%s", text.c_str());
    document_.mark_saved();
    status_ = "Saved";
    return true;
  } catch (const std::exception &e) {
    status_ = e.what();
    return false;
  }
}
bool Editor::save_level_as() {
  try {
    const std::filesystem::path target(path_);
    if (target.empty())
      throw std::invalid_argument("Level path cannot be empty");
    save(document_.level(), target);
    document_path_ = target;
    document_.mark_saved();
    status_ = "Saved as " + target.string();
    return true;
  } catch (const std::exception &e) {
    status_ = e.what();
    return false;
  }
}
void Editor::request_close() {
  if (document_.dirty())
    confirm_close_ = true;
  else
    close_requested_ = true;
}
void Editor::open_path() {
  try {
    if (stroke_active_) {
      document_.end_stroke();
      stroke_active_ = false;
    }
    auto loaded = load(path_);
    std::string asset_warning;
    std::vector<std::filesystem::path> asset_candidates{
        std::filesystem::current_path() / loaded.tileset_path};
    if (const char *executable_base = SDL_GetBasePath();
        executable_base && *executable_base)
      asset_candidates.emplace_back(std::filesystem::path(executable_base) /
                                    loaded.tileset_path);
    asset_candidates.emplace_back(
        std::filesystem::path(PIXELFORGE_SOURCE_ASSET_DIR).parent_path() /
        loaded.tileset_path);
    std::exception_ptr asset_error;
    for (const auto &asset_path : asset_candidates) {
      try {
        tileset_.load(renderer_, asset_path, loaded.atlas_tile_width,
                      loaded.atlas_tile_height);
        asset_error = nullptr;
        break;
      } catch (...) {
        asset_error = std::current_exception();
      }
    }
    if (asset_error) {
      try {
        std::rethrow_exception(asset_error);
      } catch (const std::exception &error) {
        asset_warning = std::string("; tileset unavailable: ") + error.what();
      }
    } else {
      std::snprintf(tileset_path_, sizeof(tileset_path_), "%s",
                    loaded.tileset_path.c_str());
      tile_width_ = loaded.atlas_tile_width;
      tile_height_ = loaded.atlas_tile_height;
    }
    document_ = Document(std::move(loaded));
    document_.mark_saved();
    document_path_ = path_;
    genre_ = document_.level().genre == Genre::top_down_rpg ? 0 : 1;
    width_ = document_.level().width;
    height_ = document_.level().height;
    level_tile_width_ = document_.level().tile_width;
    level_tile_height_ = document_.level().tile_height;
    set_active_layer(0);
    status_ = "Opened level" + asset_warning;
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
    Level level(genre_ == 0 ? Genre::top_down_rpg : Genre::platformer, width_,
                height_);
    level.tile_width = level_tile_width_;
    level.tile_height = level_tile_height_;
    level.atlas_tile_width = tile_width_;
    level.atlas_tile_height = tile_height_;
    level.tileset_path = tileset_path_;
    level.validate();
    document_ = Document(std::move(level));
    document_path_.clear();
    std::snprintf(path_, sizeof(path_), "level.json");
    set_active_layer(0);
    status_ = "Created new level";
    selected_entity_id_ = 0;
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
      if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
        save_level_as();
      ImGui::Separator();
      if (ImGui::MenuItem("Exit"))
        request_close();
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
      if (ImGui::MenuItem("Playtest collision debug", "F3",
                          &debug_overlay_)) {
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
    if (playtest_) {
      ImGui::TextUnformatted("PLAYTEST");
      ImGui::SameLine();
      if (ImGui::Button(playtest_->paused() ? "Resume" : "Pause"))
        playtest_->set_paused(!playtest_->paused());
      ImGui::SameLine();
      if (ImGui::Button("Reset"))
        playtest_->reset();
      ImGui::SameLine();
      if (ImGui::Button("Stop"))
        playtest_.reset();
      ImGui::SameLine();
      ImGui::Text("%s", genre_name(document_.level().genre).c_str());
      ImGui::SameLine();
      ImGui::Checkbox("Debug overlay", &debug_overlay_);
    } else {
      if (ImGui::Button("Play")) {
        try {
          if (stroke_active_) {
            document_.end_stroke();
            stroke_active_ = false;
          }
          if (collision_stroke_active_) {
            document_.end_collision_stroke();
            collision_stroke_active_ = false;
          }
          playtest_ = std::make_unique<gameplay::Session>(document_.level());
          last_playtest_frame_ = std::chrono::steady_clock::now();
        } catch (const std::exception &error) {
          status_ = error.what();
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Undo"))
        document_.undo();
      ImGui::SameLine();
      if (ImGui::Button("Redo"))
        document_.redo();
      ImGui::SameLine();
      for (auto tool : {Tool::pencil, Tool::eraser, Tool::rectangle, Tool::line,
                        Tool::fill, Tool::eyedropper, Tool::collision,
                        Tool::entity}) {
        if (tool != Tool::pencil)
          ImGui::SameLine();
        if (tool_ == tool)
          ImGui::PushStyleColor(
              ImGuiCol_Button,
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
      ImGui::SameLine();
      ImGui::Checkbox("Collision", &collision_visible_);
    }
  }
  ImGui::End();
  if (confirm_new_) {
    ImGui::OpenPopup("Discard edits before New?");
    confirm_new_ = false;
  }
  if (ImGui::BeginPopupModal("Discard edits before New?", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("The current document has unsaved changes.");
    if (ImGui::Button("Save and create")) {
      if (save_level()) {
        new_level();
        ImGui::CloseCurrentPopup();
      }
    }
    ImGui::SameLine();
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
  if (confirm_close_) {
    ImGui::OpenPopup("Save changes before exit?");
    confirm_close_ = false;
  }
  if (ImGui::BeginPopupModal("Save changes before exit?", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("The current document has unsaved changes.");
    if (ImGui::Button("Save and exit")) {
      if (save_level()) {
        close_requested_ = true;
        confirm_close_ = false;
        ImGui::CloseCurrentPopup();
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Discard and exit")) {
      close_requested_ = true;
      confirm_close_ = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      confirm_close_ = false;
      ImGui::CloseCurrentPopup();
    }
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
void Editor::sync_entity_fields() {
  const auto &entities = document_.level().entities;
  const auto it =
      std::find_if(entities.begin(), entities.end(), [&](const auto &entity) {
        return entity.id == selected_entity_id_;
      });
  if (it == entities.end()) {
    selected_entity_id_ = 0;
    entity_name_[0] = entity_message_[0] = '\0';
    return;
  }
  entity_x_ = it->x;
  entity_y_ = it->y;
  const EntityType types[] = {EntityType::player_spawn, EntityType::npc,
                              EntityType::enemy,        EntityType::collectible,
                              EntityType::trigger,      EntityType::checkpoint,
                              EntityType::hazard};
  for (int i = 0; i < 7; ++i)
    if (types[i] == it->type)
      entity_type_index_ = i;
  const auto name = it->properties.find("name");
  if (name != it->properties.end() && name->second.is_string())
    std::snprintf(entity_name_, sizeof(entity_name_), "%s",
                  name->second.get<std::string>().c_str());
  else
    entity_name_[0] = '\0';
  const auto message = it->properties.find("message");
  if (message != it->properties.end() && message->second.is_string())
    std::snprintf(entity_message_, sizeof(entity_message_), "%s",
                  message->second.get<std::string>().c_str());
  else
    entity_message_[0] = '\0';
}
void Editor::entity_inspector() {
  ImGui::SetNextWindowPos(ImVec2(1016, 88), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(256, 320), ImGuiCond_FirstUseEver);
  ImGui::Begin("Entities");
  const char *types =
      "Player spawn\0NPC\0Enemy\0Collectible\0Trigger\0Checkpoint\0Hazard\0";
  ImGui::Combo("Place type", &entity_type_index_, types);
  const auto &entities = document_.level().entities;
  auto found =
      std::find_if(entities.begin(), entities.end(), [&](const auto &entity) {
        return entity.id == selected_entity_id_;
      });
  if (found == entities.end()) {
    selected_entity_id_ = 0;
    ImGui::TextWrapped(
        "Choose Entity and click the canvas to place an entity.");
    ImGui::End();
    return;
  }
  ImGui::Text("ID: %llu | %s", static_cast<unsigned long long>(found->id),
              entity_type_name(found->type).c_str());
  ImGui::InputFloat("X", &entity_x_, 0.25f, 1.0f, "%.2f");
  ImGui::InputFloat("Y", &entity_y_, 0.25f, 1.0f, "%.2f");
  ImGui::InputText("Name", entity_name_, sizeof(entity_name_));
  ImGui::InputTextMultiline("Message", entity_message_, sizeof(entity_message_),
                            ImVec2(0, 70));
  if (ImGui::Button("Apply properties")) {
    Entity changed = *found;
    changed.x = entity_x_;
    changed.y = entity_y_;
    if (entity_name_[0])
      changed.properties["name"] = entity_name_;
    else
      changed.properties.erase("name");
    if (entity_message_[0])
      changed.properties["message"] = entity_message_;
    else
      changed.properties.erase("message");
    try {
      document_.update_entity(std::move(changed));
      mark_dirty_status();
    } catch (const std::exception &e) {
      status_ = e.what();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete entity")) {
    try {
      document_.delete_entity(selected_entity_id_);
      selected_entity_id_ = 0;
      entity_name_[0] = entity_message_[0] = '\0';
      mark_dirty_status();
    } catch (const std::exception &e) {
      status_ = e.what();
    }
  }
  ImGui::TextWrapped("Select and drag markers with the Entity tool. Entity "
                     "coordinates use tile units.");
  ImGui::End();
}
void Editor::canvas() {
  if (playtest_) {
    playtest_canvas();
    return;
  }
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
  if (focused && ImGui::IsKeyPressed(ImGuiKey_S) && io.KeyCtrl) {
    if (io.KeyShift)
      save_level_as();
    else
      save_level();
  }
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
  float cell_x = document_.level().tile_width * zoom_;
  float cell_y = document_.level().tile_height * zoom_;
  ImVec2 origin(pos.x + pan_x_, pos.y + pan_y_);
  auto cursor_cell = [&]() {
    int x = static_cast<int>(std::floor((io.MousePos.x - origin.x) / cell_x)),
        y = static_cast<int>(std::floor((io.MousePos.y - origin.y) / cell_y));
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
    } else if (tool_ == Tool::collision) {
      collision_value_ = !document_.solid(mx, my);
      document_.begin_collision_stroke();
      collision_stroke_active_ = true;
      previous_x_ = mx;
      previous_y_ = my;
      document_.paint_collision_line(mx, my, mx, my, collision_value_);
    } else if (tool_ == Tool::entity) {
      const auto &entities = document_.level().entities;
      const Entity *found = nullptr;
      const float world_x = (io.MousePos.x - origin.x) / cell_x;
      const float world_y = (io.MousePos.y - origin.y) / cell_y;
      for (auto it = entities.rbegin(); it != entities.rend(); ++it)
        if (std::abs(it->x - world_x) < 0.5f &&
            std::abs(it->y - world_y) < 0.5f) {
          found = &*it;
          break;
        }
      try {
        if (found) {
          selected_entity_id_ = found->id;
          entity_drag_id_ = found->id;
          entity_drag_active_ = true;
          entity_drag_x_ = found->x;
          entity_drag_y_ = found->y;
          sync_entity_fields();
        } else {
          const EntityType types[] = {
              EntityType::player_spawn, EntityType::npc,
              EntityType::enemy,        EntityType::collectible,
              EntityType::trigger,      EntityType::checkpoint,
              EntityType::hazard};
          const auto id = document_.add_entity(types[entity_type_index_],
                                               mx + 0.5f, my + 0.5f);
          selected_entity_id_ = id;
          sync_entity_fields();
          mark_dirty_status();
        }
      } catch (const std::exception &e) {
        status_ = e.what();
      }
    } else if (tool_ == Tool::rectangle || tool_ == Tool::line) {
      gesture_active_ = true;
      gesture_tool_ = tool_;
      gesture_value_ = selected_tile_;
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
  if (collision_stroke_active_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    auto [x, y] = cursor_cell();
    x = std::clamp(x, 0, document_.level().width - 1);
    y = std::clamp(y, 0, document_.level().height - 1);
    document_.paint_collision_line(previous_x_, previous_y_, x, y,
                                   collision_value_);
    previous_x_ = x;
    previous_y_ = y;
  }
  if (collision_stroke_active_ &&
      ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    auto [x, y] = cursor_cell();
    x = std::clamp(x, 0, document_.level().width - 1);
    y = std::clamp(y, 0, document_.level().height - 1);
    document_.paint_collision_line(previous_x_, previous_y_, x, y,
                                   collision_value_);
    document_.end_collision_stroke();
    collision_stroke_active_ = false;
    mark_dirty_status();
  }
  if (entity_drag_active_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    auto [x, y] = cursor_cell();
    Entity moved;
    const auto &entities = document_.level().entities;
    auto it =
        std::find_if(entities.begin(), entities.end(),
                     [&](const auto &e) { return e.id == entity_drag_id_; });
    if (it != entities.end()) {
      moved = *it;
      moved.x = std::clamp(x, 0, document_.level().width - 1) + 0.5f;
      moved.y = std::clamp(y, 0, document_.level().height - 1) + 0.5f;
      try {
        document_.update_entity(std::move(moved));
        sync_entity_fields();
        mark_dirty_status();
      } catch (const std::exception &e) {
        status_ = e.what();
      }
    }
    entity_drag_active_ = false;
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
                     gesture_value_);
    else
      document_.rectangle(gesture_layer_, gesture_x_, gesture_y_, x, y,
                          gesture_value_);
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
    zoom_ = std::clamp(std::min(size.x / (document_.level().width *
                                          document_.level().tile_width),
                                size.y / (document_.level().height *
                                          document_.level().tile_height)),
                       0.2f, 6.0f);
    cell_x = document_.level().tile_width * zoom_;
    cell_y = document_.level().tile_height * zoom_;
    pan_x_ = (size.x - document_.level().width * cell_x) * 0.5f;
    pan_y_ = (size.y - document_.level().height * cell_y) * 0.5f;
    origin = ImVec2(pos.x + pan_x_, pos.y + pan_y_);
    fit_requested_ = false;
  }
  const int x0 = std::clamp(static_cast<int>(std::floor(-pan_x_ / cell_x)), 0,
                            document_.level().width),
            y0 = std::clamp(static_cast<int>(std::floor(-pan_y_ / cell_y)), 0,
                            document_.level().height);
  const int x1 = std::clamp(
                static_cast<int>(std::ceil((size.x - pan_x_) / cell_x)), 0,
                document_.level().width),
            y1 = std::clamp(
                static_cast<int>(std::ceil((size.y - pan_y_) / cell_y)), 0,
                document_.level().height);
  for (int y = y0; y < y1; ++y)
    for (int x = x0; x < x1; ++x) {
      ImVec2 a(origin.x + x * cell_x, origin.y + y * cell_y),
          b(a.x + cell_x, a.y + cell_y);
      for (const auto &layer : document_.level().layers)
        if (layer.visible) {
          int id = layer.tiles[static_cast<std::size_t>(
              y * document_.level().width + x)];
          if (id)
            draw_tile(draw, tileset_, id, a, b);
        }
      const auto collision_index =
          static_cast<std::size_t>(y * document_.level().width + x);
      if (collision_visible_ && document_.level().collision[collision_index])
        draw->AddRectFilled(a, b, IM_COL32(220, 35, 55, 115));
      if (grid_visible_)
        draw->AddRect(a, b, IM_COL32(40, 53, 59, 160));
    }
  for (const auto &entity : document_.level().entities) {
    ImVec2 center(origin.x + entity.x * cell_x, origin.y + entity.y * cell_y);
    const ImU32 color =
        entity.type == EntityType::player_spawn  ? IM_COL32(70, 210, 130, 255)
        : entity.type == EntityType::npc         ? IM_COL32(80, 170, 255, 255)
        : entity.type == EntityType::hazard      ? IM_COL32(255, 75, 65, 255)
        : entity.type == EntityType::collectible ? IM_COL32(255, 215, 80, 255)
                                                 : IM_COL32(190, 125, 240, 255);
    draw->AddCircleFilled(center, std::max(5.0f, cell_x * 0.23f), color);
    if (entity.id == selected_entity_id_)
      draw->AddCircle(center, std::max(7.0f, cell_x * 0.3f),
                      IM_COL32(255, 255, 255, 255), 0, 2.0f);
  }
  if (gesture_active_) {
    auto [x, y] = cursor_cell();
    ImVec2 a(origin.x + std::min(gesture_x_, x) * cell_x,
             origin.y + std::min(gesture_y_, y) * cell_y),
        b(origin.x + (std::max(gesture_x_, x) + 1) * cell_x,
          origin.y + (std::max(gesture_y_, y) + 1) * cell_y);
    if (gesture_tool_ == Tool::line)
      draw->AddLine(ImVec2(origin.x + (gesture_x_ + 0.5f) * cell_x,
                           origin.y + (gesture_y_ + 0.5f) * cell_y),
                    ImVec2(origin.x + (x + 0.5f) * cell_x,
                           origin.y + (y + 0.5f) * cell_y),
                    IM_COL32(255, 230, 125, 255), 2.0f);
    else
      draw->AddRect(a, b, IM_COL32(255, 230, 125, 255), 0, 0, 2.0f);
  }
  draw->PopClipRect();
  ImGui::End();
}
void Editor::playtest_canvas() {
  ImGui::SetNextWindowPos(ImVec2(278, 88), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(730, 680), ImGuiCond_FirstUseEver);
  ImGui::Begin("Playtest");
  const ImVec2 pos = ImGui::GetCursorScreenPos();
  ImVec2 size = ImGui::GetContentRegionAvail();
  size.x = std::max(size.x, 1.0f);
  size.y = std::max(size.y, 1.0f);
  ImGui::InvisibleButton("runtime-canvas", size);
  const bool focused = ImGui::IsItemFocused();
  const auto now = std::chrono::steady_clock::now();
  const float elapsed =
      std::chrono::duration<float>(now - last_playtest_frame_).count();
  last_playtest_frame_ = now;
  gameplay::Input input;
  if (focused && !ImGui::GetIO().WantTextInput) {
    input.left =
        ImGui::IsKeyDown(ImGuiKey_LeftArrow) || ImGui::IsKeyDown(ImGuiKey_A);
    input.right =
        ImGui::IsKeyDown(ImGuiKey_RightArrow) || ImGui::IsKeyDown(ImGuiKey_D);
    input.up =
        ImGui::IsKeyDown(ImGuiKey_UpArrow) || ImGui::IsKeyDown(ImGuiKey_W);
    input.down =
        ImGui::IsKeyDown(ImGuiKey_DownArrow) || ImGui::IsKeyDown(ImGuiKey_S);
    input.jump = ImGui::IsKeyPressed(ImGuiKey_Space) ||
                 ImGui::IsKeyPressed(ImGuiKey_UpArrow) ||
                 ImGui::IsKeyPressed(ImGuiKey_W);
    input.interact = ImGui::IsKeyPressed(ImGuiKey_E);
  }
  playtest_->advance(elapsed, input);
  const auto &level = playtest_->visual_level();
  const auto &world = playtest_->world();
  const auto &player = playtest_->player();
  if (focused && ImGui::IsKeyPressed(ImGuiKey_F3))
    debug_overlay_ = !debug_overlay_;
  const float cell = std::clamp(std::min(size.x / std::min(level.width, 30),
                                         size.y / std::min(level.height, 20)),
                                8.0f, 40.0f);
  const coordinates::TileCamera camera{
      pos.x + size.x * 0.5f - player.x * cell,
      pos.y + size.y * 0.5f - player.y * cell, cell};
  auto *draw = ImGui::GetWindowDrawList();
  draw->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
  draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      IM_COL32(22, 26, 29, 255));
  for (const auto &layer : level.layers) {
    if (!layer.visible)
      continue;
    for (int y = 0; y < level.height; ++y)
      for (int x = 0; x < level.width; ++x) {
        const int tile =
            layer.tiles[static_cast<std::size_t>(y * level.width + x)];
        if (tile == 0)
          continue;
        const auto a = camera.project(static_cast<float>(x),
                                      static_cast<float>(y));
        const auto b = camera.project(static_cast<float>(x + 1),
                                      static_cast<float>(y + 1));
        draw_tile(draw, tileset_, tile, ImVec2(a.x, a.y), ImVec2(b.x, b.y));
      }
  }
  if (debug_overlay_) {
    for (int y = 0; y < world.height; ++y)
      for (int x = 0; x < world.width; ++x) {
        if (!world.solid(x, y))
          continue;
        const auto a = camera.project(static_cast<float>(x),
                                      static_cast<float>(y));
        const auto b = camera.project(static_cast<float>(x + 1),
                                      static_cast<float>(y + 1));
        draw->AddRect(ImVec2(a.x, a.y), ImVec2(b.x, b.y),
                      IM_COL32(255, 96, 72, 210), 0, 0, 1.5f);
      }
  }
  for (const auto &entity : world.entities) {
    if (entity.type == EntityType::player_spawn ||
        entity.type == EntityType::collectible ||
        entity.type == EntityType::npc || entity.type == EntityType::hazard ||
        entity.type == EntityType::checkpoint ||
        entity.type == EntityType::trigger) {
      const auto projected = camera.project(entity.x, entity.y);
      const ImVec2 center(projected.x, projected.y);
      const ImU32 color =
          entity.type == EntityType::npc           ? IM_COL32(80, 170, 255, 255)
          : entity.type == EntityType::collectible ? IM_COL32(255, 215, 80, 255)
          : entity.type == EntityType::hazard      ? IM_COL32(255, 75, 65, 255)
                                              : IM_COL32(190, 125, 240, 255);
      draw->AddCircleFilled(center, std::max(5.0f, cell * 0.22f), color);
    }
  }
  const auto player_bounds = gameplay::bounds(player);
  const auto player_rect = camera.project(
      player_bounds.left, player_bounds.top, player_bounds.right,
      player_bounds.bottom);
  draw->AddRectFilled(ImVec2(player_rect.left, player_rect.top),
                      ImVec2(player_rect.right, player_rect.bottom),
                      IM_COL32(80, 235, 145, 255), 3.0f);
  if (debug_overlay_) {
    draw->AddRect(ImVec2(player_rect.left, player_rect.top),
                  ImVec2(player_rect.right, player_rect.bottom),
                  IM_COL32(255, 255, 255, 255), 0, 0, 2.0f);
    char details[512];
    const char *mode = level.genre == Genre::platformer ? "platformer"
                                                        : "top_down_rpg";
    char support_text[64], contact_text[64];
    if (player.support_tile_x >= 0)
      std::snprintf(support_text, sizeof(support_text), "tile (%d, %d)",
                    player.support_tile_x, player.support_tile_y);
    else
      std::snprintf(support_text, sizeof(support_text), "none");
    if (player.contact_tile_x >= 0)
      std::snprintf(contact_text, sizeof(contact_text), "tile (%d, %d)",
                    player.contact_tile_x, player.contact_tile_y);
    else
      std::snprintf(contact_text, sizeof(contact_text), "none");
    std::snprintf(details, sizeof(details),
                  "Mode: %s\nPosition: %.3f, %.3f\nVelocity: %.3f, %.3f\n"
                  "Grounded: %s\nSupport: %s\nContact: %s\n"
                  "Normals: (%d, %d)\nCamera origin: %.1f, %.1f\n"
                  "Scale: %.1f px/tile",
                  mode, player.x, player.y, player.velocity_x,
                  player.velocity_y, player.grounded ? "yes" : "no",
                  support_text, contact_text, player.contact_normal_x,
                  player.contact_normal_y, camera.origin_x, camera.origin_y,
                  camera.pixels_per_tile);
    const ImVec2 text_size = ImGui::CalcTextSize(details);
    const ImVec2 box(pos.x + 8, pos.y + 54);
    draw->AddRectFilled(box, ImVec2(box.x + text_size.x + 18,
                                    box.y + text_size.y + 14),
                        IM_COL32(10, 14, 18, 220), 4.0f);
    draw->AddText(ImVec2(box.x + 9, box.y + 7), IM_COL32(220, 240, 255, 255),
                  details);
  }
  draw->AddText(ImVec2(pos.x + 10, pos.y + 10), IM_COL32_WHITE,
                playtest_->paused()
                    ? "Paused"
                    : "Arrows / WASD move | Space jumps | E interacts");
  if (!playtest_->message().empty())
    draw->AddText(ImVec2(pos.x + 10, pos.y + 32), IM_COL32(255, 230, 125, 255),
                  playtest_->message().c_str());
  draw->PopClipRect();
  ImGui::End();
}
void Editor::draw() {
  ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
  toolbar();
  if (playtest_) {
    canvas();
    return;
  }
  ImGui::SetNextWindowPos(ImVec2(8, 88), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(262, 680), ImGuiCond_FirstUseEver);
  ImGui::Begin("Tileset Palette");
  ImGui::Combo("New genre", &genre_, "Top-down RPG\0Platformer\0");
  ImGui::InputInt("New width", &width_);
  ImGui::InputInt("New height", &height_);
  ImGui::InputInt("Cell width", &level_tile_width_);
  ImGui::InputInt("Cell height", &level_tile_height_);
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
  ImGui::SameLine();
  if (ImGui::Button("Save As"))
    save_level_as();
  ImGui::Separator();
  ImGui::InputText("PNG path", tileset_path_, sizeof(tileset_path_));
  ImGui::InputInt("Tile width", &tile_width_);
  ImGui::InputInt("Tile height", &tile_height_);
  if (ImGui::Button("Load PNG")) {
    try {
      Level settings = document_.level();
      settings.tileset_path = tileset_path_;
      settings.tile_width = level_tile_width_;
      settings.tile_height = level_tile_height_;
      settings.atlas_tile_width = tile_width_;
      settings.atlas_tile_height = tile_height_;
      settings.validate();
      tileset_.load(renderer_, tileset_path_, tile_width_, tile_height_);
      document_.set_asset_settings(tileset_path_, level_tile_width_,
                                   level_tile_height_, tile_width_,
                                   tile_height_);
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
  entity_inspector();
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
