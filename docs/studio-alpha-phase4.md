# Studio Alpha Phase 4: document safety

## Saving and replacement

`save` serializes and validates first, writes a same-directory temporary sibling, flushes and closes it, and only then replaces the destination. Windows uses `MoveFileExW` with replace-existing/write-through flags; POSIX uses `rename`. Failed writes and replacement clean up the temporary file and preserve the prior destination whenever replacement has not occurred.

The UI supports Save and Save As through the Level JSON path field. Ctrl+S saves the current document path; Ctrl+Shift+S saves to the field path. New and Open warn before discarding edits. Closing via the window or File > Exit prompts with Save, Discard, and Cancel. A failed save leaves the document dirty and keeps the close prompt open. The toolbar and status bar show the modified state.

## Validation and limitations

Serialization tests cover initial save, replacement round-trip, deterministic replacement failure against an existing directory, preservation of marker content, and cleanup of temporary siblings. Existing v1/v2 serialization and all runtime/document suites remain active. The dummy SDL smoke test validates startup only; actual close-prompt interaction and keyboard focus need manual desktop verification.

The path field acts as the Save As destination. Native file dialogs are not included. Portable Windows packaging and genuine screenshots remain future work.
