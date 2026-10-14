# PixelForge — portfolio summary

PixelForge is a native C++20 desktop editor for building and playtesting 2D top-down RPG and platformer levels. A shared SDL3 and Dear ImGui workspace edits both genres through one validated level model, layered tile canvas, collision and entity tools, and versioned JSON persistence.

The project demonstrates renderer-independent gameplay rules, fixed-step simulation, deterministic collision tests, bounded undo/redo, atomic document saves, document-relative asset packaging, and cross-platform Windows/Linux CI. A recent Windows-specific path-validation issue was fixed by rejecting every rooted path rather than relying on host-specific `filesystem::path::is_absolute()` behavior. The playtest also includes collision overlays that expose the hitbox, supporting tile, contact normals, and camera transform.

## Resume-ready description

Built PixelForge, a C++20/SDL3 2D level editor with a shared RPG/platformer workflow, Dear ImGui tooling, JSON serialization, deterministic collision and playtesting, native file dialogs, and Windows/Ubuntu CI. Added regression tests for physics, serialization, unsafe paths, asset resolution, and document history; packaged a portable Windows ZIP with runtime assets and dependency notices.

## Evidence and current limits

The repository contains deterministic model, document, and gameplay suites, a headless SDL/ImGui smoke that loads both demos and exercises repeated playtest sessions, and Windows packaging validation. This environment cannot capture a genuine interactive Windows screenshot. The engine has no audio, enemy AI, scripting, or full animation system. The CC0 atlas and third-party dependency notices are present, but the repository does not declare a project-wide license; maintainers should choose one before public release.
