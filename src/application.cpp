#include "application.hpp"
#include "editor.hpp"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <memory>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <vector>
#include <nlohmann/json.hpp>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
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
int run(bool smoke, const std::filesystem::path &visual_output) {
  const bool visual = !visual_output.empty();
  if (visual)
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
  if (visual) {
    std::filesystem::remove_all(visual_output / "screenshots");
    std::filesystem::remove_all(visual_output / "comparison-diffs");
    std::filesystem::create_directories(visual_output / "screenshots");
    std::filesystem::create_directories(visual_output / "comparison-diffs");
  }
  Runtime runtime;
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
      SDL_CreateWindow("PixelForge Studio", 1440, 900,
                       SDL_WINDOW_RESIZABLE |
                           (visual ? SDL_WindowFlags{0}
                                   : SDL_WINDOW_HIGH_PIXEL_DENSITY)),
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
  nlohmann::json layout_report = {{"schema_version", 1}, {"scenes", nlohmann::json::array()}, {"failures", nlohmann::json::array()}};
  nlohmann::json gameplay_report = {{"schema_version", 1}, {"scenarios", nlohmann::json::array()}, {"failures", nlohmann::json::array()}};
  layout_report["deliberate_overflow_probe_detected"] = Editor::visual_overflow_probe();
  if (!layout_report["deliberate_overflow_probe_detected"])
    layout_report["failures"].push_back({{"message", "Geometry instrumentation missed its deliberate overflow probe"}});
  const auto draw_frame = [&]() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    editor.draw();
    ImGui::Render();
    SDL_SetRenderDrawColor(renderer.get(), 24, 27, 32, 255);
    if (!SDL_RenderClear(renderer.get()))
      throw std::runtime_error(SDL_GetError());
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer.get());
    if (!SDL_RenderPresent(renderer.get()))
      throw std::runtime_error(SDL_GetError());
  };
  if (visual) {
    ImGui::GetIO().IniFilename = nullptr;
    constexpr int widths[] = {1280, 1366, 1662, 1920};
    constexpr int heights[] = {720, 768, 896, 1080};
    constexpr int scales[] = {100, 125, 150, 200};
    struct Scene { const char *name; int genre; bool playtest; bool welcome; int scenario; };
    constexpr Scene scenes[] = {
        {"welcome", 0, false, true, 0}, {"rpg-editor", 0, false, false, 0},
        {"rpg-playtest", 0, true, false, 0}, {"platformer-editor", 1, false, false, 0},
        {"platformer-playtest", 1, true, false, 0}, {"layers-entity-inspector", 0, false, false, 0},
        {"resized-window", 1, false, false, 0}, {"large-map-fit", 0, false, false, 0},
        {"rpg-interaction", 0, true, false, 1}, {"rpg-collectible", 0, true, false, 2},
        {"platformer-grounded", 1, true, false, 3}, {"platformer-hazard-respawn", 1, true, false, 4}};
    for (int resolution = 0; resolution < 4; ++resolution) {
      for (int scale = 0; scale < 4; ++scale) {
        if (!SDL_SetWindowSize(window.get(), widths[resolution], heights[resolution]))
          throw std::runtime_error(SDL_GetError());
        editor.set_display_scale(static_cast<float>(scales[scale]) / 100.0f);
        SDL_PumpEvents();
        for (const auto &scene : scenes) {
          if (std::string_view(scene.name) == "resized-window") {
            if (!SDL_SetWindowSize(window.get(), widths[resolution] * 19 / 20,
                                   heights[resolution] * 19 / 20))
              throw std::runtime_error(SDL_GetError());
            SDL_PumpEvents();
          } else if (std::string_view(scene.name) == "large-map-fit") {
            if (!SDL_SetWindowSize(window.get(), widths[resolution],
                                   heights[resolution]))
              throw std::runtime_error(SDL_GetError());
            SDL_PumpEvents();
          }
          if (scene.welcome) {
            editor.stop_playtest();
            editor.visual_show_welcome(true);
          } else if (std::string_view(scene.name) == "large-map-fit") {
            editor.visual_create_large_map();
          } else {
            editor.visual_load_demo(scene.genre == 0 ? Genre::top_down_rpg : Genre::platformer, scene.playtest);
            if (std::string_view(scene.name) == "layers-entity-inspector")
              editor.visual_select_entity(2);
          }
          if (scene.scenario != 0) {
            using pf::gameplay::Input;
            Input input;
            if (scene.scenario == 1) {
              input.right = true; editor.visual_step_gameplay(input, 45);
              input = {}; input.down = true; editor.visual_step_gameplay(input, 45);
              input = {}; input.interact = true; editor.visual_step_gameplay(input, 1);
            } else if (scene.scenario == 2) {
              input.down = true; editor.visual_step_gameplay(input, 105);
              input = {}; input.right = true; editor.visual_step_gameplay(input, 180);
              input = {}; input.up = true; editor.visual_step_gameplay(input, 60);
            } else if (scene.scenario == 3) {
              editor.visual_step_gameplay({}, 8);
              input.right = true; editor.visual_step_gameplay(input, 18);
              input = {}; input.jump = true; editor.visual_step_gameplay(input, 1);
              input = {}; editor.visual_step_gameplay(input, 25);
              input.right = true; editor.visual_step_gameplay(input, 45);
              input = {}; editor.visual_step_gameplay(input, 20);
            } else if (scene.scenario == 4) {
              editor.visual_step_gameplay({}, 8);
              input.right = true; editor.visual_step_gameplay(input, 120);
            }
          }
          // Let DockBuilder and the renderer settle after resolution/scale changes.
          draw_frame();
          draw_frame();
          int capture_width = 0, capture_height = 0;
          SDL_GetWindowSize(window.get(), &capture_width, &capture_height);
          const std::string filename = std::string(scene.name) + "_" + std::to_string(capture_width) + "x" + std::to_string(capture_height) + "_" + std::to_string(scales[scale]) + "pct.png";
          SDL_Surface *surface = SDL_RenderReadPixels(renderer.get(), nullptr);
          if (!surface)
            throw std::runtime_error(std::string("Screenshot readback failed: ") + SDL_GetError());
          SDL_Surface *rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
          SDL_DestroySurface(surface);
          if (!rgba)
            throw std::runtime_error(std::string("Screenshot conversion failed: ") + SDL_GetError());
          if (rgba->w != capture_width || rgba->h != capture_height) {
            SDL_DestroySurface(rgba);
            throw std::runtime_error("Renderer screenshot size differs from requested window size");
          }
          const auto path = visual_output / "screenshots" / filename;
          if (!stbi_write_png(path.string().c_str(), rgba->w, rgba->h, 4, rgba->pixels, rgba->pitch)) {
            SDL_DestroySurface(rgba);
            throw std::runtime_error("Could not write visual QA PNG");
          }
          SDL_DestroySurface(rgba);
          if (scene.scenario != 0) {
            const auto &player = editor.visual_player();
            const std::string message = editor.visual_gameplay_message();
            bool passed = false;
            std::string assertion;
            if (scene.scenario == 1) {
              passed = message.find("old path is blocked") != std::string::npos;
              assertion = "RPG interaction returns the bundled NPC message";
            } else if (scene.scenario == 2) {
              passed = message == "Collected Moon crystal";
              assertion = "RPG route collects the bundled Moon crystal";
            } else if (scene.scenario == 3) {
              passed = player.grounded && player.x < 9.0f;
              assertion = "Platformer deterministic jump lands on the lower platform";
            } else {
              passed = message.find("hazard") != std::string::npos;
              assertion = "Platformer hazard contact triggers a respawn";
            }
            nlohmann::json result = {{"scenario", scene.name}, {"passed", passed},
                                     {"assertion", assertion}, {"player", {player.x, player.y}},
                                     {"grounded", player.grounded}, {"message", message},
                                     {"screenshot", std::string("screenshots/") + filename}};
            gameplay_report["scenarios"].push_back(result);
            if (!passed) {
              gameplay_report["failures"].push_back(result);
              layout_report["failures"].push_back({{"scene", scene.name},
                                                    {"resolution", {capture_width, capture_height}},
                                                    {"scale", scales[scale]},
                                                    {"message", assertion}});
            }
          }
          nlohmann::json record = {{"name", scene.name}, {"resolution", {capture_width, capture_height}}, {"ui_scale_percent", scales[scale]}, {"screenshot", filename}, {"windows", nlohmann::json::array()}, {"failures", nlohmann::json::array()}};
          record["controls"] = nlohmann::json::array();
          for (const auto &control : editor.visual_controls()) {
            record["controls"].push_back({{"window", control.window},
                                           {"label", control.label},
                                           {"bounds", control.bounds},
                                           {"clip_bounds", control.clip_bounds},
                                           {"visible", control.visible},
                                           {"clipped_horizontally", control.clipped_horizontally}});
            if (control.visible && control.clipped_horizontally)
              record["failures"].push_back(std::string("Visible control clipped horizontally: ") + control.window + "/" + control.label);
          }
          const auto &canvas = editor.visual_geometry();
          if (!scene.playtest) {
            record["canvas"] = {{"bounds", {canvas.canvas_x, canvas.canvas_y,
                                              canvas.canvas_x + canvas.canvas_width,
                                              canvas.canvas_y + canvas.canvas_height}},
                                 {"tilemap_bounds", {canvas.map_x, canvas.map_y,
                                                      canvas.map_x + canvas.map_width,
                                                      canvas.map_y + canvas.map_height}},
                                 {"tilemap_dimensions", {canvas.map_width_tiles,
                                                         canvas.map_height_tiles}},
                                 {"zoom", canvas.zoom},
                                 {"ui_scale_percent", canvas.ui_scale_percent}};
            if (std::string_view(scene.name) == "large-map-fit" &&
                (canvas.map_width > canvas.canvas_width + 1.0f ||
                 canvas.map_height > canvas.canvas_height + 1.0f))
              record["failures"].push_back("Large map does not fit within canvas bounds");
          }
          ImGuiViewport *viewport = ImGui::GetMainViewport();
          const ImVec2 view_min = viewport->Pos, view_max(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y);
          record["dockspace"] = {{"bounds", {viewport->Pos.x, viewport->Pos.y,
                                                viewport->Pos.x + viewport->Size.x,
                                                viewport->Pos.y + viewport->Size.y}},
                                 {"work_size", {viewport->WorkSize.x,
                                                viewport->WorkSize.y}}};
          for (ImGuiWindow *item : ImGui::GetCurrentContext()->Windows) {
            if (!item || item->Hidden || !item->WasActive ||
                item->HiddenFramesCanSkipItems > 0)
              continue;
            const ImVec2 min = item->Pos, max(item->Pos.x + item->Size.x, item->Pos.y + item->Size.y);
            record["windows"].push_back({{"name", item->Name}, {"bounds", {min.x, min.y, max.x, max.y}}, {"content_region", {item->InnerRect.Min.x, item->InnerRect.Min.y, item->InnerRect.Max.x, item->InnerRect.Max.y}}, {"content_extent_bounds", {item->DC.CursorPosPrevLine.x, item->DC.CursorPosPrevLine.y, item->DC.CursorMaxPos.x, item->DC.CursorMaxPos.y}}, {"scroll", {item->Scroll.x, item->Scroll.y}}, {"scroll_max", {item->ScrollMax.x, item->ScrollMax.y}}});
            if (item->Size.x <= 0 || item->Size.y <= 0)
              record["failures"].push_back(std::string("Invalid window geometry: ") + item->Name);
            if (max.x < view_min.x || max.y < view_min.y || min.x > view_max.x || min.y > view_max.y)
              record["failures"].push_back(std::string("Window outside viewport: ") + item->Name);
            if (std::string_view(item->Name).starts_with("Level Canvas") && (item->InnerRect.GetWidth() < 240 || item->InnerRect.GetHeight() < 160))
              record["failures"].push_back("Canvas below 240x160 practical minimum");
          }
          for (const auto &failure : record["failures"])
            layout_report["failures"].push_back({{"scene", scene.name}, {"resolution", {capture_width, capture_height}}, {"scale", scales[scale]}, {"message", failure}});
          layout_report["scenes"].push_back(std::move(record));
        }
      }
    }
    layout_report["passed"] = layout_report["failures"].empty();
    gameplay_report["passed"] = gameplay_report["failures"].empty();
    std::ofstream report(visual_output / "layout-report.json");
    report << layout_report.dump(2) << '\n';
    if (!report)
      throw std::runtime_error("Could not write layout report");
    std::ofstream gameplay(visual_output / "gameplay-report.json");
    gameplay << gameplay_report.dump(2) << '\n';
    if (!gameplay)
      throw std::runtime_error("Could not write gameplay report");
    std::ofstream gallery(visual_output / "index.html");
    gallery << "<!doctype html><meta charset='utf-8'><title>PixelForge Visual QA</title><style>body{font:14px system-ui;background:#20252c;color:#edf2f7}main{display:grid;grid-template-columns:repeat(auto-fit,minmax(360px,1fr));gap:16px}figure{margin:0;background:#303741;padding:8px}img{width:100%;height:auto}</style><h1>PixelForge Visual QA</h1><main>";
    for (const auto &scene : layout_report["scenes"])
      gallery << "<figure><img loading='lazy' src='screenshots/" << scene["screenshot"].get<std::string>() << "'><figcaption>" << scene["name"] << " — " << scene["resolution"][0] << "×" << scene["resolution"][1] << " — " << scene["ui_scale_percent"] << "%</figcaption></figure>";
    gallery << "</main>";
    std::ofstream summary(visual_output / "summary.md");
    summary << "# PixelForge Visual QA\n\nCaptured " << layout_report["scenes"].size() << " real SDL/ImGui frames across four resolutions, four UI scales, and twelve scenes.\n\nLayout/gameplay failures: " << layout_report["failures"].size() << ". See `layout-report.json`, `gameplay-report.json`, and `index.html`.\n";
    if (!layout_report["failures"].empty()) return 1;
    return 0;
  }
  int frames = 0;
  constexpr int smoke_widths[] = {1440, 1280, 1024, 800};
  constexpr int smoke_heights[] = {900, 720, 768, 560};
  constexpr float smoke_scales[] = {1.0f, 1.25f, 1.5f, 2.0f};
  while (running) {
    if (smoke && frames == 1)
      editor.start_smoke_playtest(Genre::top_down_rpg);
    if (smoke && frames == 2) {
      editor.stop_playtest();
      editor.start_smoke_playtest(Genre::top_down_rpg);
    }
    if (smoke && frames == 3) {
      editor.stop_playtest();
      editor.start_smoke_playtest(Genre::platformer);
    }
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
  if (smoke)
    editor.stop_playtest();
  return 0;
}
} // namespace pf
