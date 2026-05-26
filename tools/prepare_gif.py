#!/usr/bin/env python3
"""Resize a GIF so AnimatedGIF can draw it cleanly on the 240x240 ST7789."""
from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageSequence


DISPLAY_SIZE = (240, 240)


def fit_frame(frame: Image.Image) -> Image.Image:
    rgba = frame.convert("RGBA")
    rgba.thumbnail(DISPLAY_SIZE, Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", DISPLAY_SIZE, (0, 0, 0, 255))
    x = (DISPLAY_SIZE[0] - rgba.width) // 2
    y = (DISPLAY_SIZE[1] - rgba.height) // 2
    canvas.alpha_composite(rgba, (x, y))
    return canvas.convert("P", palette=Image.Palette.ADAPTIVE, colors=256)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("src", nargs="?", default="assets/sample.gif")
    parser.add_argument("dst", nargs="?", default="assets/sample_240.gif")
    args = parser.parse_args()

    src = Path(args.src)
    dst = Path(args.dst)
    image = Image.open(src)
    frames = [fit_frame(frame.copy()) for frame in ImageSequence.Iterator(image)]
    duration = image.info.get("duration", 70)

    dst.parent.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        dst,
        save_all=True,
        append_images=frames[1:],
        duration=duration,
        loop=image.info.get("loop", 0),
        optimize=True,
    )

    print(
        f"{src}: {image.size[0]}x{image.size[1]}, {len(frames)} frames, "
        f"{src.stat().st_size} bytes"
    )
    print(f"{dst}: 240x240, {dst.stat().st_size} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
