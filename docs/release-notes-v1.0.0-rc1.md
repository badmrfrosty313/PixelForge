# PixelForge v1.0.0 release candidate 1

PixelForge is a native C++20 2D level editor and lightweight playtest tool for top-down RPGs and platformers. Both genres share the same tile canvas, layer model, entity and collision data, and JSON serialization.

## Included

- Responsive dockable editor, four UI scale presets, initial map fit and centering, and mouse zoom/pan.
- Pencil, eraser, line, rectangle, flood-fill, and eyedropper tools; layers; collision editing; entity placement and properties; bounded undo/redo.
- Native Windows file dialogs, strict versioned JSON loading, safe relative asset paths, and atomic saves.
- Fixed-step RPG and platformer playtests with movement, collision, NPC interaction, collectibles, jumping, checkpoints, hazards, and respawn.
- RPG Village and Platformer Challenge examples.
- Windows portable package, bundled asset and dependency notices, and native cross-platform CI.
- 192-scene native screenshot and deterministic gameplay QA with a contact sheet and detailed reports.

## Validation

Local CTest, core ASan/UBSan, and SDL/ImGui Visual QA evidence are recorded in `release-readiness.md`. Check that Windows, Ubuntu, and sanitizer jobs completed on the exact source revision under review; earlier Beta runs do not verify later commits.

## Known limitations and pending gates

There is no audio, scripting, or enemy AI. Native Windows dialogs and mouse-driven workflows still need a manual Windows acceptance pass. No visual baselines have been approved. The project-wide license is awaiting maintainer approval. This is a release candidate for review: no v1.0.0 tag or GitHub Release has been published.
