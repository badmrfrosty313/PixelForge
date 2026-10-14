# Studio Alpha Phase 5: build quality and Windows package

## Build and tests

The C++20 model, document, and gameplay code are separate CMake targets with authored-target warnings enabled. Dependencies are pinned by immutable Git commit references. GitHub Actions builds and runs CTest on Windows and Ubuntu. Headless Linux sanitizer verification is available with AddressSanitizer and UndefinedBehaviorSanitizer.

The gameplay suite covers sample RPG navigation around its collision wall, NPC interaction, collectible pickup, and a platformer route that reaches both raised platforms, activates the checkpoint, and reaches the finish trigger. Save tests cover successful replacement and deterministic failed replacement/temporary cleanup.

## Windows package

CMake install rules package the executable, both sample JSON files, the original CC0 atlas, README, third-party notices, and full pinned dependency license texts. The MSVC runtime is statically linked so the ZIP does not depend on an installed Visual C++ runtime. GitHub Actions validates each expected file and uploads the ZIP as the `PixelForge-Windows` artifact.

The package is checked for layout and nonempty archive creation in CI. On Linux, the install prefix was validated by running the editor's SDL dummy-driver smoke test from the installed directory and loading both sample levels from the packaged examples folder.

## Portfolio readiness and remaining gaps

The README contains the project overview, module map, controls, build commands, tests, and limitations. No genuine desktop screenshots were captured because this environment has no interactive display. File dialogs, polished input layouts across resolutions, and native Windows manual interaction remain to verify. The artifact is a CI download, not a GitHub Release.
