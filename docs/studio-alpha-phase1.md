# Studio Alpha Phase 1 design notes

## Foundation audit

The verified starting point is Milestone 1 commit `b9aa20a23795f93012153643cbb56d98d84c9174` on `work`, still open as PR #1. The model and serializer are independent of SDL, and the SDL window, renderer, ImGui backends, and PNG/image-independent UI lifetime already use explicit cleanup. Existing Level JSON version 1 and its bounds checks are sound for the original fields. The 42 model/serialization checks and SDL dummy-driver smoke check were green in PR #1 CI on Windows and Ubuntu.

The editor mutates `Level` directly, so there is nowhere to group stroke edits, implement history, or derive a reliable saved-document state. Layers only have names and tile arrays; the UI paints every layer and cannot protect content. The canvas uses a consistent screen-to-cell transform based on its current content origin and zoom, but edits only the current sampled cell, so fast strokes leave gaps. It renders tile IDs as repeating colors, not images. The SDL and ImGui owners are RAII-managed, but no texture asset currently exists.

## Phase 1 architecture changes

- Keep the serialized `Level` as renderer-independent data. Add optional layer visibility and lock values with defaults so existing version 1 maps remain readable.
- Add a renderer-independent `Document` that owns a Level, grouped tile commands, layer-operation snapshots, bounded undo/redo, and saved-state identity. Raster tools and UI call this API; they do not implement history themselves.
- Keep image decoding and SDL texture ownership in the rendering/tileset boundary. Decode PNG data with a pinned stb_image source and own the resulting SDL texture with RAII. Atlas dimensions and tile dimensions belong to this asset object, not Level.
- Keep view transforms in the canvas view: cursor-to-cell conversion uses the current canvas origin and zoom; stroke interpolation occurs in domain code so it can be tested without SDL.
- Keep genre-specific runtime behavior out of this phase.
