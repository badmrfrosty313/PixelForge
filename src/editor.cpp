#include "editor.hpp"
#include "serialization.hpp"
#include "rendering.hpp"
#include <imgui.h>
#include <algorithm>
#include <cmath>
namespace pf {
void Editor::draw() {
 ImGui::DockSpaceOverViewport(0,ImGui::GetMainViewport());
 ImGui::SetNextWindowPos(ImVec2(10,10),ImGuiCond_FirstUseEver);
 ImGui::SetNextWindowSize(ImVec2(330,740),ImGuiCond_FirstUseEver);
 ImGui::Begin("Level & Tools");
 ImGui::TextUnformatted("PixelForge | Milestone 1");
 ImGui::Combo("New genre",&genre,"Top-down RPG\0Platformer\0");
 ImGui::InputInt("Width",&width); ImGui::InputInt("Height",&height);
 if(ImGui::Button("New level")) {
  if(dirty) ImGui::OpenPopup("Discard edits?");
  else {try {level=Level(genre==0?Genre::top_down_rpg:Genre::platformer,width,height);layer=0;status="New level";}catch(const std::exception& e){status=e.what();}}
 }
 if(ImGui::BeginPopupModal("Discard edits?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
  ImGui::TextUnformatted("Discard unsaved edits and create a new level?");
  if(ImGui::Button("Discard")) {try{level=Level(genre==0?Genre::top_down_rpg:Genre::platformer,width,height);layer=0;dirty=false;}catch(const std::exception& e){status=e.what();}ImGui::CloseCurrentPopup();}
  ImGui::SameLine(); if(ImGui::Button("Cancel")) ImGui::CloseCurrentPopup(); ImGui::EndPopup();
 }
 ImGui::Separator(); ImGui::Text("Current: %s, %d x %d%s",genre_name(level.genre).c_str(),level.width,level.height,dirty?" *":"");
 ImGui::InputText("JSON path",path,sizeof(path));
 if(ImGui::Button("Save")) {try{save(level,path);dirty=false;status="Saved";}catch(const std::exception& e){status=e.what();}}
 ImGui::SameLine();
 if(ImGui::Button("Load")) {if(dirty) ImGui::OpenPopup("Load and discard?");else {try{level=load(path);layer=0;status="Loaded";}catch(const std::exception& e){status=e.what();}}}
 if(ImGui::BeginPopupModal("Load and discard?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
  ImGui::TextUnformatted("Loading will discard unsaved edits.");
  if(ImGui::Button("Load anyway")){try{auto loaded=load(path);level=std::move(loaded);layer=0;dirty=false;status="Loaded";}catch(const std::exception& e){status=e.what();}ImGui::CloseCurrentPopup();}
  ImGui::SameLine();if(ImGui::Button("Cancel"))ImGui::CloseCurrentPopup();ImGui::EndPopup();
 }
 ImGui::Separator();
 for(int i=0;i<static_cast<int>(level.layers.size());++i) if(ImGui::Selectable(level.layers[i].name.c_str(),layer==i)) layer=i;
 ImGui::SliderInt("Tile ID",&selected,1,255);
 for(int i=1;i<=4;++i) {ImGui::PushID(i);ImGui::PushStyleColor(ImGuiCol_Button,ImGui::ColorConvertU32ToFloat4(tile_color(i)));if(ImGui::Button(std::to_string(i).c_str(),ImVec2(40,30)))selected=i;ImGui::PopStyleColor();ImGui::PopID();ImGui::SameLine();}
 ImGui::NewLine();ImGui::TextWrapped("Left drag: paint | Right drag: erase | Middle drag: pan | Wheel: zoom\nLayers render in order; ID 0 is empty.");
 ImGui::TextWrapped("%s",status.c_str()); ImGui::End();
 canvas();
}
void Editor::canvas() {
 ImGui::SetNextWindowPos(ImVec2(350,10),ImGuiCond_FirstUseEver);
 ImGui::SetNextWindowSize(ImVec2(900,740),ImGuiCond_FirstUseEver);
 ImGui::Begin("Canvas");
 auto pos=ImGui::GetCursorScreenPos(); auto size=ImGui::GetContentRegionAvail();
 size.x=std::max(size.x,1.0f);size.y=std::max(size.y,1.0f);
 ImGui::InvisibleButton("canvas",size,ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonRight|ImGuiButtonFlags_MouseButtonMiddle);
 bool hovered=ImGui::IsItemHovered();auto& io=ImGui::GetIO();
 if(hovered && io.MouseWheel!=0) {
  float next=std::clamp(zoom*std::pow(1.15f,io.MouseWheel),0.25f,4.0f);
  pan_x=(pan_x-(io.MousePos.x-pos.x))*next/zoom+(io.MousePos.x-pos.x);
  pan_y=(pan_y-(io.MousePos.y-pos.y))*next/zoom+(io.MousePos.y-pos.y);zoom=next;
 }
 if(hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)){pan_x+=io.MouseDelta.x;pan_y+=io.MouseDelta.y;}
 float cell=24*zoom; ImVec2 origin(pos.x+pan_x,pos.y+pan_y);
 if(hovered && (ImGui::IsMouseDown(0)||ImGui::IsMouseDown(1))) {
  int x=static_cast<int>(std::floor((io.MousePos.x-origin.x)/cell)),y=static_cast<int>(std::floor((io.MousePos.y-origin.y)/cell));
  if(x>=0&&y>=0&&x<level.width&&y<level.height){int value=ImGui::IsMouseDown(1)?0:selected;if(level.tile(layer,x,y)!=value){level.paint(layer,x,y,value);dirty=true;}}
 }
 auto* d=ImGui::GetWindowDrawList();d->PushClipRect(pos,ImVec2(pos.x+size.x,pos.y+size.y),true);
 d->AddRectFilled(pos,ImVec2(pos.x+size.x,pos.y+size.y),IM_COL32(24,27,32,255));
 int x0=std::clamp(static_cast<int>(std::floor(-pan_x/cell)),0,level.width), y0=std::clamp(static_cast<int>(std::floor(-pan_y/cell)),0,level.height);
 int x1=std::clamp(static_cast<int>(std::ceil((size.x-pan_x)/cell)),0,level.width), y1=std::clamp(static_cast<int>(std::ceil((size.y-pan_y)/cell)),0,level.height);
 for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){
  ImVec2 a(origin.x+x*cell,origin.y+y*cell),b(a.x+cell,a.y+cell);
  for(int l=0;l<static_cast<int>(level.layers.size());++l){int id=level.tile(l,x,y);if(id)d->AddRectFilled(a,b,tile_color(id));}
  d->AddRect(a,b,IM_COL32(90,95,105,120));
 }
 d->PopClipRect();ImGui::End();
}
}
