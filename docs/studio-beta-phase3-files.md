# Studio Beta Phase 3: File operations

The editor now uses SDL3's asynchronous native file dialogs for opening and saving JSON levels and importing PNG tilesets. Dialog results cross back to the editor thread through a mutex-protected mailbox; canceling a picker does not alter the document. Dirty documents offer Save, Discard, and Cancel before Open, New, or Exit.

Bundled resources resolve from the executable directory, with a source-tree asset fallback during development. Tileset references are validated as safe relative paths. Loads look beside the document first and then at the explicit bundled resource root. Saving copies a selected tileset next to the JSON level, chooses a suffix when a different file already occupies the intended name, and writes the document through the existing atomic save path. PNG decoding uses wide-character file access on Windows.

Automated coverage exercises document-local and bundled resolution, asset packaging and conflict handling, parent traversal, POSIX-rooted, drive-rooted, and UNC references. CTest exercises the SDL/ImGui smoke startup on the dummy Linux video driver. Windows file picker behavior, Unicode path interaction, and the full edit/save/reopen workflow still need manual Windows verification.
