#!/usr/bin/env python3
"""
Rendering tests for Duel 6 Reloaded.

Basic scenarios (always run): the windowed menu, a windowed game and the same game toggled
to fullscreen. Each starts the game with automation flags, takes a screenshot after a few
seconds and checks that
  * the screenshot has the window's size (taken from the game's own "[video] screenshot"
    log line, so fullscreen works too),
  * the frame reaches the right and bottom window edges (a stale backbuffer leaves a black
    band there),
  * the game exits cleanly and the Vulkan validation layers report nothing.

Extended scenarios (with --reference BINARY): every scenario, plus a game screenshot at a
later water-wave phase, is also run with the reference binary (normally the OpenGL ES 3
build) and the two screenshots are compared pixel by pixel. Runs are seeded and timed in
game ticks, so both renderers draw the same game state. The menu's animated banner strip
is left out of the comparison.

Usage:
    tests/render_test.py BINARY [--reference BINARY] [--out DIR] [--only NAME...]

Screenshots, difference masks and the game logs go to .tmp/render_test in the repository
(previous results are removed on every run) unless --out is given.

The binaries must be built with the CMake option D6R_AUTOMATION=ON, which compiles in the
automation flags (source/automation/Automation.h). They run from their own directory, so
build directories need the resource symlinks (data, levels, profiles, sound, textures,
shaders). Requires Pillow.
"""

import argparse
import glob
import os
import re
import subprocess
import sys

try:
    from PIL import Image, ImageChops
except ImportError:
    sys.exit("Pillow is required: python3 -m pip install pillow")

SEED = 1
LEVEL = "duel_01"
WINDOW = (1280, 720)
TIMEOUT = 40
DEFAULT_OUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), ".tmp", "render_test")

# Edge strip checked for content, and how dark a pixel must be to count as "black"
EDGE_STRIP = 8
BLACK_THRESHOLD = 16
MAX_BLACK_RATIO = 0.5

# Reference comparison: a pixel differs when a channel differs by more than PIXEL_TOLERANCE.
# All scenarios are identical today; TOLERANCE leaves room for driver differences while still
# catching e.g. static water (~180 px) or a culled face (~40 px).
PIXEL_TOLERANCE = 64
TOLERANCE = 50
# The menu's animated banner: the GUI is centred vertically in a 900 px tall layout
MENU_LAYOUT_HEIGHT = 900
MENU_BANNER_HEIGHT = 120


def scenario(name, game=False, fullscreen=False, screenshot_after=None, extended=False):
    if screenshot_after is None:
        # Let the game's round-start overlay fade out, and fullscreen settle (~1s after the toggle)
        screenshot_after = 4.0 if game else 2.5
    return dict(name=name, game=game, fullscreen=fullscreen, screenshot_after=screenshot_after, extended=extended)


SCENARIOS = [
    scenario("menu"),
    scenario("game", game=True),
    scenario("game-fullscreen", game=True, fullscreen=True),
    # Another water-wave phase than "game"
    scenario("game-later", game=True, screenshot_after=5.5, extended=True),
]
FULLSCREEN_AFTER = 1.0

LOG_PATTERN = re.compile(r"^\[(video|vulkan|automation)\].*$", re.MULTILINE)
VK_ERROR_PATTERN = re.compile(r"^\[VK ERROR\].*$", re.MULTILINE)
SCREENSHOT_SIZE_PATTERN = re.compile(r"^\[video\] screenshot.*?window (\d+)x(\d+)", re.MULTILINE)
SCREENSHOT_TICK_PATTERN = re.compile(r"^\[video\] screenshot at tick (\d+)", re.MULTILINE)


def black_ratio(image, box):
    histogram = image.crop(box).convert("L").histogram()
    return sum(histogram[:BLACK_THRESHOLD]) / sum(histogram)


