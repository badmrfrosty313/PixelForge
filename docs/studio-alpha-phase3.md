# Studio Alpha Phase 3: built-in playtest

## Runtime boundaries

`gameplay::make_runtime_level` validates and converts a `Level` into copied visual data and mutable runtime world state. The editor continues to own its `Document`; the session never writes back into it. `gameplay::Session` resolves the player spawn and advances through a fixed 1/60-second accumulator. Large frame delays are bounded to avoid a runaway catch-up loop. Reset restores the original entities, collision and spawn state; pause freezes updates.

The shared `World` exposes deterministic solid-cell queries with out-of-bounds treated as solid. Both controllers use bounded substeps for collision movement. `TopDownController` normalizes four-direction input and supports nearby NPC interaction plus collectible pickup. `PlatformerController` applies gravity, horizontal movement and jump impulse, resolves solid tiles, tracks checkpoints, and respawns at the last checkpoint after hazards or falling below the map.

## Editor playtest

Play constructs a new runtime session from the current document without saving or changing it. While active, the level canvas displays runtime entities and the player, follows the player, and routes keyboard input separately from editor tools. The toolbar exposes Pause/Resume, Reset, and Stop. Stop destroys runtime state and returns to the still-intact editor document. Repeated sessions construct fresh state.

Controls: click the play canvas to focus it; arrows or WASD move; Space or Up jumps in the platformer; E interacts with an NPC. Pause/reset/stop use the toolbar.

## Validation and limits

The headless gameplay suite exercises spawn resolution, top-down solid collision and NPC messages, collectible behavior, pause/reset isolation, gravity and landing, jump impulse, checkpoint progression, hazard respawn, world bounds, and preservation of source level data. It also follows the RPG example around its wall to interact with the NPC and collect the item, and plays the platformer sample across both raised platforms to its checkpoint and finish trigger. The SDL dummy-driver smoke test still checks application startup/rendering. Desktop keyboard focus, perceived physics feel, and Windows interaction need manual verification.

This is a small prototype runtime, not a complete game framework. Enemies are static, triggers display a message only, and there is no animation, audio, scripting, camera dead zone, or authored physics configuration. Phase 4 adds atomic document saves, Save As, and close-time unsaved-change handling. Windows packaging remains future work.
