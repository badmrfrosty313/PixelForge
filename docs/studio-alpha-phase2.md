# Studio Alpha Phase 2: structured world building

## Data and ownership

`Level` owns renderer-independent metadata, ordered visual layers, one explicit row-major collision grid, and typed entities. Entity IDs are stable nonzero 64-bit values; positions are floating-point cell coordinates. Optional properties are bounded JSON values. The editor owns only selection and interaction state. Genre-specific behavior remains outside the data model.

The v2 format adds map tile dimensions, a safe relative tileset path and atlas cell size, collision booleans, and entities. Version 1 continues to load with 24x24 map cells, the bundled atlas reference, 16x16 atlas slicing, no collision/entities, and the old layer defaults. Unknown fields are rejected to avoid lossy migration. The serializer bounds files and checks dimensions, IDs, entity coordinates and property sizes; paths that are absolute or traverse directories are rejected.

## Editor interactions

The Collision tool toggles the first cell's solid state and paints an interpolated grouped stroke. The overlay shows solid cells independently of visual tiles. The Entity tool places the selected type at a cell center, selects nearby markers, and moves them by drag. The Entities panel edits type-specific identity, grid position, name/message fields, and deletes selected items. Collision strokes, entity creation/update/deletion, and asset settings are undoable document operations.

Both sample levels use this same editor and serialization path. The RPG sample has boundary/obstacle collision, a player spawn, an NPC, and a collectible. The platformer sample has ground/platform collision, a spawn, checkpoint, hazard, and trigger.

## Validation and limits

The model and document suites run without window initialization. They check v1 migration, v2 persistence and validation, unsafe paths, invalid collision data, entity IDs/types/coordinates/properties, collision stroke grouping, entity undo, and asset-setting undo. The editor smoke test initializes SDL and ImGui with the dummy video driver and loads the real bundled atlas. Manual Windows interaction is still needed to assess entity dragging and inspector usability at desktop resolutions.

No runtime controllers use this data yet. Phase 3 will add shared world conversion/collision queries and isolated top-down/platformer controllers.
