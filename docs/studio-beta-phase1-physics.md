# Studio Beta Phase 1: physics and visual collision correctness

## Coordinate convention

The level grid uses tile units with tile `(x,y)` occupying `[x,x+1) × [y,y+1)`. A gameplay `Player` position is the center of its axis-aligned collision box. The shared box dimensions are 0.62 tile wide by 0.78 tile high. Collision, editor playtest rendering, and tests use these same dimensions. The camera maps world points through `screen = origin + world * pixels_per_tile`; it centers the player in the playtest viewport and uses the same scale for tiles, entities, and player bounds. Contact resolution snaps the center to the exact touched tile face using the hitbox half-size.

## Findings and fixes

The reported airborne appearance was a genuine visual grounding mismatch. Physics used a 0.78-tile box but rendered a 0.72-tile rectangle. A grounded player therefore appeared approximately 0.03 tile above the floor. The player rectangle now derives its screen bounds from the collision box. Separately, collision previously stopped at the last 0.2-tile movement substep before a surface, leaving small variable gaps; collision now resolves to the precise contacted tile edge. The physics simulation itself could hold grounded state stably, but visual and collision bounds did not match exactly.

An additional control-flow defect made fall respawning unreachable: `World::solid` treated cells below every map as solid, so the player could never pass the map bottom to the existing respawn check. Platformer worlds now treat below-map cells as void and respawn after the player's hitbox passes the bottom boundary. Top-down worlds keep all map edges solid. Horizontal sides and the ceiling remain solid for both genres.

The optional playtest overlay is off by default. Toggle it from View → Playtest collision debug, the playtest toolbar checkbox, or F3 while the playtest canvas is focused. It outlines solid collision tiles and the player hitbox, and displays mode, position, velocity, grounded state, contacted/support tile, contact normal, camera origin, and pixel scale.

## Deterministic coverage

The gameplay suite now reports 72 assertions and checks flat-floor and raised-platform grounding, exact projected foot alignment, jump/land stability over repeated jumps, a traversable jump from beside a lower platform to an upper one, walking off a ledge, wall and ceiling contacts, high-speed falling without tunneling, below-map respawn, hazard/checkpoint respawn, pause/resume, repeated reset, stable corners, RPG wall collision, normalized diagonal speed, NPC interaction, non-solid NPC movement, collectibles, and the two bundled demo routes. The camera projection assertions exercise the same pure transform used by the playtest renderer; the separate SDL editor smoke test remains a startup/render check.

## Validation

- Local build succeeded.
- CTest passed 4/4 targets: level tests (70 checks), document tests (62 checks), gameplay tests (72 checks), and editor smoke.
- ASan + UBSan build with `PIXELFORGE_EDITOR=OFF` passed all 3 core test targets; leak detection and halt-on-error were enabled.
- Interactive visual inspection on a real Windows display remains necessary to assess the overlay and perceived jump feel.
