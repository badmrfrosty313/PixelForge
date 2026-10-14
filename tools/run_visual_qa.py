#!/usr/bin/env python3
"""Run PixelForge's native screenshot matrix and compare approved PNG baselines."""
from __future__ import annotations
import argparse
import json
import pathlib
import subprocess
import sys

try:
    from PIL import Image, ImageChops, ImageEnhance
except ImportError:
    print("Visual QA needs Pillow; install tools/requirements-visual-qa.txt", file=sys.stderr)
    raise SystemExit(2)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, type=pathlib.Path)
    parser.add_argument("--output-dir", required=True, type=pathlib.Path)
    parser.add_argument("--baseline-dir", type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parents[1] / "tests/visual_baselines")
    parser.add_argument("--changed-pixel-limit", type=float, default=0.01,
                        help="Maximum changed-pixel ratio after per-channel tolerance")
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    run = subprocess.run([str(args.binary.resolve()), "--visual-test", "--output-dir", str(args.output_dir.resolve())])
    if run.returncode:
        return run.returncode

    captures = sorted((args.output_dir / "screenshots").glob("*.png"))
    comparisons, failures = [], []
    diff_dir = args.output_dir / "comparison-diffs"
    diff_dir.mkdir(exist_ok=True)
    for capture in captures:
        baseline = args.baseline_dir / capture.name
        if not baseline.is_file():
            continue
        with Image.open(capture).convert("RGBA") as current, Image.open(baseline).convert("RGBA") as approved:
            if current.size != approved.size:
                changed = 1.0
                diff = Image.new("RGBA", current.size, (255, 0, 255, 255))
                comparison = Image.new("RGBA", (max(current.width, approved.width), current.height + approved.height), (25, 28, 34, 255))
                comparison.alpha_composite(approved, (0, 0))
                comparison.alpha_composite(current, (0, approved.height))
            else:
                delta = ImageChops.difference(current, approved)
                changed_mask = ImageChops.lighter(
                    ImageChops.lighter(delta.getchannel("R"), delta.getchannel("G")),
                    delta.getchannel("B")).point(lambda value: 255 if value > 24 else 0)
                changed = sum(1 for pixel in changed_mask.getdata() if pixel) / (current.width * current.height)
                changed_region = changed_mask.getbbox()
                diff = ImageEnhance.Contrast(delta).enhance(4)
                comparison = Image.new("RGBA", (current.width, current.height * 2), (25, 28, 34, 255))
                comparison.alpha_composite(approved, (0, 0))
                comparison.alpha_composite(current, (0, current.height))
            diff_name = capture.stem + "-diff.png"
            comparison_name = capture.stem + "-before-after.png"
            diff.save(diff_dir / diff_name)
            comparison.save(diff_dir / comparison_name)
            metric = {"screenshot": capture.name, "baseline": str(baseline.relative_to(args.baseline_dir)),
                      "changed_pixel_ratio": changed, "pixel_tolerance": 24,
                      "similarity": 1.0 - changed, "diff": diff_name,
                      "changed_region": list(changed_region) if current.size == approved.size and changed_region else None,
                      "before_after": comparison_name}
            comparisons.append(metric)
            if changed > args.changed_pixel_limit:
                failures.append({**metric, "failure": "changed pixel ratio exceeded approved threshold"})
    report = {"schema_version": 1, "baseline_directory": str(args.baseline_dir),
              "approved_baselines_compared": len(comparisons), "comparisons": comparisons,
              "failures": failures, "status": "passed" if not failures else "failed"}
    (args.output_dir / "comparison-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    summary_path = args.output_dir / "summary.md"
    with summary_path.open("a", encoding="utf-8") as summary:
        summary.write(f"\nApproved baselines compared: {len(comparisons)}. Changed-pixel failures: {len(failures)}.\n")
        if not comparisons:
            summary.write("No approved baseline PNGs are present; comparison was skipped. Baselines are never accepted automatically.\n")
    gallery_path = args.output_dir / "index.html"
    with gallery_path.open("a", encoding="utf-8") as gallery:
        if comparisons:
            gallery.write("<h2>Approved baseline comparisons</h2><main>")
            for metric in comparisons:
                gallery.write(f'<figure><img loading="lazy" src="comparison-diffs/{metric["before_after"]}"><figcaption>{metric["screenshot"]}: similarity {metric["similarity"]:.4%}; changed pixels {metric["changed_pixel_ratio"]:.4%} — <a href="comparison-diffs/{metric["diff"]}">difference image</a></figcaption></figure>')
            gallery.write("</main>")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
