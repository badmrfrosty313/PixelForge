# Studio Beta Phase 7: Portable Windows package

The Windows job installs the executable, both example JSON files, the original CC0 atlas, README, selected application screenshots, release documentation, third-party notices, and license texts for SDL3, Dear ImGui, nlohmann/json, and stb_image. It validates the expected files, copies the package into the runner's temporary directory, launches `PixelForge.exe --smoke` from a different current directory, and prepares `PixelForge-v1.0.0-windows-x64.zip` with a SHA-256 checksum. CI publishes that ZIP as the temporary `PixelForge-v1.0.0-rc1-windows-x64` artifact; it is not a public GitHub Release.

The packaged smoke loads both example levels through `SDL_GetBasePath()`, initializes each runtime controller, draws their runtime graphics, and repeats start/stop cycles. This establishes a functional packaged headless launch on the Windows runner and verifies the executable does not rely on the checkout's working directory. It does not replace interactive visual acceptance or screenshots from a real Windows desktop.

The atlas is CC0 and dependency notices are packaged. No project-wide license is present in the repository. `docs/proposed-MIT-license.md` is a proposal only; a maintainer must approve the license and copyright holder before the root license or a public release is created.
