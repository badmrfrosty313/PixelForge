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
      SDL_CreateWindow("PixelForge", 1280, 800, SDL_WINDOW_RESIZABLE),
      SDL_DestroyWindow);
  if (!window)
    throw std::runtime_error(SDL_GetError());
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
  Editor editor(renderer.get());
  if (smoke && !editor.assets_ready())
    throw std::runtime_error("Built-in PNG tileset did not load");
  bool running = true;
  int frames = 0;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT ||
          (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
           event.window.windowID == SDL_GetWindowID(window.get())))
        running = false;
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
    if (smoke && ++frames == 3)
      running = false;
  }
  return 0;
}
} // namespace pf
