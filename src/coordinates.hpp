#pragma once

namespace pf::coordinates {
struct Point {
  float x = 0, y = 0;
};

struct Rect {
  float left = 0, top = 0, right = 0, bottom = 0;
};

// Maps tile-unit world coordinates to Dear ImGui screen coordinates. The
// origin is the screen position of world (0,0); scale is logical pixels/tile.
struct TileCamera {
  float origin_x = 0, origin_y = 0, pixels_per_tile = 1;

  Point project(float world_x, float world_y) const {
    return {origin_x + world_x * pixels_per_tile,
            origin_y + world_y * pixels_per_tile};
  }

  Rect project(float left, float top, float right, float bottom) const {
    const Point a = project(left, top);
    const Point b = project(right, bottom);
    return {a.x, a.y, b.x, b.y};
  }
};
} // namespace pf::coordinates