def menu_banner_bottom(image):
    return max(0, (image.height - MENU_LAYOUT_HEIGHT) // 2) + MENU_BANNER_HEIGHT


def count_different_pixels(path_a, path_b, skip_banner, mask_path):
    a = Image.open(path_a).convert("RGB")
    b = Image.open(path_b).convert("RGB")
    if a.size != b.size:
        return None, f"sizes differ: {a.size} vs {b.size}"
    skip_top = menu_banner_bottom(a) if skip_banner else 0
    box = (0, skip_top, a.width, a.height)
    diff = ImageChops.difference(a.crop(box), b.crop(box))
    mask = diff.convert("L").point(lambda v: 255 if v > PIXEL_TOLERANCE else 0)
    different = mask.histogram()[255]
    if different:
        mask.save(mask_path)
    return different, None


def run_game(binary, sc, screenshot):
    """Run one scenario and return (failures, output)."""
    after = sc["screenshot_after"]
    cmd = [binary, "-log-stdout", "-seed", str(SEED), "-window", f"{WINDOW[0]}x{WINDOW[1]}",
           "-screenshot", screenshot, "-screenshot-after", str(after), "-quit-after", str(after + 0.3)]
    if sc["game"]:
        cmd += ["-play", "-level", LEVEL]
    if sc["fullscreen"]:
        cmd += ["-fullscreen-after", str(FULLSCREEN_AFTER)]

    try:
        proc = subprocess.run(cmd, cwd=os.path.dirname(binary), capture_output=True, text=True,
                              errors="replace", timeout=TIMEOUT)
    except subprocess.TimeoutExpired as e:
        output = e.stdout if isinstance(e.stdout, str) else (e.stdout or b"").decode(errors="replace")
        failure = f"game did not quit within {TIMEOUT}s"
        if "[automation]" not in output:
            failure += " - no automation output, was it built with -DD6R_AUTOMATION=ON?"
        return [failure], output
    output = proc.stdout + proc.stderr

    failures = []
    if proc.returncode != 0:
        failures.append(f"game exited with code {proc.returncode}")

    vk_errors = VK_ERROR_PATTERN.findall(output)
    if vk_errors:
        failures.append(f"{len(vk_errors)} Vulkan validation error(s), first: {vk_errors[0]}")

    if not os.path.exists(screenshot):
        failures.append("no screenshot was written")
        return failures, output

    image = Image.open(screenshot)
    width, height = image.size
    logged = SCREENSHOT_SIZE_PATTERN.search(output)
    if not logged:
        failures.append("game did not log the window size at screenshot time")
    elif (width, height) != (int(logged.group(1)), int(logged.group(2))):
        failures.append(f"screenshot is {width}x{height}, window is {logged.group(1)}x{logged.group(2)}")

    right = black_ratio(image, (width - EDGE_STRIP, 0, width, height))
    bottom = black_ratio(image, (0, height - EDGE_STRIP, width, height))
    if right > MAX_BLACK_RATIO:
        failures.append(f"right edge is {right:.0%} black - frame does not reach the window edge")
    if bottom > MAX_BLACK_RATIO:
        failures.append(f"bottom edge is {bottom:.0%} black - frame does not reach the window edge")

    return failures, output


def compare_with_reference(reference, sc, screenshot, output, out_dir):
    name = sc["name"]
    ref_screenshot = os.path.join(out_dir, f"{name}_reference.png")
    ref_failures, ref_output = run_game(reference, sc, ref_screenshot)
    write_log(out_dir, f"{name}_reference", ref_output)
    if ref_failures:
        return [f"reference run: {f}" for f in ref_failures]

    different, error = count_different_pixels(screenshot, ref_screenshot, not sc["game"],
                                              os.path.join(out_dir, f"{name}_reference_diff.png"))
    if error:
        return [f"vs reference: {error}"]
    if different <= TOLERANCE:
        return []

    failures = [f"{different} pixels differ from reference renderer (limit {TOLERANCE}), "
                f"mask: {name}_reference_diff.png"]
    # A frame can cover several update ticks, so the screenshot tick may differ by one
    ticks = [SCREENSHOT_TICK_PATTERN.search(o) for o in (output, ref_output)]
    if all(ticks) and ticks[0].group(1) != ticks[1].group(1):
        failures.append(f"screenshots were taken at different game ticks: {ticks[0].group(1)} "
                        f"vs {ticks[1].group(1)} (reference) - rerun")
    return failures


def write_log(out_dir, name, output):
    with open(os.path.join(out_dir, f"{name}.log"), "w") as log:
        log.write(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("binary", help="game binary inside a build directory")
    parser.add_argument("--reference", metavar="BINARY",
                        help="run the extended scenarios and compare every screenshot with this binary's")
    parser.add_argument("--out", metavar="DIR", default=DEFAULT_OUT_DIR,
                        help="directory for screenshots and logs; old ones are removed (default: .tmp/render_test)")
    parser.add_argument("--only", nargs="+", metavar="NAME", help="run only these scenarios")
    parser.add_argument("--verbose", action="store_true", help="print the game's log lines")
    args = parser.parse_args()

    binary = os.path.abspath(args.binary)
    reference = os.path.abspath(args.reference) if args.reference else None
    for path in filter(None, [binary, reference]):
        if not os.access(path, os.X_OK):
            sys.exit(f"not an executable: {path}")

    out_dir = os.path.abspath(args.out)
    os.makedirs(out_dir, exist_ok=True)
    for old in glob.glob(os.path.join(out_dir, "*.png")) + glob.glob(os.path.join(out_dir, "*.log")):
        os.remove(old)

    scenarios = [sc for sc in SCENARIOS
                 if (reference or not sc["extended"]) and (not args.only or sc["name"] in args.only)]
    failed = 0
    for sc in scenarios:
        name = sc["name"]
        screenshot = os.path.join(out_dir, f"{name}.png")
        failures, output = run_game(binary, sc, screenshot)
        write_log(out_dir, name, output)
        if reference and not failures:
            failures += compare_with_reference(reference, sc, screenshot, output, out_dir)

        print(f"[{'PASS' if not failures else 'FAIL'}] {name}")
        if args.verbose:
            for match in LOG_PATTERN.finditer(output):
                print(f"         {match.group(0)}")
        for failure in failures:
            print(f"   !!    {failure}")
        failed += bool(failures)

    print(f"\nScreenshots and logs in {out_dir}")
    print(f"{len(scenarios) - failed}/{len(scenarios)} scenarios passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
