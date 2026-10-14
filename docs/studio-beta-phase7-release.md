# Studio Beta Phase 7: Portable Windows package

The Windows job installs the release executable, both example JSON files, the original CC0 atlas, README, third-party notices, and license texts for SDL3, Dear ImGui, nlohmann/json, and stb_image. It validates the expected files, copies the package into the runner's temporary directory, launches `PixelForge.exe --smoke` from a different current directory, and archives the release folder as `PixelForge-Windows.zip`.

The packaged smoke loads both example levels through `SDL_GetBasePath()`, initializes each runtime controller, draws their runtime graphics, and repeats start/stop cycles. This establishes a functional packaged headless launch on the Windows runner and verifies the executable does not rely on the checkout's working directory. It does not replace interactive visual acceptance or screenshots from a real Windows desktop.

The atlas is CC0 and dependency notices are packaged. No project-wide license is present in the repository; this campaign does not select one. A maintainer should resolve the project license before describing a public release as fully licensed.
