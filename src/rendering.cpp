#include "rendering.hpp"
#include <cstdint>
namespace pf {
void draw_tile(ImDrawList *draw_list, const Tileset &tileset, int id,
               ImVec2 top_left, ImVec2 bottom_right) {
  if (!tileset.loaded())
    return;
  if (id < 1 || id > tileset.tile_count()) {
    draw_list->AddRectFilled(top_left, bottom_right,
                             IM_COL32(120, 32, 64, 255));
    draw_list->AddText(top_left, IM_COL32(255, 255, 255, 255), "?");
    return;
  }
  const auto texture = static_cast<ImTextureID>(
      reinterpret_cast<std::intptr_t>(tileset.texture()));
  const auto uv = tileset.region(id);
  draw_list->AddImage(texture, top_left, bottom_right, ImVec2(uv.u0, uv.v0),
                      ImVec2(uv.u1, uv.v1));
}
} // namespace pf
