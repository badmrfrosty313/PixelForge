#include "application.hpp"
#include "editor.hpp"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <memory>
#include <stdexcept>
namespace pf {
namespace {
struct Runtime {
  Runtime() {
    if (!SDL_Init(SDL_INIT_VIDEO))
      throw std::runtime_error(SDL_GetError());
  }
  ~Runtime() { SDL_Quit(); }
};
struct Gui {
  bool platform = false, renderer = false;
  Gui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(12, 10);
    style.FramePadding = ImVec2(9, 6);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.IndentSpacing = 18;
    style.WindowRounding = 7;
    style.ChildRounding = 5;
    style.FrameRounding = 4;
    style.PopupRounding = 5;
    style.ScrollbarRounding = 6;
    style.GrabRounding = 4;
    style.WindowBorderSize = 1;
    style.FrameBorderSize = 0;
    auto &colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.88f, 0.91f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.48f, 0.53f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.075f, 0.088f, 0.105f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.065f, 0.077f, 0.092f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.095f, 0.110f, 0.130f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.21f, 0.25f, 0.80f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.23f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.28f, 0.35f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.095f, 0.115f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.09f, 0.105f, 0.125f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.19f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.29f, 0.37f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.24f, 0.40f, 0.54f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.16f, 0.23f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.21f, 0.31f, 0.41f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.39f, 0.52f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.38f, 0.71f, 0.91f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.32f, 0.62f, 0.82f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.43f, 0.76f, 0.96f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.32f, 0.42f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.16f, 0.24f, 0.32f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.30f, 0.58f, 0.76f, 0.60f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.055f, 0.065f, 0.078f, 1.00f);
  }
  ~Gui() {
    if (renderer)
      ImGui_ImplSDLRenderer3_Shutdown();
    if (platform)
      ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
  }
};
} // namespace
int run(bool smoke) {
  Runtime runtime;
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
      SDL_CreateWindow("PixelForge Studio", 1440, 900,
                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY),
      SDL_DestroyWindow);
  if (!window)
    throw std::runtime_error(SDL_GetError());
  SDL_SetWindowMinimumSize(window.get(), 800, 560);
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
      SDL_CreateRenderer(window.get(), nullptr), SDL_DestroyRenderer);
  if (!renderer)
    throw std::runtime_error(SDL_GetError());
  SDL_SetRenderVSync(renderer.get(), 1);
  Gui gui;
  if (!(gui.platform =
            ImGui_ImplSDL3_InitForSDLRenderer(window.get(), renderer.get())))
    throw std::runtime_error("ImGui SDL initialization failed");
  if (!(gui.renderer = ImGui_ImplSDLRenderer3_Init(renderer.get())))
    throw std::runtime_error("ImGui renderer initialization failed");
  if (smoke)
    ImGui::GetIO().IniFilename = nullptr;
  Editor editor(renderer.get(), SDL_GetWindowDisplayScale(window.get()));
  if (smoke && !editor.assets_ready())
    throw std::runtime_error("Built-in PNG tileset did not load");
  bool running = true;
  int frames = 0;
  constexpr int smoke_widths[] = {1440, 1280, 1024, 800};
  constexpr int smoke_heights[] = {900, 720, 768, 560};
  constexpr float smoke_scales[] = {1.0f, 1.25f, 1.5f, 2.0f};
  while (running) {
    if (smoke && frames == 1)
      editor.start_smoke_playtest(Genre::top_down_rpg);
    if (smoke && frames == 3)
      editor.start_smoke_playtest(Genre::platformer);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT ||
          (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
           event.window.windowID == SDL_GetWindowID(window.get())))
        editor.request_close();
      if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED &&
          event.window.windowID == SDL_GetWindowID(window.get()))
        editor.set_display_scale(SDL_GetWindowDisplayScale(window.get()));
    }
    if (editor.should_close())
      running = false;
    if (!running)
      break;
    if (smoke && frames < 4) {
      editor.set_display_scale(smoke_scales[frames]);
      if (!SDL_SetWindowSize(window.get(), smoke_widths[frames],
                             smoke_heights[frames]))
        throw std::runtime_error(SDL_GetError());
      SDL_PumpEvents();
    }
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    editor.draw();
    ImGui::Render();
    if (smoke && frames > 0 && ImGui::GetDrawData()->TotalVtxCount == 0)
      throw std::runtime_error("Editor produced no geometry");
    SDL_SetRenderDrawColor(renderer.get(), 24, 27, 32, 255);
    if (!SDL_RenderClear(renderer.get()))
      throw std::runtime_error(SDL_GetError());
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer.get());
    if (!SDL_RenderPresent(renderer.get()))
      throw std::runtime_error(SDL_GetError());
    if (editor.should_close())
      running = false;
    if (smoke && ++frames == 4)
      running = false;
  }
  return 0;
}
} // namespace pf
