#include "rendering.hpp"
#include <algorithm>
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

void draw_runtime_entity(ImDrawList *draw, EntityType type, ImVec2 center,
                         float tile_pixels) {
  const float p = std::max(0.5f, tile_pixels / 16.0f);
  const ImU32 dark = IM_COL32(32, 37, 48, 255);
  const ImU32 cream = IM_COL32(244, 229, 192, 255);
  const ImU32 blue = IM_COL32(67, 144, 190, 255);
  switch (type) {
  case EntityType::player_spawn: {
    draw->AddRectFilled(ImVec2(center.x - p, center.y - 5 * p),
                        ImVec2(center.x, center.y + 5 * p), dark);
    draw->AddTriangleFilled(ImVec2(center.x, center.y - 5 * p),
                            ImVec2(center.x + 5 * p, center.y - 3 * p),
                            ImVec2(center.x, center.y - p),
                            IM_COL32(86, 207, 159, 255));
    draw->AddRectFilled(ImVec2(center.x - 3 * p, center.y + 4 * p),
                        ImVec2(center.x + 3 * p, center.y + 5 * p), dark);
    break;
  }
  case EntityType::npc:
  case EntityType::enemy: {
    const bool enemy = type == EntityType::enemy;
    const ImU32 body = enemy ? IM_COL32(155, 62, 82, 255) : blue;
    const ImU32 hair = enemy ? IM_COL32(62, 39, 62, 255)
                             : IM_COL32(73, 49, 57, 255);
    draw->AddRectFilled(ImVec2(center.x - 4 * p, center.y + 2 * p),
                        ImVec2(center.x + 4 * p, center.y + 6 * p), dark);
    draw->AddRectFilled(ImVec2(center.x - 4 * p, center.y - p),
                        ImVec2(center.x + 4 * p, center.y + 4 * p), body);
    draw->AddRectFilled(ImVec2(center.x - 3 * p, center.y - 6 * p),
                        ImVec2(center.x + 3 * p, center.y - p), cream);
    draw->AddRectFilled(ImVec2(center.x - 4 * p, center.y - 7 * p),
                        ImVec2(center.x + 4 * p, center.y - 5 * p), hair);
    if (!enemy)
      draw->AddRectFilled(ImVec2(center.x + p, center.y - 3 * p),
                          ImVec2(center.x + 2 * p, center.y - 2 * p), dark);
    else {
      draw->AddRectFilled(ImVec2(center.x - p, center.y - 3 * p),
                          ImVec2(center.x, center.y - 2 * p),
                          IM_COL32(255, 221, 105, 255));
      draw->AddRectFilled(ImVec2(center.x + p, center.y - 3 * p),
                          ImVec2(center.x + 2 * p, center.y - 2 * p),
                          IM_COL32(255, 221, 105, 255));
    }
    break;
  }
  case EntityType::collectible: {
    const ImVec2 top(center.x, center.y - 6 * p);
    const ImVec2 right(center.x + 4 * p, center.y);
    const ImVec2 bottom(center.x, center.y + 6 * p);
    const ImVec2 left(center.x - 4 * p, center.y);
    draw->AddQuadFilled(top, right, bottom, left,
                        IM_COL32(244, 182, 64, 255));
    draw->AddQuadFilled(ImVec2(center.x, center.y - 3 * p),
                        ImVec2(center.x + 2 * p, center.y),
                        ImVec2(center.x, center.y + 3 * p),
                        ImVec2(center.x - 2 * p, center.y),
                        IM_COL32(255, 237, 166, 255));
    draw->AddRectFilled(ImVec2(center.x - p, center.y - 4 * p),
                        ImVec2(center.x, center.y - 2 * p), cream);
    break;
  }
  case EntityType::checkpoint: {
    draw->AddRectFilled(ImVec2(center.x - 3 * p, center.y + 5 * p),
                        ImVec2(center.x + 3 * p, center.y + 6 * p), dark);
    draw->AddRectFilled(ImVec2(center.x - p, center.y - 6 * p),
                        ImVec2(center.x, center.y + 5 * p), cream);
    draw->AddTriangleFilled(ImVec2(center.x, center.y - 6 * p),
                            ImVec2(center.x + 6 * p, center.y - 4 * p),
                            ImVec2(center.x, center.y - 2 * p),
                            IM_COL32(92, 210, 157, 255));
    break;
  }
  case EntityType::hazard: {
    const ImU32 red = IM_COL32(230, 71, 74, 255);
    for (int i = -1; i <= 1; ++i) {
      const float x = center.x + static_cast<float>(i) * 5 * p;
      draw->AddTriangleFilled(ImVec2(x - 3 * p, center.y + 4 * p),
                              ImVec2(x, center.y - 5 * p),
                              ImVec2(x + 3 * p, center.y + 4 * p), red);
    }
    draw->AddRectFilled(ImVec2(center.x - 8 * p, center.y + 4 * p),
                        ImVec2(center.x + 8 * p, center.y + 6 * p), dark);
    break;
  }
  case EntityType::trigger: {
    draw->AddRect(ImVec2(center.x - 6 * p, center.y - 6 * p),
                  ImVec2(center.x + 6 * p, center.y + 6 * p),
                  IM_COL32(245, 205, 105, 255), 1.0f, 0, 2.0f * p);
    draw->AddRectFilled(ImVec2(center.x - p, center.y - 3 * p),
                        ImVec2(center.x + p, center.y - p), cream);
    draw->AddRectFilled(ImVec2(center.x - p, center.y + p),
                        ImVec2(center.x + p, center.y + 3 * p), cream);
    break;
  }
  }
}

