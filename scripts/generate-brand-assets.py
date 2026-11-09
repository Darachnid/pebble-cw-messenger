#!/usr/bin/env python3
"""Generate Pebble menu/splash icons and Android launcher PNGs (CW paddle motif)."""
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
RES = ROOT / "resources" / "images"
ANDROID = ROOT / "android" / "app" / "src" / "main" / "res"


def draw_icon(size: int) -> Image.Image:
    img = Image.new("RGBA", (size, size), (12, 12, 14, 255))
    d = ImageDraw.Draw(img)
    margin = size // 8
    # Paddle lever + knob (straight-key cue)
    cx = size // 2 + size // 16
    base_y = size - margin
    knob_y = margin + size // 10
    d.rectangle((margin, base_y - 2, size - margin, base_y + 1), fill=(60, 60, 65, 255))
    d.line((cx, base_y - 2, cx, knob_y + size // 12), fill=(230, 230, 235, 255), width=max(2, size // 20))
    r = max(3, size // 10)
    d.ellipse((cx - r, knob_y - r, cx + r, knob_y + r), fill=(0, 200, 220, 255))
    # Dit dah dots
    d.ellipse((margin + 2, base_y - size // 4, margin + 2 + r, base_y - size // 4 + r), fill=(255, 180, 0, 255))
    d.rectangle((margin + 2, base_y - size // 3, margin + 2 + size // 5, base_y - size // 3 + r), fill=(255, 180, 0, 255))
    return img


def main() -> None:
    RES.mkdir(parents=True, exist_ok=True)
    # Pebble launcher menu icon must be <= 25x25 (see SDK process_sdk_resources).
    draw_icon(25).save(RES / "menu_icon.png")
    splash = draw_icon(160)
    splash.save(RES / "tile_splash.png")

    for dpi, size in (("mdpi", 48), ("hdpi", 72), ("xhdpi", 96), ("xxhdpi", 144), ("xxxhdpi", 192)):
        d = ANDROID / f"mipmap-{dpi}"
        d.mkdir(parents=True, exist_ok=True)
        draw_icon(size).save(d / "ic_launcher.png")

    docs = ROOT / "docs" / "store"
    docs.mkdir(parents=True, exist_ok=True)
    draw_icon(512).save(docs / "app-icon-512.png")
    draw_icon(80).save(docs / "pebble-menu-icon-80.png")
    print("Wrote resources/images and android mipmap icons")

if __name__ == "__main__":
    main()
