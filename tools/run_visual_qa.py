#!/usr/bin/env python3
"""Run PixelForge's native screenshot matrix and compare approved PNG baselines."""
from __future__ import annotations
import argparse
import json
import pathlib
import subprocess
import sys
import time

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
    capture_started = time.perf_counter()
    run = subprocess.run([str(args.binary.resolve()), "--visual-test", "--output-dir", str(args.output_dir.resolve())])
    capture_seconds = time.perf_counter() - capture_started
    if run.returncode:
        return run.returncode

    captures = sorted((args.output_dir / "screenshots").glob("*.png"))
    layout = json.loads((args.output_dir / "layout-report.json").read_text(encoding="utf-8"))
    # A contact sheet gives reviewers a fast overview without replacing the
    # full-size native captures or requiring them to approve baselines.
    candidates = [scene for scene in layout["scenes"]
                  if scene["ui_scale_percent"] == 100]
    selected_by_scene: dict[str, dict] = {}
    for scene in candidates:
        selected_by_scene.setdefault(scene["name"], scene)
        if scene["resolution"] == [1662, 896]:
            selected_by_scene[scene["name"]] = scene
    selected = list(selected_by_scene.values())
    if len(selected) != 12:
        print(f"Expected captures for 12 scenes, found {len(selected)}",
              file=sys.stderr)
        return 2
    columns, thumb_width, gap, caption_height = 3, 480, 16, 34
    thumb_height = round(thumb_width * 896 / 1662)
    rows = (len(selected) + columns - 1) // columns
    sheet = Image.new("RGB", (gap + columns * (thumb_width + gap),
                              gap + rows * (thumb_height + caption_height + gap)),
                      (32, 37, 44))
    from PIL import ImageDraw
    draw = ImageDraw.Draw(sheet)
    for index, scene in enumerate(selected):
        capture = args.output_dir / "screenshots" / scene["screenshot"]
        with Image.open(capture).convert("RGB") as image:
            image.thumbnail((thumb_width, thumb_height), Image.Resampling.LANCZOS)
            x = gap + (index % columns) * (thumb_width + gap)
            y = gap + (index // columns) * (thumb_height + caption_height + gap)
            sheet.paste(image, (x, y))
            draw.text((x, y + thumb_height + 8), scene["name"], fill=(235, 240, 246))
    sheet.save(args.output_dir / "contact-sheet.png", optimize=True)
    performance = {"native_capture_seconds": round(capture_seconds, 3),
                   "screenshot_count": len(captures),
                   "layout_assertion_count": layout.get("layout_assertion_count", 0)}
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
    performance["baseline_comparison_count"] = len(comparisons)
    (args.output_dir / "qa-performance.json").write_text(
        json.dumps(performance, indent=2) + "\n", encoding="utf-8")
    (args.output_dir / "comparison-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    summary_path = args.output_dir / "summary.md"
    with summary_path.open("a", encoding="utf-8") as summary:
        summary.write(f"\nNative capture time: {capture_seconds:.3f} seconds. Contact sheet: `contact-sheet.png`.\n")
        summary.write(f"\nApproved baselines compared: {len(comparisons)}. Changed-pixel failures: {len(failures)}.\n")
        if not comparisons:
            summary.write("No approved baseline PNGs are present; comparison was skipped. Baselines are never accepted automatically.\n")
    gallery_path = args.output_dir / "index.html"
    gallery = gallery_path.read_text(encoding="utf-8")
    gallery = gallery.replace(
        "<h1>PixelForge Visual QA</h1>",
        "<h1>PixelForge Visual QA</h1><h2>Overview</h2><p><a href='contact-sheet.png'>Open the 12-scene contact sheet</a></p><p>Full-size images below are native PixelForge captures.</p>",
        1)
    gallery_path.write_text(gallery, encoding="utf-8")
    with gallery_path.open("a", encoding="utf-8") as gallery:
        if comparisons:
            gallery.write("<h2>Approved baseline comparisons</h2><main>")
            for metric in comparisons:
                gallery.write(f'<figure><img loading="lazy" src="comparison-diffs/{metric["before_after"]}"><figcaption>{metric["screenshot"]}: similarity {metric["similarity"]:.4%}; changed pixels {metric["changed_pixel_ratio"]:.4%} — <a href="comparison-diffs/{metric["diff"]}">difference image</a></figcaption></figure>')
            gallery.write("</main>")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
