# PixelForge v1.0.0 release notes (prepared, unpublished)

PixelForge 1.0 is a portable Windows-first, native C++20 2D level editor for top-down RPGs and platformers. Both genres share a tile/layer/entity model, JSON persistence, and a built-in playtest.

## Features

- Dockable, DPI-aware SDL3 and Dear ImGui editing workspace with original PNG tileset and RPG/platformer demonstration levels.
- Pencil, eraser, rectangle, line, flood fill, eyedropper, collision painting, entity inspection, and undo/redo.
- Native Open, Save, Save As, and PNG tileset import dialogs; versioned JSON, atomic saving, safe asset handling, and unsaved-change prompts.
- Top-down movement, walls, collectibles, and NPC interactions; platformer jumping, accurate collision contact, hazards, checkpoints, and finish.
- Playtest pause, reset, and stop, preserving the editing document.

## Reliability and test coverage

- RC2 fixed editor/playtest dock identity and repeated focus stealing, plus deferred dirty-exit prompts.
- Maintainer manually accepted the RC2 Windows experience, including physical input, Play/Stop, application exit, and file dialogs, on October 14, 2026.
- The validated RC2 revision passed 16/16 CTest targets on Windows and Ubuntu, 3/3 sanitizer targets, 192 screenshot captures per platform, and 17,877 reported visual layout assertions with no failures. See [release readiness](release-readiness.md).
- The final licensed release-preparation commit and its Windows archive must receive fresh CI validation before publishing; do not reuse the prior RC2 checksum.

## Distribution

The Windows x64 release is a portable ZIP containing PixelForge.exe, RPG and platformer examples, artwork, README, project MIT LICENSE, and required third-party notices. Extract the ZIP and run PixelForge.exe.

The project is licensed under the MIT License, Copyright (c) 2026 badmrfrosty313. SDL3, Dear ImGui, nlohmann/json, stb, and the original CC0 tileset retain their own licenses.

## Known limitations

PixelForge is a focused level editor and simple playtest, not a complete game engine. There is no enemy AI, audio, or scripting; triggers have basic message behavior and player animation is minimal. Automatic screenshot-diff baselines are not approved yet, though geometry and deterministic gameplay assertions run in CI.

**Publication status:** Prepared for final merge, CI, and owner approval. No final v1.0.0 tag or GitHub Release has been published by this preparation step.
