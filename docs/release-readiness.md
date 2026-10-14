# PixelForge v1.0.0 release readiness

This checklist records observed evidence, not intended behavior. At the initial audit, the Beta branch pointed to `687b4cc197670ab53f690eeaa64f401cb0ec448d`; its successful hosted run [38631871411](https://github.com/badmrfrosty313/PixelForge/actions/runs/38631871411) predates the release-preparation changes below. Verify Windows, Ubuntu, and sanitizer results on the exact candidate revision in [GitHub Actions](https://github.com/badmrfrosty313/PixelForge/actions/workflows/build.yml).

## Verified locally on the release-preparation worktree

- **Repository:** Beta branch `codex/pixelforge-studio-beta`, base HEAD `687b4cc197670ab53f690eeaa64f401cb0ec448d`; working tree began clean. All three stacked pull requests were open at the time of audit.
- **Build and CTest:** Linux build succeeded; CTest passed 4/4 targets. Direct suite counts: level/serialization 81, document 62, gameplay 78. SDL/ImGui editor smoke passed.
- **Sanitizers:** core-only ASan+UBSan build with leak detection and halt-on-error passed 3/3 CTest targets (level, document, gameplay).
- **Visual QA:** 192 real application frames at 1280×720, 1366×768, 1662×896, and 1920×1080, at 100%, 125%, 150%, and 200% UI scale. Layout report: 17,781 geometry/control assertions, 0 failures. It checks map fit and centering, canvas and sidebar dimensions, toolbar/status size, clipped controls versus enabled scrolling, and viewport bounds. Four resolutions are captured directly and resized-window scenes add four sizes.
- **Gameplay QA:** 64 deterministic demo scenarios passed, including the RPG message/interaction and crystal, platformer landing, and hazard respawn. The standalone gameplay suite passed 78 checks, including collision surface contact, camera projection, and non-square tile-coordinate round trips.
- **Visual artifacts:** gallery, contact sheet, layout/gameplay JSON, unit assertion report, baseline comparison report, and capture timing report are generated under `build/visual-qa/`. The screenshot gallery is from the actual app. No approved baselines exist; image comparison is explicitly skipped.
- **Screenshots:** four representative 1662×896 captures from the native app are checked into `docs/screenshots/`; they are not golden baselines.
- **Dependency/assets:** CMake pins SDL3, Dear ImGui, nlohmann/json, and stb sources. `THIRD_PARTY_NOTICES.md`, exact dependency license texts in the Windows package, and the original atlas's CC0 notice are present. The Windows CI smoke previously launched its extracted package from a separate directory and loaded both examples.

## Human acceptance required

- On Windows, launch the extracted release-candidate ZIP and check default layout, scale and resize behavior, both demos, movement/jump/collision, entity and collision editing, paint/erase, undo/redo, Save As, reopen, and dirty-document prompts.
- Exercise the native Open, Save As, and Import PNG dialogs. Current smoke tests cannot automate their operating-system UI.
- Review and approve screenshots as baselines before enabling pixel-diff enforcement. Current geometry checks do not prove visual taste or approve a screenshot.
- Confirm packaged asset resolution and test the versioned ZIP and checksum produced by the final hosted Windows run.

## Release blockers

1. **Project license approval — blocking.** There is no root `LICENSE`. The MIT text is a proposal in `proposed-MIT-license.md`; the copyright holder and selection require maintainer approval. Do not package a project license or publish until approved.
2. **Final hosted CI — blocking.** Release-preparation commits must pass Windows and Ubuntu build, CTest, native Visual QA, and the sanitizer job.
3. **Manual Windows acceptance — blocking for public release.** Native dialogs and interactive editing have not been verified by a person on a Windows desktop.
4. **Visual baseline approval — non-blocking for RC review, required before enforcing screenshot diffs.** No baseline is silently accepted.
5. **Publication approval — blocking.** PRs must be reviewed and merged in dependency order; final tag and GitHub Release require explicit maintainer authorization.

## Stacked pull requests and merge plan

At the repository audit, the PR stack was:

1. [PR #1](https://github.com/badmrfrosty313/PixelForge/pull/1), `work` → `main`.
2. [PR #2](https://github.com/badmrfrosty313/PixelForge/pull/2), `codex/pixelforge-studio-alpha` → `work`, depends on PR #1.
3. [PR #3](https://github.com/badmrfrosty313/PixelForge/pull/3), `codex/pixelforge-studio-beta` → `codex/pixelforge-studio-alpha`, depends on PR #2.

Proposed order after maintainer review: merge #1, retarget/review and merge #2, then retarget/review and merge #3. Recheck live ancestry and CI at each step. No PR has been merged by this campaign.

## Final manual Windows acceptance checklist

1. Download the release-candidate artifact and verify its SHA-256 file.
2. Extract the ZIP and launch `PixelForge.exe`.
3. Confirm the default workspace is readable; resize the window and check 100%, 125%, 150%, and 200% scaling.
4. Load the RPG demo, move with WASD/arrows, and interact with Mira.
5. Return to editing; paint and erase tiles, then undo and redo.
6. Use Save As, reopen the saved level, and confirm the map and entities are unchanged.
7. Load the platformer demo; run, jump, and confirm landing and exact ground contact.
8. Test the hazard respawn, checkpoint, and finish.
9. Exercise native Open, Save As, and Import PNG dialogs; close the app.

Automated coverage does not substitute for this Windows desktop pass. Do not report these manual steps as complete until a maintainer performs them.
