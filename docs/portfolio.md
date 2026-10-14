# PixelForge — portfolio summary

PixelForge is a native C++20 desktop editor for building and playtesting 2D top-down RPG and platformer levels. Both genres share one SDL3 and Dear ImGui workspace, one renderer-independent level/document model, and versioned JSON persistence.

## Engineering work

- Built a dockable editor with tile painting, erase/line/rectangle/flood-fill/eyedropper tools, layers, collision grids, entities, and bounded undo/redo.
- Kept gameplay rules separate from editor and level data; implemented fixed-step RPG and platformer sessions using shared collision code.
- Added strict JSON schema validation and migrations, safe relative-asset resolution, and atomic saves.
- Fixed Windows path validation by rejecting any rooted path independently of host-specific `filesystem::path::is_absolute()` behavior.
- Fixed platformer rendered-player and collision-box alignment, exact tile contact, and below-map respawn behavior; added collision diagnostics.
- Added a native visual QA lab that captures the real application and validates layout and deterministic gameplay on Windows and Linux.
- Packaged the application and example levels for Windows with third-party notices and a working-directory-independent resource lookup.

## Testing evidence

Current local run: CTest passed 4/4 targets, comprising 81 level/serialization checks, 62 document checks, 78 gameplay checks, and the SDL/ImGui editor smoke. The core suites also passed ASan and UBSan with leak detection enabled (3/3 CTest targets).

The Visual QA matrix captured 192 real SDL/ImGui frames at four resolutions and four UI scales. Its layout report recorded 17,781 assertions, and deterministic scenarios passed 64 gameplay checks. The screenshot comparison step reports skipped until human-approved baselines are added; no baselines are accepted automatically. Consult [release readiness](release-readiness.md) and the linked hosted workflow for evidence from the exact release-candidate source revision; earlier Beta runs do not verify later commits.

## Current limitations

PixelForge is an editor and minimal playtest environment, not a full game engine. It has no audio, scripting, enemy AI, or full animation system. Native Windows dialogs and interactive editing still require a maintainer's Windows acceptance pass. The project-wide license is pending approval; the prepared MIT proposal is not an active license.
