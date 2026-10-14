#pragma once
#include "level.hpp"
namespace pf {
class Editor {
 Level level;
 char path[512]="level.json";
 int genre=0, width=32,height=24, selected=1, layer=0;
 float zoom=1.0f, pan_x=20,pan_y=20;
 bool dirty=false;
 std::string status="Ready";
 void canvas();
public:
 void draw();
};
}
