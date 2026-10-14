#include "application.hpp"
#include "editor.hpp"
#include "serialization.hpp"
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
#include <cmath>
#include <chrono>
#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
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

class InteractionDriver {
  enum class ActionKind { click, window_close, resize_scale, move_right,
                          cancel_dialog };
  struct Action {
    ActionKind kind;
    std::string control;
  };
  std::string scenario_;
  std::filesystem::path output_;
  std::filesystem::path temp_level_;
  std::string original_level_contents_;
  std::vector<Action> actions_;
  std::size_t next_action_ = 0;
  std::string verify_control_;
  int frame_ = 0, release_wait_ = 0, move_frames_ = 0, layout_wait_ = 0,
      startup_wait_ = 3;
  bool mouse_down_queued_ = false, key_down_ = false, done_ = false;
  bool hover_pending_ = false;
  float mouse_x_ = 0, mouse_y_ = 0;
  bool expect_exit_ = false, expect_save_ = false, expect_dirty_ = false;
  bool active_capture_ = false, restored_capture_ = false;
  bool played_ = false, moved_ = false;
  float player_x_before_move_ = 0;
  std::size_t initial_layer_count_ = 0;
  std::chrono::steady_clock::time_point started_ =
      std::chrono::steady_clock::now();

  static void queue_mouse(SDL_Window *window, float x, float y,
                          bool down) {
    const auto id = SDL_GetWindowID(window);
    SDL_WarpMouseInWindow(window, x, y);
    SDL_Event motion{};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.windowID = id;
    motion.motion.x = x;
    motion.motion.y = y;
    if (!SDL_PushEvent(&motion))
      throw std::runtime_error(SDL_GetError());
    SDL_Event button{};
    button.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN
                       : SDL_EVENT_MOUSE_BUTTON_UP;
    button.button.windowID = id;
    button.button.button = SDL_BUTTON_LEFT;
    button.button.down = down;
    button.button.clicks = 1;
    button.button.x = x;
    button.button.y = y;
    if (!SDL_PushEvent(&button))
      throw std::runtime_error(SDL_GetError());
  }
  static void queue_hover(SDL_Window *window, float x, float y) {
    SDL_WarpMouseInWindow(window, x, y);
    SDL_Event motion{};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.windowID = SDL_GetWindowID(window);
    motion.motion.x = x;
    motion.motion.y = y;
    if (!SDL_PushEvent(&motion))
      throw std::runtime_error(SDL_GetError());
  }
  static void queue_close(SDL_Window *window) {
    SDL_Event event{};
    event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    event.window.windowID = SDL_GetWindowID(window);
    if (!SDL_PushEvent(&event))
      throw std::runtime_error(SDL_GetError());
  }
  static void queue_key(SDL_Window *window, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.windowID = SDL_GetWindowID(window);
    event.key.key = SDLK_RIGHT;
    event.key.scancode = SDL_SCANCODE_RIGHT;
    event.key.mod = SDL_KMOD_NONE;
    event.key.down = down;
    event.key.repeat = false;
    if (!SDL_PushEvent(&event))
      throw std::runtime_error(SDL_GetError());
  }
  const VisualControlGeometry *find_control(const Editor &editor,
                                             std::string_view label) const {
    for (const auto &control : editor.visual_controls())
      if (control.label == label && control.visible &&
          control.bounds[2] > control.bounds[0] &&
          control.bounds[3] > control.bounds[1])
        return &control;
    return nullptr;
  }
  void capture(SDL_Renderer *renderer, const std::filesystem::path &path) {
    if (output_.empty())
      return;
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, nullptr);
    if (!surface)
      throw std::runtime_error(std::string("Interaction screenshot read failed: ") + SDL_GetError());
    SDL_Surface *rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);
    if (!rgba)
      throw std::runtime_error(std::string("Interaction screenshot conversion failed: ") + SDL_GetError());
    const bool written = stbi_write_png(path.string().c_str(), rgba->w, rgba->h,
                                       4, rgba->pixels, rgba->pitch) != 0;
    SDL_DestroySurface(rgba);
    if (!written)
      throw std::runtime_error("Could not write interaction screenshot");
  }
  bool is_transition() const {
    return scenario_ == "transition-rpg" ||
           scenario_ == "transition-platformer";
  }
  bool is_dirty_scenario() const {
    return scenario_.starts_with("file-dirty-") ||
           scenario_.starts_with("window-dirty-") ||
           scenario_ == "dirty-playtest-discard";
  }
  void configure(Editor &editor) {
    editor.visual_enable_interaction_test();
    if (is_transition()) {
      const auto genre = scenario_ == "transition-rpg"
                             ? Genre::top_down_rpg : Genre::platformer;
      editor.visual_load_demo(genre, false);
      expect_dirty_ = true;
      actions_ = {{ActionKind::click, "Add layer"},
                  {ActionKind::click, "Play"},
                  {ActionKind::move_right, {}},
                  {ActionKind::click, "Pause/Resume"},
                  {ActionKind::click, "Reset"},
                  {ActionKind::resize_scale, {}},
                  {ActionKind::click, "Stop"},
                  {ActionKind::click, "Play"},
                  {ActionKind::click, "Stop"}};
      return;
    }
    if (scenario_ == "exit-file-clean") {
      actions_ = {{ActionKind::click, "File menu"},
                  {ActionKind::click, "Exit menu item"}};
      expect_exit_ = true;
    } else if (scenario_ == "exit-window-clean") {
      actions_ = {{ActionKind::window_close, {}}};
      expect_exit_ = true;
    } else if (scenario_ == "exit-playtest") {
      editor.visual_load_demo(Genre::top_down_rpg, false);
      actions_ = {{ActionKind::click, "Play"},
                  {ActionKind::window_close, {}}};
      expect_exit_ = true;
    } else if (is_dirty_scenario()) {
      const auto source = std::filesystem::path(PIXELFORGE_SOURCE_ASSET_DIR)
                              .parent_path() / "examples/rpg.json";
      temp_level_ = std::filesystem::temp_directory_path() /
                    ("PixelForge-interaction-" + scenario_ + ".json");
      std::filesystem::copy_file(source, temp_level_,
                                 std::filesystem::copy_options::overwrite_existing);
      std::ifstream initial(temp_level_, std::ios::binary);
      original_level_contents_.assign(std::istreambuf_iterator<char>(initial),
                                      std::istreambuf_iterator<char>());
      editor.visual_load_path(temp_level_);
      const auto level = load(temp_level_);
      initial_layer_count_ = level.layers.size();
      if (scenario_ == "dirty-playtest-discard") {
        actions_ = {{ActionKind::click, "Add layer"},
                    {ActionKind::click, "Play"},
                    {ActionKind::window_close, {}},
                    {ActionKind::click, "Discard and exit"}};
        expect_exit_ = true;
      } else if (scenario_ == "window-dirty-discard") {
        actions_ = {{ActionKind::click, "Add layer"},
                    {ActionKind::window_close, {}},
                    {ActionKind::click, "Discard and exit"}};
        expect_exit_ = true;
      } else if (scenario_ == "window-dirty-save") {
        actions_ = {{ActionKind::click, "Add layer"},
                    {ActionKind::window_close, {}},
                    {ActionKind::click, "Save and exit"}};
        expect_exit_ = true;
        expect_save_ = true;
      } else if (scenario_ == "window-dirty-cancel") {
        actions_ = {{ActionKind::click, "Add layer"},
                    {ActionKind::window_close, {}},
                    {ActionKind::click, "Cancel close"}};
        expect_dirty_ = true;
      } else if (scenario_ == "file-dirty-cancel") {
        actions_ = {{ActionKind::click, "Add layer"},
                    {ActionKind::click, "File menu"},
                    {ActionKind::click, "Exit menu item"},
                    {ActionKind::click, "Cancel close"}};
        expect_dirty_ = true;
      } else if (scenario_ == "file-dirty-save") {
        actions_ = {{ActionKind::click, "Add layer"},
                    {ActionKind::click, "File menu"},
                    {ActionKind::click, "Exit menu item"},
                    {ActionKind::click, "Save and exit"}};
        expect_exit_ = true;
        expect_save_ = true;
      } else {
        throw std::runtime_error("Unknown dirty interaction scenario: " + scenario_);
      }
      return;
    } else if (scenario_ == "save-dialog-cancel") {
      editor.visual_create_large_map();
      initial_layer_count_ = 1;
      actions_ = {{ActionKind::click, "Add layer"},
                  {ActionKind::click, "File menu"},
                  {ActionKind::click, "Exit menu item"},
                  {ActionKind::click, "Save and exit"},
                  {ActionKind::cancel_dialog, {}},
                  {ActionKind::click, "Cancel close"}};
      expect_dirty_ = true;
      return;
    } else {
      throw std::runtime_error("Unknown interaction-test scenario: " + scenario_);
    }
  }
  void validate_complete(const Editor &editor) {
    if (expect_exit_ && !editor.should_close())
      throw std::runtime_error("Interaction scenario did not close the application");
    if (!expect_exit_ && editor.should_close())
      throw std::runtime_error("Cancel/stop scenario unexpectedly requested application close");
    if (expect_dirty_ && !editor.visual_document_dirty())
      throw std::runtime_error("Interaction scenario lost unsaved document edits");
    if (expect_save_) {
      if (editor.visual_document_dirty())
        throw std::runtime_error("Save and Exit closed with a dirty document");
      const auto saved = load(temp_level_);
      if (saved.layers.size() != initial_layer_count_ + 1 ||
          saved.layers.back().name != "Layer")
        throw std::runtime_error("Save and Exit did not persist the added layer");
    }
    if ((scenario_ == "exit-playtest" ||
         scenario_ == "dirty-playtest-discard") &&
        !editor.visual_playtest_active())
      throw std::runtime_error("Close request was not exercised during playtest");
    if (!temp_level_.empty() && !expect_save_) {
      std::ifstream current(temp_level_, std::ios::binary);
      const std::string contents{std::istreambuf_iterator<char>(current),
                                 std::istreambuf_iterator<char>()};
      if (contents != original_level_contents_)
        throw std::runtime_error("Discard/Cancel changed the saved document");
    }
    if (is_transition()) {
      if (editor.visual_playtest_active() ||
          !find_control(editor, "Play") || find_control(editor, "Stop"))
        throw std::runtime_error("Stop did not restore the editor toolbar");
      if (!editor.visual_document_dirty())
        throw std::runtime_error("Playtest transition lost the unsaved editor change");
      if (output_.empty() || !active_capture_ || !restored_capture_)
        throw std::runtime_error("Transition screenshots were not captured");
      if (!moved_)
        throw std::runtime_error("SDL keyboard input did not move the playtest character");
    }
    done_ = true;
  }
  void verify_click(const Editor &editor) {
    if (verify_control_.empty())
      return;
    const auto control = std::exchange(verify_control_, {});
    if (control == "Add layer" && !editor.visual_document_dirty())
      throw std::runtime_error("Add layer UI input did not modify the document");
    if (control == "Play" && !editor.visual_playtest_active())
      throw std::runtime_error("Play UI input did not start a session");
    if (control == "Stop" && editor.visual_playtest_active())
      throw std::runtime_error("Stop UI input did not return to editor mode");
    if (control == "File menu" && !find_control(editor, "Exit menu item"))
      throw std::runtime_error("File menu did not open from SDL mouse input");
    if (control == "Exit menu item" && !editor.should_close() &&
        !find_control(editor, "Cancel close") &&
        !editor.visual_exit_menu_activated()) {
      throw std::runtime_error("Exit did not close or queue the unsaved-changes prompt");
    }
    if (control == "Discard and exit" && !editor.should_close())
      throw std::runtime_error("Discard and Exit did not request shutdown");
    if (control == "Save and exit" && !editor.should_close() &&
        !editor.visual_dialog_pending())
      throw std::runtime_error("Save and Exit did not save or request a Save dialog");
    if (control == "Cancel close" &&
        (!find_control(editor, "Play") || editor.should_close()))
      throw std::runtime_error("Cancel did not return to an interactive editor");
  }

