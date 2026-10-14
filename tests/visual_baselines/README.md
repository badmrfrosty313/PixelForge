# Approved visual baselines

Place reviewed PNGs here using the exact names produced in `visual-qa/screenshots/`.
The comparison runner never writes or updates this directory. Review `index.html`,
`layout-report.json`, and `comparison-diffs/` before an intentional baseline change,
then add or replace only the PNGs approved by a human reviewer. Font and
anti-aliasing differences of up to 24 channel values are ignored; the default
changed-pixel budget is 1% per screenshot.
