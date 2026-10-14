#pragma once
#include "tileset.hpp"
#include "level.hpp"
#include <imgui.h>
namespace pf {
void draw_tile(ImDrawList *draw_list, const Tileset &tileset, int id,
               ImVec2 top_left, ImVec2 bottom_right);
void draw_runtime_entity(ImDrawList *draw_list, EntityType type, ImVec2 center,
                         float tile_pixels);
void draw_runtime_player(ImDrawList *draw_list, ImVec2 hitbox_min,
                         ImVec2 hitbox_max, int facing_x, int facing_y,
                         bool walking, bool jumping, bool falling,
                         int animation_frame);
}