public:
  InteractionDriver(std::string_view scenario, std::filesystem::path output,
                    Editor &editor)
      : scenario_(scenario), output_(std::move(output)) {
    configure(editor);
    if (!output_.empty())
      std::filesystem::create_directories(output_);
  }
  ~InteractionDriver() {
    if (!temp_level_.empty()) {
      std::error_code error;
      std::filesystem::remove(temp_level_, error);
    }
  }
  bool done() const noexcept { return done_; }
  void on_application_close(Editor &editor) {
    if (!expect_exit_)
      throw std::runtime_error("Application closed during a cancel or transition test");
    if (next_action_ < actions_.size())
      throw std::runtime_error("Application exited before the interaction script completed");
    validate_complete(editor);
  }
  void after_present(Editor &editor, SDL_Window *window,
                     SDL_Renderer *renderer) {
    ++frame_;
    if (std::chrono::steady_clock::now() - started_ > std::chrono::seconds(12))
      throw std::runtime_error("Timed out waiting for user-driven interaction test");
    if (is_transition()) {
      if (editor.visual_playtest_active() && find_control(editor, "Stop") &&
          !active_capture_) {
        capture(renderer, output_ / "playtest-active.png");
        active_capture_ = true;
      } else if (active_capture_ && !editor.visual_playtest_active() &&
                 find_control(editor, "Play") && !restored_capture_) {
        capture(renderer, output_ / "editor-restored-after-stop.png");
        restored_capture_ = true;
      }
    }
    verify_click(editor);
    if (done_)
      return;
    if (hover_pending_) {
      queue_mouse(window, mouse_x_, mouse_y_, true);
      hover_pending_ = false;
      mouse_down_queued_ = true;
      return;
    }
    if (startup_wait_ > 0) {
      --startup_wait_;
      return;
    }
    if (mouse_down_queued_) {
      const auto &action = actions_.at(next_action_);
      queue_mouse(window, mouse_x_, mouse_y_, false);
      if (action.kind == ActionKind::click)
        verify_control_ = action.control;
      mouse_down_queued_ = false;
      ++next_action_;
      release_wait_ = 1;
      return;
    }
    if (release_wait_ > 0) {
      --release_wait_;
      return;
    }
    if (next_action_ >= actions_.size()) {
      if (expect_exit_) {
        if (frame_ > 12)
          throw std::runtime_error("Exit action did not terminate the application");
        return;
      }
      if (!find_control(editor, "Play"))
        return;
      validate_complete(editor);
      return;
    }
    auto &action = actions_[next_action_];
    switch (action.kind) {
    case ActionKind::window_close:
      queue_close(window);
      ++next_action_;
      release_wait_ = 1;
      return;
    case ActionKind::resize_scale:
      if (layout_wait_ == 0) {
        if (!SDL_SetWindowSize(window, 1280, 720))
          throw std::runtime_error(SDL_GetError());
        editor.set_display_scale(1.5f);
        layout_wait_ = 4;
      }
      if (--layout_wait_ == 0)
        ++next_action_;
      return;
    case ActionKind::move_right:
      if (move_frames_ == 0 && !key_down_) {
        player_x_before_move_ = editor.visual_player().x;
        queue_key(window, true);
        key_down_ = true;
        move_frames_ = 12;
      } else if (move_frames_ > 0) {
        SDL_Delay(17);
        if (--move_frames_ == 0) {
          queue_key(window, false);
          key_down_ = false;
          moved_ = editor.visual_player().x > player_x_before_move_ + 0.02f;
          if (!moved_)
            throw std::runtime_error("Playtest did not respond to SDL right-arrow input");
          ++next_action_;
          release_wait_ = 1;
        }
      }
      return;
    case ActionKind::cancel_dialog:
      if (editor.visual_dialog_pending()) {
        editor.visual_cancel_pending_dialog_for_test();
        ++next_action_;
        release_wait_ = 1;
      }
      return;
    case ActionKind::click: {
      const auto *control = find_control(editor, action.control);
      if (!control)
        return;
      const float x = (control->bounds[0] + control->bounds[2]) * 0.5f;
      const float y = (control->bounds[1] + control->bounds[3]) * 0.5f;
      queue_hover(window, x, y);
      mouse_x_ = x;
      mouse_y_ = y;
      hover_pending_ = true;
      return;
    }
    }
  }
};
} // namespace
int run(bool smoke, const std::filesystem::path &visual_output,
        std::string_view interaction_test,
        const std::filesystem::path &interaction_output) {
  const bool visual = !visual_output.empty();
  const bool interaction = !interaction_test.empty();
  if (interaction && (smoke || visual))
    throw std::invalid_argument("Interaction tests cannot run with smoke/visual modes");
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
  SDL_SetRenderVSync(renderer.get(), interaction ? 0 : 1);
  Gui gui;
  if (!(gui.platform =
            ImGui_ImplSDL3_InitForSDLRenderer(window.get(), renderer.get())))
    throw std::runtime_error("ImGui SDL initialization failed");
  if (!(gui.renderer = ImGui_ImplSDLRenderer3_Init(renderer.get())))
    throw std::runtime_error("ImGui renderer initialization failed");
  if (smoke)
    ImGui::GetIO().IniFilename = nullptr;
  Editor editor(renderer.get(), SDL_GetWindowDisplayScale(window.get()));
  std::unique_ptr<InteractionDriver> interaction_driver;
  if (interaction)
    interaction_driver = std::make_unique<InteractionDriver>(
        interaction_test, interaction_output, editor);
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
          std::size_t scene_assertions = 0;
          record["controls"] = nlohmann::json::array();
          for (const auto &control : editor.visual_controls()) {
            record["controls"].push_back({{"window", control.window},
                                           {"label", control.label},
                                           {"bounds", control.bounds},
                                           {"clip_bounds", control.clip_bounds},
                                           {"visible", control.visible},
                                           {"clipped_horizontally", control.clipped_horizontally},
                                           {"clipped_vertically", control.clipped_vertically},
                                           {"vertically_scrollable", control.vertically_scrollable}});
            ++scene_assertions;
            if (control.visible && control.clipped_horizontally)
              record["failures"].push_back(std::string("Visible control clipped horizontally: ") + control.window + "/" + control.label);
            ++scene_assertions;
            if (control.clipped_vertically && !control.vertically_scrollable)
              record["failures"].push_back(std::string("Control clipped vertically without a scrollable parent: ") + control.window + "/" + control.label);
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
            const std::string_view scene_name(scene.name);
            const bool should_fit = scene_name == "rpg-editor" ||
                                    scene_name == "platformer-editor" ||
                                    scene_name == "large-map-fit";
            if (should_fit) {
              const float map_right = canvas.map_x + canvas.map_width;
              const float map_bottom = canvas.map_y + canvas.map_height;
              const float map_center_x = canvas.map_x + canvas.map_width * 0.5f;
              const float map_center_y = canvas.map_y + canvas.map_height * 0.5f;
              const float canvas_center_x = canvas.canvas_x + canvas.canvas_width * 0.5f;
              const float canvas_center_y = canvas.canvas_y + canvas.canvas_height * 0.5f;
              record["canvas"]["map_padding"] = {
                  canvas.map_x - canvas.canvas_x,
                  canvas.map_y - canvas.canvas_y,
                  canvas.canvas_x + canvas.canvas_width - map_right,
                  canvas.canvas_y + canvas.canvas_height - map_bottom};
              record["canvas"]["center_offset"] = {
                  map_center_x - canvas_center_x, map_center_y - canvas_center_y};
              ++scene_assertions;
              if (canvas.map_x < canvas.canvas_x - 1.0f ||
                  canvas.map_y < canvas.canvas_y - 1.0f ||
                  map_right > canvas.canvas_x + canvas.canvas_width + 1.0f ||
                  map_bottom > canvas.canvas_y + canvas.canvas_height + 1.0f)
                record["failures"].push_back("Initial tilemap framing crops the level");
              ++scene_assertions;
              if (std::abs(map_center_x - canvas_center_x) > 2.0f ||
                  std::abs(map_center_y - canvas_center_y) > 2.0f)
                record["failures"].push_back("Initial tilemap framing is not centered");
            }
          }
          ImGuiViewport *viewport = ImGui::GetMainViewport();
          const ImVec2 view_min = viewport->Pos, view_max(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y);
          record["dockspace"] = {{"bounds", {viewport->Pos.x, viewport->Pos.y,
                                                viewport->Pos.x + viewport->Size.x,
                                                viewport->Pos.y + viewport->Size.y}},
                                 {"work_size", {viewport->WorkSize.x,
                                                viewport->WorkSize.y}}};
          record["viewport_aspect_ratio"] = capture_height > 0
                                                  ? static_cast<float>(capture_width) / capture_height
                                                  : 0.0f;
          ++scene_assertions;
          if (capture_height <= 0 || capture_width / static_cast<float>(capture_height) < 1.25f ||
              capture_width / static_cast<float>(capture_height) > 2.1f)
            record["failures"].push_back("Viewport aspect ratio is outside the supported QA range");
          for (ImGuiWindow *item : ImGui::GetCurrentContext()->Windows) {
            if (!item || item->Hidden || !item->WasActive ||
                item->HiddenFramesCanSkipItems > 0)
              continue;
            const ImVec2 min = item->Pos, max(item->Pos.x + item->Size.x, item->Pos.y + item->Size.y);
            record["windows"].push_back({{"name", item->Name}, {"bounds", {min.x, min.y, max.x, max.y}}, {"content_region", {item->InnerRect.Min.x, item->InnerRect.Min.y, item->InnerRect.Max.x, item->InnerRect.Max.y}}, {"content_extent_bounds", {item->DC.CursorPosPrevLine.x, item->DC.CursorPosPrevLine.y, item->DC.CursorMaxPos.x, item->DC.CursorMaxPos.y}}, {"scroll", {item->Scroll.x, item->Scroll.y}}, {"scroll_max", {item->ScrollMax.x, item->ScrollMax.y}}});
            ++scene_assertions;
            if (item->Size.x <= 0 || item->Size.y <= 0)
              record["failures"].push_back(std::string("Invalid window geometry: ") + item->Name);
            ++scene_assertions;
            if (max.x < view_min.x || max.y < view_min.y || min.x > view_max.x || min.y > view_max.y)
              record["failures"].push_back(std::string("Window outside viewport: ") + item->Name);
            if (std::string_view(item->Name).starts_with("Level Canvas")) {
              ++scene_assertions;
              if (item->InnerRect.GetWidth() < 240 || item->InnerRect.GetHeight() < 160)
                record["failures"].push_back("Canvas below 240x160 practical minimum");
            }
            const float window_width = max.x - min.x;
            const float window_height = max.y - min.y;
            if (std::string_view(item->Name).starts_with("Toolbar")) {
              ++scene_assertions;
              if (window_height > capture_height * 0.28f)
                record["failures"].push_back("Toolbar consumes more than 28% of the viewport height");
            }
            if (std::string_view(item->Name).starts_with("Status")) {
              ++scene_assertions;
              if (window_height > capture_height * 0.23f)
                record["failures"].push_back("Status panel consumes more than 23% of the viewport height");
            }
            if (std::string_view(item->Name).starts_with("Tileset Palette") ||
                std::string_view(item->Name).starts_with("Layers") ||
                std::string_view(item->Name).starts_with("Entities")) {
              ++scene_assertions;
              if (window_width < 190.0f || window_width > capture_width * 0.42f)
                record["failures"].push_back(std::string("Sidebar width is not usable: ") + item->Name);
            }
          }
          record["layout_assertions"] = scene_assertions;
          for (const auto &failure : record["failures"])
            layout_report["failures"].push_back({{"scene", scene.name}, {"resolution", {capture_width, capture_height}}, {"scale", scales[scale]}, {"message", failure}});
          layout_report["scenes"].push_back(std::move(record));
        }
      }
    }
    layout_report["layout_assertion_count"] = 1;
    for (const auto &scene : layout_report["scenes"])
      layout_report["layout_assertion_count"] =
          layout_report["layout_assertion_count"].get<std::size_t>() +
          scene["layout_assertions"].get<std::size_t>();
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
    summary << "# PixelForge Visual QA\n\nCaptured " << layout_report["scenes"].size() << " real SDL/ImGui frames across four resolutions, four UI scales, and twelve scenes.\n\nLayout assertions: " << layout_report["layout_assertion_count"] << ". Layout/gameplay failures: " << layout_report["failures"].size() << ". The report includes crop/centering, panel and canvas geometry, toolbar/status size, control clipping, and intentional scroll checks. See `layout-report.json`, `gameplay-report.json`, and `index.html`.\n";
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
           (event.window.windowID == SDL_GetWindowID(window.get()) ||
            event.window.windowID == 0)))
        editor.request_close();
      if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED &&
          event.window.windowID == SDL_GetWindowID(window.get()))
        editor.set_display_scale(SDL_GetWindowDisplayScale(window.get()));
    }
    if (editor.should_close()) {
      if (interaction_driver)
        interaction_driver->on_application_close(editor);
      running = false;
    }
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
    if (interaction_driver) {
      interaction_driver->after_present(editor, window.get(), renderer.get());
      if (interaction_driver->done())
        running = false;
    }
    if (editor.should_close()) {
      if (interaction_driver)
        interaction_driver->on_application_close(editor);
      running = false;
    }
    if (smoke && ++frames == 4)
      running = false;
  }
  if (smoke)
    editor.stop_playtest();
  return 0;
}
} // namespace pf
