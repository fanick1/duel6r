#!/usr/bin/env python3
"""
Rendering tests for Duel 6 Reloaded: menu and in-game screenshots, with and without
window resizes, fullscreen toggling and split screen.

Every scenario starts the game with automation flags, lets it run for a few seconds and
takes a screenshot. The screenshot must have the window's size (taken from the game's
own "[video] screenshot" log line, so fullscreen works too), the frame must reach the
right and bottom window edges (a stale backbuffer leaves a black band there), the game
must exit cleanly and the Vulkan validation layers must not report anything.

Some scenarios additionally compare against another screenshot:
  * "same_as": the same binary without resizing must render the identical frame, e.g.
    after shrinking and growing back (a frame is identified by its game tick, and the
    game is seeded, so runs are reproducible);
  * with --reference BINARY, every scenario is also run with that binary (normally the
    OpenGL ES 3 build) and the two screenshots are compared pixel by pixel. The menu's
    animated banner strip is left out of that comparison.

Usage:
    tests/render_test.py BINARY [--reference BINARY] [--keep DIR] [--only NAME...]

Binaries run from their own directory, so build directories need the resource symlinks
(data, levels, profiles, sound, textures, shaders). Requires Pillow.
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile

try:
    from PIL import Image, ImageChops
except ImportError:
    sys.exit("Pillow is required: python3 -m pip install pillow")

SEED = 1
LEVEL = "duel_01"
FULL = (1280, 720)
SMALL = (1000, 600)
JITTER = [(1001, 600), (1001, 601), (1000, 601), (1000, 600)]
STRESS = [(1100, 650), (900, 700), (1280, 500), (700, 480), (1250, 710), (1000, 600)]

# Edge strip checked for content, and how dark a pixel must be to count as "black"
EDGE_STRIP = 8
BLACK_THRESHOLD = 16
MAX_BLACK_RATIO = 0.5

# Pixel comparison: a pixel differs when a channel differs by more than PIXEL_TOLERANCE
PIXEL_TOLERANCE = 64
# The menu's animated banner: the GUI is centred vertically in a 900 px tall layout
MENU_LAYOUT_HEIGHT = 900
MENU_BANNER_HEIGHT = 120
TIMEOUT = 40


# Reference tolerance (pixels). All scenarios are identical today; 50 leaves room for driver
# differences while still catching e.g. static water (~180 px) or a culled face (~40 px).
TOLERANCE = 50


def scenario(name, game=False, start=FULL, resizes=(), interval=0.5, fullscreen=False, split=False,
             same_as=None, edges=True, screenshot_after=None):
    return dict(name=name, game=game, start=start, resizes=list(resizes), interval=interval,
                fullscreen=fullscreen, split=split, same_as=same_as, edges=edges, screenshot_after=screenshot_after)


SCENARIOS = [
    scenario("menu-baseline"),
    scenario("menu-shrink", resizes=[SMALL]),
    scenario("menu-grow", start=SMALL, resizes=[FULL]),
    scenario("menu-grow-back", resizes=[SMALL, FULL], same_as="menu-baseline"),
    scenario("menu-jitter", start=SMALL, resizes=JITTER, interval=0.3),
    scenario("menu-stress", resizes=STRESS, interval=0.15),
    scenario("menu-fullscreen", fullscreen=True),
    scenario("game-baseline", game=True),
    # A later tick: the water wave is in another phase, so it must still match the reference
    scenario("game-later", game=True, screenshot_after=5.5),
    scenario("game-shrink", game=True, resizes=[SMALL]),
    scenario("game-grow-back", game=True, resizes=[SMALL, FULL], same_as="game-baseline"),
    scenario("game-jitter", game=True, start=SMALL, resizes=JITTER, interval=0.3),
    # Split screen leaves black borders around the views, so the edge check does not apply
    scenario("game-split", game=True, split=True, edges=False),
    scenario("game-split-shrink", game=True, split=True, resizes=[SMALL], edges=False),
]

LOG_PATTERN = re.compile(r"^\[(video|vulkan|automation)\].*$", re.MULTILINE)
VK_ERROR_PATTERN = re.compile(r"^\[VK ERROR\].*$", re.MULTILINE)
SCREENSHOT_SIZE_PATTERN = re.compile(r"^\[video\] screenshot.*?window (\d+)x(\d+)", re.MULTILINE)


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
    """Run one scenario and return (failures, output). The screenshot is taken ~1s after the
    last resize settled; the game quits shortly after."""
    if os.path.exists(screenshot):
        os.remove(screenshot)

    resize_after = 1.0
    last_action = resize_after + max(len(sc["resizes"]) - 1, 0) * sc["interval"]
    if sc["fullscreen"]:
        last_action = max(last_action, resize_after + 1.5)  # toggleFullscreen settles after 1s
    screenshot_after = last_action + 1.5
    if sc["game"]:
        screenshot_after = max(screenshot_after, 4.0)  # let the round-start overlay fade out
    if sc["screenshot_after"]:
        screenshot_after = sc["screenshot_after"]

    cmd = [binary, "-log-stdout", "-seed", str(SEED), "-window", f"{sc['start'][0]}x{sc['start'][1]}",
           "-screenshot", screenshot, "-screenshot-after", str(screenshot_after),
           "-quit-after", str(screenshot_after + 0.3)]
    if sc["game"]:
        cmd += ["-play", "-level", LEVEL]
    if sc["split"]:
        cmd += ["-split-screen"]
    if sc["resizes"]:
        cmd += ["-resize", ",".join(f"{w}x{h}" for w, h in sc["resizes"]),
                "-resize-after", str(resize_after), "-resize-interval", str(sc["interval"])]
    if sc["fullscreen"]:
        cmd += ["-fullscreen-after", str(resize_after)]

    try:
        proc = subprocess.run(cmd, cwd=os.path.dirname(binary), capture_output=True, text=True,
                              errors="replace", timeout=TIMEOUT)
    except subprocess.TimeoutExpired as e:
        output = e.stdout if isinstance(e.stdout, str) else (e.stdout or b"").decode(errors="replace")
        return [f"game did not quit within {TIMEOUT}s"], output
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

    if sc["edges"]:
        right = black_ratio(image, (width - EDGE_STRIP, 0, width, height))
        bottom = black_ratio(image, (0, height - EDGE_STRIP, width, height))
        if right > MAX_BLACK_RATIO:
            failures.append(f"right edge is {right:.0%} black - frame does not reach the window edge")
        if bottom > MAX_BLACK_RATIO:
            failures.append(f"bottom edge is {bottom:.0%} black - frame does not reach the window edge")

    return failures, output


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("binary", help="game binary inside a build directory")
    parser.add_argument("--reference", metavar="BINARY", help="binary of another renderer to compare with")
    parser.add_argument("--keep", metavar="DIR", help="directory to keep screenshots in (default: temporary)")
    parser.add_argument("--only", nargs="+", metavar="NAME", help="run only these scenarios")
    parser.add_argument("--verbose", action="store_true", help="print the game's log lines")
    args = parser.parse_args()

    binary = os.path.abspath(args.binary)
    reference = os.path.abspath(args.reference) if args.reference else None
    for path in filter(None, [binary, reference]):
        if not os.access(path, os.X_OK):
            sys.exit(f"not an executable: {path}")

    out_dir = os.path.abspath(args.keep) if args.keep else tempfile.mkdtemp(prefix="duel6r_render_")
    os.makedirs(out_dir, exist_ok=True)

    scenarios = [sc for sc in SCENARIOS if not args.only or sc["name"] in args.only]
    screenshots = {}
    failed = 0
    for sc in scenarios:
        name = sc["name"]
        label = f"{name}: {sc['start'][0]}x{sc['start'][1]}" + "".join(f" -> {w}x{h}" for w, h in sc["resizes"])
        if sc["fullscreen"]:
            label += " -> fullscreen"

        screenshot = os.path.join(out_dir, f"{name}.png")
        skip_banner = not sc["game"]
        failures, output = run_game(binary, sc, screenshot)
        if os.path.exists(screenshot):
            screenshots[name] = screenshot

        if sc["same_as"] and name in screenshots:
            other = screenshots.get(sc["same_as"])
            if not other:
                failures.append(f"cannot compare with {sc['same_as']}: no screenshot")
            else:
                different, error = count_different_pixels(screenshot, other, skip_banner,
                                                          os.path.join(out_dir, f"{name}_same_diff.png"))
                if error:
                    failures.append(f"vs {sc['same_as']}: {error}")
                elif different > 0:
                    failures.append(f"{different} pixels differ from {sc['same_as']} (same binary, same tick)")

        if reference and name in screenshots:
            ref_screenshot = os.path.join(out_dir, f"{name}_reference.png")
            ref_failures, ref_output = run_game(reference, sc, ref_screenshot)
            if ref_failures:
                failures += [f"reference run: {f}" for f in ref_failures]
            else:
                different, error = count_different_pixels(screenshot, ref_screenshot, skip_banner,
                                                          os.path.join(out_dir, f"{name}_reference_diff.png"))
                if error:
                    failures.append(f"vs reference: {error}")
                elif different > TOLERANCE:
                    failures.append(f"{different} pixels differ from reference renderer "
                                    f"(limit {TOLERANCE}), mask: {name}_reference_diff.png")

        print(f"[{'PASS' if not failures else 'FAIL'}] {label}")
        if args.verbose:
            for match in LOG_PATTERN.finditer(output):
                print(f"         {match.group(0)}")
        for failure in failures:
            print(f"   !!    {failure}")
        failed += bool(failures)

    print(f"\nScreenshots in {out_dir}")
    print(f"{len(scenarios) - failed}/{len(scenarios)} scenarios passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
