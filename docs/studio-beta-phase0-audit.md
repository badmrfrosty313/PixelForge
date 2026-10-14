# Studio Beta Phase 0: repository and regression audit

## Starting point

- Audited commit: `7bc5612afb6471b0b296c76ce8ff97d956ec2182` (`Package PixelForge Windows release artifact`).
- Starting branch: `codex/pixelforge-studio-alpha`; clean working tree.
- Beta work continues on `codex/pixelforge-studio-beta`, created from that commit.
- PR #1 is open from `work` to `main`; its latest reported Windows and Ubuntu checks pass.
- PR #2 is open from `codex/pixelforge-studio-alpha` to `work`; the latest push and PR runs pass on Windows and Ubuntu.
- No pull request was merged as part of this audit.

## Architecture

`application` owns SDL initialization, the native window/renderer, ImGui backends, event processing, and frame lifecycle using RAII. `editor` owns the active `Document`, tileset resource, canvas/input state, panels, and playtest session. `document` applies renderer-independent editing commands and keeps bounded undo/redo deltas. `level` defines validated, renderer-independent tile layers, collision cells, and typed entities. `serialization` migrates v1 JSON, validates v2 JSON, and atomically replaces saved files. `tileset` decodes PNG data and owns an SDL texture; `rendering` draws atlas cells. `gameplay` copies level data into a runtime world and selects separate top-down or platformer controllers behind a shared fixed-step session. CMake FetchContent pins SDL3, Dear ImGui, nlohmann/json, and stb_image by commit. CI builds/tests on Ubuntu and Windows and packages a portable Windows ZIP.

## Baseline validation and examples

The existing build was current and required no recompilation. CTest passed 4/4 targets: `level_tests`, `document_tests`, `gameplay_tests`, and the SDL/ImGui dummy-video `editor_smoke`. Direct assertions reported 70 model/serialization checks, 62 document checks, and 24 gameplay checks. Both example JSON files parse and round-trip. The 20x15 RPG demo has 71 collision cells, spawn at (2.5, 2.5), NPC at (5.5, 5.5), and collectible at (14.5, 5.5). The 20x15 platformer has 31 collision cells, spawn at (1.5, 12.5), hazard at (10.5, 12.5), checkpoint at (14.5, 7.5), and finish trigger at (18.5, 12.5). Existing gameplay tests traverse the intended NPC/collectible and platform/checkpoint/finish paths.

## Grounding trace and finding

World coordinates use tile units. A tile at row `r` occupies vertical interval `[r, r+1)`. The platformer player position `(x,y)` is the center of its collision box, which is 0.62 by 0.78 world units. Collision tests use its bottom at `y + 0.39`. The camera transform in `playtest_canvas` maps a world point `(x,y)` to `(origin.x + x*cell, origin.y + y*cell)`, with the origin centered on the player. Tiles use the same cell scale. The player was drawn as a rectangle centered on that transformed point, but its vertical half-size was 0.36 instead of the collision half-height 0.39. This left the visible rectangle about 0.03 tile (roughly 0.72 screen pixels at the usual 24-pixel cell) above the support. This is a real render/hitbox alignment defect, not evidence that the simulation was floating. Phase 1 derives rendered bounds from the collision dimensions and snaps collision resolution to the exact tile face. The audit also found that treating all out-of-bounds cells as solid made the platformer's fall-respawn branch unreachable; Phase 1 makes below-map space a platformer void while retaining solid top-down bounds.

## Significant technical risks

- `editor.cpp` combines document workflows, many panels, canvas input, playtest rendering, and tool state in one large translation unit; UI changes need regression checks to avoid coupling mistakes.
- The playtest renderer uses generic rectangles/circles and has no collision/contact diagnostics, making visual investigation difficult.
- Existing gameplay tests cover representative sample routes, but not wall/ceiling collision, ledge transitions, high-speed landing, corner contacts, or repeated reset stability.
- File Open and Save As still depend on a path text field. Resource lookup has working-directory, executable-directory, and source-tree fallbacks, so runtime asset behavior is not fully anchored to the installed package.
- Dear ImGui docking is enabled, but there is no custom dock layout, DPI scaling control, reset-layout command, welcome page, or native file picker.
- The dummy-video smoke test proves initialization and rendering commands, not interactive Windows behavior or visual quality.
- ASan/UBSan have been run on core suites during Alpha, but CI currently has no sanitizer job or performance checks.
- The release ZIP checks required files and is uploaded as an Actions artifact; no genuine application screenshots or GitHub Release exist.

## Phase 0 result

The latest Alpha commit is the correct baseline and all current automated checks passed. The reported grounding symptom traced to a 0.03-world-unit difference between the collision-box half-height and rendered rectangle half-height. Phase 1 corrected the geometry and exact contact resolution, fixed unreachable below-map respawning, and expanded deterministic physics tests before interface work.
