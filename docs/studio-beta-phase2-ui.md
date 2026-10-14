# Studio Beta Phase 2: responsive desktop workspace

## Workspace

The editor creates a dockspace from the current main viewport work area. Its default layout puts the tileset palette on the left, the level/playtest canvas in the center, Layers and Entities in vertically split right panes, a compact command toolbar across the top, and a two-line document/status strip at the bottom. The split proportions adjust for smaller client widths and are selected from the display scale when the default layout is first created. No panel uses a fixed screen position or size. Dear ImGui persists docking changes in its normal `imgui.ini`; View → Reset workspace layout rebuilds the default layout.

The SDL window is resizable, starts at 1440×900, requests a high-density backbuffer, and enforces an 800×560 logical minimum. The center canvas uses all available dockspace content and recalculates its view on resize. The canvas and playtest share dock tabs, and starting/stopping a playtest focuses the appropriate view.

## DPI and readability

The application queries `SDL_GetWindowDisplayScale` at startup and reacts to `SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED`. View → UI scale offers 100%, 125%, 150%, and 200%. Applying a preset restores the unscaled base style, scales spacing and widget metrics, and rebuilds the embedded Dear ImGui default font atlas at the matching pixel size. No external font file is required. The SDL/ImGui logical mouse coordinates are never multiplied by the style scale, so canvas-to-tile calculations remain unchanged.

The scale-aware smoke run changes through 1440×900, 1280×720, 1024×768, and 800×560 while applying all four scale presets, and checks that each frame submits visible geometry. This exercises initialization, resizing, font-atlas rebuilds, and dock layout without claiming interactive Windows visual verification.

## Visual hierarchy and controls

The UI uses a restrained dark palette with higher-contrast text, a cool blue interaction accent, softened borders, and consistent padding/rounding. The toolbar keeps Play/Pause/Reset/Stop, Undo/Redo, a selected-tool combo, Grid/Collision, Fit, and Reset Zoom within a short command row; file operations remain in the File menu. Undo and Redo are disabled when unavailable. Document name/dirty marker, genre, dimensions, active layer, cursor tile, tool, zoom, and status messages appear in the bottom strip. Docked side panels inherit workspace resizing rather than relying on fixed widths.

## Validation and limitations

- Local build succeeded.
- CTest passed 4/4 targets: 70 model/serialization checks, 62 document checks, 72 gameplay checks, and the resized/scaled SDL editor smoke test.
- Phase 1's ASan+UBSan suites remain green (3/3 core tests); the Phase 2 changes affect only UI/application code.
- Windows CI will validate the SDL3/Dear ImGui APIs and Windows compilation for this phase.
- An interactive Windows desktop is unavailable here, so actual docking appearance, mouse behavior at native DPI settings, readability at 200%, and window resizing still need manual verification.
