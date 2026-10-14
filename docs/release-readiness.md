# PixelForge v1.0.0 release readiness

This checklist records observed evidence, not intended behavior. At the initial audit, the Beta branch pointed to `687b4cc197670ab53f690eeaa64f401cb0ec448d`; its successful hosted run [38631871411](https://github.com/badmrfrosty313/PixelForge/actions/runs/38631871411) predates the release-preparation changes. The first final-candidate run exposed a contact-sheet resolution assumption, which was corrected in `751838443cf86e6bea9c4e01b049c9f8f7d11bd4`.

## v1.0.0 license and acceptance update (October 14, 2026)

- **MIT approved:** Repository owner explicitly approved the MIT License with `Copyright (c) 2026 badmrfrosty313`. The root `LICENSE` file is committed on the Studio Beta release-preparation branch.
- **Manual Windows acceptance passed:** The owner confirmed RC2 passed the actual Windows interactive acceptance checklist, including Play/Stop, window and File-menu exit, unsaved-change handling, and native file dialogs.
- **Package updated:** CMake now installs the root MIT LICENSE and final release notes; Windows CI checks the copyright notice and packages `PixelForge-v1.0.0-windows-x64.zip` with a generated SHA-256.
- **Pending:** Passing hosted Windows, Ubuntu, sanitizer and package checks on the final licensed revision; maintainer approval for the stacked PR merges and release publication. This preparation does not merge or publish anything.
- **Visual baselines:** Optional follow-on improvement; not a blocker for v1.0.0. Native screenshots and geometry assertions already run in CI.

## RC2 interaction regression evidence

Windows acceptance testing found that Stop could leave the editor canvas hidden or unfocused, and dirty exit through the File menu did not reliably show its confirmation prompt. The canvas had two dock-window identities for edit and playtest, and playtest requested focus on every frame. The editor now uses one canvas identity and transfers focus only on transitions. The dirty-exit modal is queued until the File menu popup has closed. Runtime shutdown ordering was already scoped correctly; the observed failure was in the close interaction path.

The SDL event-driven interaction suite covers both genres, play/pause/reset/stop and restored editor controls, clean close paths, dirty Save/Discard/Cancel choices from menu and window-close requests, close while playing, and canceled save-dialog handling. It produces active-playtest and restored-editor screenshots for both genres. The local current-revision run passed all 16 CTest targets, the 192-capture Visual QA run passed 17,877 layout/gameplay assertions with zero failures, and the core ASan/UBSan run passed 3/3 targets. The sanitizer configuration excludes the SDL/ImGui editor. These checks use SDL's dummy/offscreen video drivers; physical Windows input, the actual Windows title bar, and native OS dialog behavior still need manual verification.

The initial RC2 interaction-fix revision, `274c3aa872ceb84022c8e3dcc38d9abb0b914bb0`, passed hosted run [38653528680](https://github.com/badmrfrosty313/PixelForge/actions/runs/38653528680). After adding that run's evidence here, the documented revision `1cd767455a188b6161312d49d848c5d1eb132ce2` passed [run 38653961488](https://github.com/badmrfrosty313/PixelForge/actions/runs/38653961488): Windows, Ubuntu, and core ASan/UBSan jobs all succeeded. Both platform jobs passed all 16 CTest targets and completed native Visual QA; Windows installed the portable package, verified its contents after extraction, and passed `PixelForge.exe --smoke` from a separate directory. The Windows log reports a 1,981,263-byte RC2 ZIP with SHA-256 `7b73519f2a2f57f32933066f1a3af9f934ed73066eee196f7b4e892a000319f0`. The uploaded [RC2 artifact](https://github.com/badmrfrosty313/PixelForge/actions/runs/38653961488/artifacts/11935634590) is retained for 14 days. No public release was created.

## Verified locally on the release-preparation worktree

- **Repository:** Beta branch `codex/pixelforge-studio-beta`, base HEAD `687b4cc197670ab53f690eeaa64f401cb0ec448d`; working tree began clean. All three stacked pull requests were open at the time of audit.
- **Build and CTest:** Linux build succeeded; CTest passed 4/4 targets. Direct suite counts: level/serialization 81, document 62, gameplay 78. SDL/ImGui editor smoke passed.
- **Sanitizers:** core-only ASan+UBSan build with leak detection and halt-on-error passed 3/3 CTest targets (level, document, gameplay).
- **Visual QA:** 192 real application frames at 1280×720, 1366×768, 1662×896, and 1920×1080, at 100%, 125%, 150%, and 200% UI scale. Layout report: 17,781 geometry/control assertions, 0 failures. It checks map fit and centering, canvas and sidebar dimensions, toolbar/status size, clipped controls versus enabled scrolling, and viewport bounds. Four resolutions are captured directly and resized-window scenes add four sizes.
- **Gameplay QA:** 64 deterministic demo scenarios passed, including the RPG message/interaction and crystal, platformer landing, and hazard respawn. The standalone gameplay suite passed 78 checks, including collision surface contact, camera projection, and non-square tile-coordinate round trips.
- **Visual artifacts:** gallery, contact sheet, layout/gameplay JSON, unit assertion report, baseline comparison report, and capture timing report are generated under `build/visual-qa/`. The screenshot gallery is from the actual app. No approved baselines exist; image comparison is explicitly skipped.
- **Screenshots:** four representative 1662×896 captures from the native app are checked into `docs/screenshots/`; they are not golden baselines.
- **Dependency/assets:** CMake pins SDL3, Dear ImGui, nlohmann/json, and stb sources. `THIRD_PARTY_NOTICES.md`, exact dependency license texts in the Windows package, and the original atlas's CC0 notice are present. The Windows CI smoke previously launched its extracted package from a separate directory and loaded both examples.

## Final hosted candidate verification

The exact release-preparation head `751838443cf86e6bea9c4e01b049c9f8f7d11bd4` passed both [push run 38638860039](https://github.com/badmrfrosty313/PixelForge/actions/runs/38638860039) and [PR run 38638872191](https://github.com/badmrfrosty313/PixelForge/actions/runs/38638872191). In the push run, Windows and Ubuntu Release builds each passed all 4 CTest targets; native Visual QA completed on both platforms; and the Ubuntu ASan/UBSan job passed all 3 core CTest targets. Windows also installed the portable package, checked required files after extracting the ZIP, and passed `PixelForge.exe --smoke` from a different working directory.

The Windows job produced `PixelForge-v1.0.0-windows-x64.zip` (1,970,049 bytes), SHA-256 `bd17eb533b660da8e25db59666bb6b768c37a48dca39ee8c2b09bc2b53b6f172`, plus its checksum file. The uploaded [RC artifact](https://github.com/badmrfrosty313/PixelForge/actions/runs/38638860039/artifacts/11929985732) is retained for 14 days. This artifact is a temporary CI download, not a published release.

## Remaining release gates

1. **Final licensed CI validation — pending.** Verify Windows and Ubuntu 16/16 CTest runs, sanitizer 3/3, Visual QA, and Windows package smoke on the exact new branch revision.
2. **Release package checksum — pending.** Confirm SHA-256 for the newly built licensed ZIP and verify that `LICENSE`, release notes, and third-party notices are in the archive.
3. **PR integration and publication approval — pending.** Review and merge PR #1, then #2, then #3 after rechecking dependencies and mergeability. Obtain explicit authorization before creating the final tag or GitHub Release.
4. **Approved screenshot baselines — deferred.** Baseline pixel-diff approval is optional for the initial release.

### Completed gate: Windows manual acceptance

The repository owner confirmed that PixelForge RC2 passed Windows manual acceptance on October 14, 2026. The reported Stop/Exit regressions are fixed and the release candidate was exercised with real mouse and keyboard input. Manual acceptance is not being reported as evidence of the new licensed ZIP until that artifact is available.


## Stacked pull requests and merge plan

At the repository audit, the PR stack was:

1. [PR #1](https://github.com/badmrfrosty313/PixelForge/pull/1), `work` → `main`.
2. [PR #2](https://github.com/badmrfrosty313/PixelForge/pull/2), `codex/pixelforge-studio-alpha` → `work`, depends on PR #1.
3. [PR #3](https://github.com/badmrfrosty313/PixelForge/pull/3), `codex/pixelforge-studio-beta` → `codex/pixelforge-studio-alpha`, depends on PR #2.

Proposed order after maintainer review: merge #1, retarget/review and merge #2, then retarget/review and merge #3. Recheck live ancestry and CI at each step. No PR has been merged by this campaign.

## Windows regression checklist for future changes

RC2 manual acceptance passed on October 14, 2026. If a later code change affects interactive behavior, recheck Play/Stop in both genres, File → Exit, title-bar X, dirty-document Save/Discard/Cancel, and Open/Save As/Import PNG dialogs. Pure license/documentation/packaging changes require fresh CI and ZIP verification rather than a full new gameplay acceptance pass.