void draw_runtime_player(ImDrawList *draw, ImVec2 hitbox_min,
                         ImVec2 hitbox_max, int facing_x, int facing_y,
                         bool walking, bool jumping, bool falling,
                         int animation_frame) {
  static constexpr char sprite[15][13] = {
      "....HHHH....", "...HHHHHH...", "..HHHHHHHH..", ".HHHHSSHHHH.",
      ".HSSSSSSSSH.", "..SSSESSSS..", "..SSSSSSSS..", "...SGGGG...",
      "..CCCGGCCC..", ".CCCCGGCCCC.", ".CCCCCcCCCC.", "..CCCccCCC..",
      "...BB..BB...", "..BBB..BBB..", "..BB....BB.."};
  const float pixel = std::min((hitbox_max.x - hitbox_min.x) / 12.0f,
                               (hitbox_max.y - hitbox_min.y) / 15.0f);
  const float width = pixel * 12.0f, height = pixel * 15.0f;
  const float x0 = (hitbox_min.x + hitbox_max.x - width) * 0.5f;
  const float y0 = hitbox_max.y - height;
  const bool idle = !walking && !jumping && !falling;
  const float bob = (walking || idle) && animation_frame % 2 != 0 ? -pixel
                                                                  : 0.0f;
  const ImU32 hair = IM_COL32(58, 39, 49, 255);
  const ImU32 skin = IM_COL32(238, 189, 143, 255);
  const ImU32 eye = IM_COL32(38, 43, 53, 255);
  const ImU32 coat = IM_COL32(61, 129, 164, 255);
  const ImU32 coat_shadow = IM_COL32(40, 81, 108, 255);
  const ImU32 gold = IM_COL32(246, 190, 82, 255);
  const ImU32 boots = IM_COL32(62, 49, 58, 255);
  for (int row = 0; row < 15; ++row)
    for (int column = 0; column < 12; ++column) {
      const char code = sprite[row][column];
      if (code == '.')
        continue;
      int drawn_column = facing_x < 0 ? 11 - column : column;
      float x = x0 + static_cast<float>(drawn_column) * pixel;
      float y = y0 + static_cast<float>(row) * pixel + bob;
      if (walking && row >= 13 && ((column + animation_frame) % 2 == 0))
        y -= pixel;
      if (jumping && row == 13 && (column == 2 || column == 3 || column == 8 ||
                                   column == 9))
        y -= pixel;
      if (falling && row == 14 && (column == 2 || column == 3 || column == 8 ||
                                  column == 9))
        y -= pixel;
      ImU32 color = code == 'H' ? hair
                    : code == 'S' ? skin
                    : code == 'E' ? eye
                    : code == 'C' ? coat
                    : code == 'c' ? coat_shadow
                    : code == 'G' ? gold
                                   : boots;
      if (facing_y < 0 && row == 5 && (code == 'S' || code == 'E'))
        color = hair;
      const ImVec2 a(x, y), b(x + pixel, y + pixel);
      draw->AddRectFilled(a, b, color);
      if (code == 'S' && row == 4 && (column == 3 || column == 8))
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + pixel, y + pixel * 0.35f),
                            IM_COL32(255, 223, 178, 255));
    }
}
} // namespace pf
