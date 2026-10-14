# Approved visual baselines

The approved-baseline directory is intentionally empty. Representative review
candidates are included in [`docs/screenshots`](../../docs/screenshots/); they
are release screenshots, not approved golden images. A maintainer should review
the native captures and explicitly copy only approved images here using the
exact names produced in `build/visual-qa/screenshots/`.

The comparison runner never writes or updates this directory. Review
`index.html`, `contact-sheet.png`, `layout-report.json`, and
`comparison-diffs/` before an intentional baseline change. Font and
anti-aliasing differences up to 24 channel values are ignored; the default
changed-pixel budget is 1% per screenshot. An empty baseline set is reported as
skipped, not as visual approval.
