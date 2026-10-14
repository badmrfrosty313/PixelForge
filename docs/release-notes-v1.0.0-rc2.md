# PixelForge v1.0.0 release candidate 2

RC2 fixes two critical editor interaction failures found during Windows acceptance testing: stopping playtest now restores the shared editor canvas reliably, and dirty-document exit prompts opened from the File menu now appear after the menu closes.

## Included

- All RC1 features described in `release-notes-v1.0.0-rc1.md`.
- A stable shared dock window for editing and playtesting, with keyboard focus transferred only at state transitions.
- Deferred dirty-exit confirmation after menu dismissal, with working Save, Discard, and Cancel paths for menu and window-close requests.
- SDL-event-driven regression coverage for both genres, playtest transitions, close requests, dirty prompts, and canceled save dialogs.
- An RC2 portable Windows ZIP with an adjacent SHA-256 checksum, generated and smoke-tested by Windows CI.

## Validation

Consult `release-readiness.md` and the CI run on the exact revision under review for actual CTest, sanitizer, visual QA, package, and checksum results. Native Windows user input and operating-system file dialogs still require manual desktop acceptance.

## Pending gates

The project license still requires maintainer approval, visual screenshot baselines have not been approved, and public release requires the existing pull request stack to be reviewed and merged by maintainers. No tag or GitHub Release is created by this candidate workflow.
