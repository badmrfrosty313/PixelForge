#pragma once
#include "tileset.hpp"
#include <imgui.h>
namespace pf {
void draw_tile(ImDrawList *draw_list, const Tileset &tileset, int id,
               ImVec2 top_left, ImVec2 bottom_right);
}
