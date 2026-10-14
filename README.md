# PixelForge

A C++20 native 2D level editor for top-down RPGs and platformers. Both genres use the same tile canvas, layer model and JSON format. Milestone 1 contains editing infrastructure, not gameplay simulation.

## Windows build

Install Visual Studio 2022 with **Desktop development with C++**, Git, and CMake 3.24+. In a Developer PowerShell, from the checkout root:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
.\build\Release\PixelForge.exe
```

CMake FetchContent downloads pinned SDL 3.2.10, Dear ImGui 1.91.9b docking, and nlohmann JSON 3.12.0 sources from GitHub. SDL is linked statically; no separately installed SDL DLL is required. Internet and Git are needed for the first configure; build/_deps retains dependencies. Dependency licenses are in the fetched source directories. GitHub Actions builds/tests Linux and Windows and uploads the Windows executable.

Linux requires GCC 12+/Clang with C++20, CMake, Git, a build tool and desktop development libraries (on Debian: libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev libxtst-dev).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/PixelForge
```

On a displayless Linux host add `-DSDL_UNIX_CONSOLE_BUILD=ON` at configure; this permits an SDL build without desktop libraries. `SDL_VIDEODRIVER=dummy ./build/PixelForge --smoke` renders three frames using the real SDL/ImGui backends and exits. It does not validate interactive desktop behavior. Core-only development: configure with `-DPIXELFORGE_EDITOR=OFF`.

## Editing

The Level & Tools panel creates maps from 1–256 cells per dimension with either `top_down_rpg` or `platformer` genre. Select a tile ID (1–255), then left-drag on Canvas to paint or right-drag to erase to ID 0. Middle-drag pans; the wheel zooms around the cursor. Colored tiles are placeholders for future tile art. Panels can be docked by dragging their title bars. Layout persists in imgui.ini in the working directory.

Enter a JSON path and click Save or Load. Relative paths resolve from the directory where the app was launched; from the checkout root try `examples/rpg.json` and `examples/platformer.json`. Loading and creating a new map prompt before discarding edits. Closing the application currently does not prompt, so save before exiting. Save overwrites the named file directly; keep backups for important maps.

Layers are composited in order, and the selected layer receives edits. New maps have one Ground layer; files may contain up to 16 layers. Layer creation/reordering is not yet exposed. Errors appear in the tools panel; a failed load preserves the current level.

## Architecture

- `application`: RAII SDL window/renderer and ImGui lifecycle, event/render loop.
- `editor`: workspace, tools, input, zoom/pan and active document.
- `rendering`: tile palette, with canvas draw commands in the editor.
- `level`: shared tile/layer model and checked operations, independent of SDL.
- `serialization`: versioned JSON validation and filesystem I/O, independent of UI.
- `gameplay.hpp`: reserved boundary for future genre rules; no physics or simulation yet.

JSON version 1 stores `genre`, integer `width`/`height`, and `layers` containing names and row-major integer `tiles` (index y*width+x). Zero is empty. Unknown genres/versions, invalid dimensions, malformed layers, wrong tile counts and invalid IDs are rejected. Input is capped at 16 MiB. Tests cover painting/erasing, bounds, both genres, multiple layers, memory/file round-trips, example maps and malformed data. Editor smoke testing uses SDL's dummy video driver.

## Validation and limitations

Use CTest as above; the executable prints the count of model checks. Windows CI is configured but must run on GitHub to establish Windows results. Manually on Windows: create each genre, paint and erase, zoom/pan, dock panels, save, restart, load and compare tiles; also load both examples. A dummy-driver smoke pass cannot establish these interactive results.

No undo, texture tilesets, drag interpolation, native file picker, autosave, entities, collision editing, gameplay, or packaging installer yet. Fast mouse strokes may skip cells between sampled frames. All tile IDs share four repeating display colors. Milestone 2 recommendation: undo/redo, textured tileset palette, layer tools and safe document saving before introducing gameplay.
