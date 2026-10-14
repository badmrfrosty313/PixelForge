#include "rendering.hpp"
namespace pf {
ImU32 tile_color(int id) {
 const ImU32 colors[]={IM_COL32(40,45,55,255),IM_COL32(75,155,80,255),IM_COL32(115,85,60,255),IM_COL32(55,120,195,255),IM_COL32(175,175,185,255)};
 return colors[id%5];
}
}
