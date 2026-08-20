#!/usr/bin/env python3
"""Generate readable Nebbie app icons (editor + translate) at all platform sizes."""

from __future__ import annotations

import io
import math
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError as exc:  # pragma: no cover
    raise SystemExit("Pillow is required: pip install pillow") from exc

ROOT = Path(__file__).resolve().parents[1]

# Nebbie palette — high contrast, readable at 16px.
COLORS = {
    "bg_top": (42, 24, 82),
    "bg_bottom": (18, 10, 40),
    "mist": (110, 82, 168, 48),
    "grid_dark": (58, 38, 98),
    "grid_mid": (88, 62, 138),
    "grid_line": (150, 118, 198),
    "gold": (232, 184, 48),
    "gold_light": (245, 216, 120),
    "paper": (236, 228, 210),
    "ink": (52, 34, 88),
    "teal": (72, 188, 176),
    "white": (255, 255, 255),
}

LINUX_SIZES = (16, 22, 24, 32, 48, 64, 128, 256, 512, 1024)
ICO_SIZES = (16, 32, 48, 64, 128, 256)
ICNS_SLOTS = (
    ("icp4", 16),
    ("icp5", 32),
    ("icp6", 64),
    ("ic07", 128),
    ("ic08", 256),
    ("ic09", 512),
    ("ic10", 1024),
)


@dataclass(frozen=True)
class AppIconSpec:
    name: str
    master_png: Path
    icns: Path
    ico: Path
    hicolor_dir: Path


APP_SPECS = {
    "nebbieedit": AppIconSpec(
        name="nebbieedit",
        master_png=ROOT / "nebbie-qt" / "icons" / "nebbieedit-1024.png",
        icns=ROOT / "nebbie-qt" / "icons" / "nebbieedit.icns",
        ico=ROOT / "nebbie-qt" / "icons" / "nebbieedit.ico",
        hicolor_dir=ROOT / "nebbie-qt" / "icons" / "hicolor",
    ),
    "nebbie-translate": AppIconSpec(
        name="nebbie-translate",
        master_png=ROOT / "nebbie-translator" / "icons" / "nebbie-translate-1024.png",
        icns=ROOT / "nebbie-translator" / "icons" / "nebbie-translate.icns",
        ico=ROOT / "nebbie-translator" / "icons" / "nebbie-translate.ico",
        hicolor_dir=ROOT / "nebbie-translator" / "icons" / "hicolor",
    ),
}


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def lerp_color(c1: tuple[int, ...], c2: tuple[int, ...], t: float) -> tuple[int, ...]:
    return tuple(int(lerp(a, b, t)) for a, b in zip(c1, c2))


def draw_background(size: int, *, teal_tint: float = 0.0) -> Image.Image:
    img = Image.new("RGBA", (size, size), COLORS["bg_bottom"])
    px = img.load()
    top = lerp_color(COLORS["bg_top"], COLORS["teal"], teal_tint * 0.35)
    bottom = lerp_color(COLORS["bg_bottom"], (16, 36, 44), teal_tint * 0.25)
    for y in range(size):
        t = y / max(size - 1, 1)
        row = lerp_color(top, bottom, t)
        for x in range(size):
            px[x, y] = row + (255,)

    overlay = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    mist_r = size * 0.34
    draw.ellipse(
        (size * 0.08 - mist_r * 0.2, size * 0.12 - mist_r * 0.3, size * 0.08 + mist_r, size * 0.12 + mist_r),
        fill=COLORS["mist"],
    )
    draw.ellipse(
        (size * 0.72 - mist_r, size * 0.78 - mist_r, size * 0.72 + mist_r * 0.8, size * 0.78 + mist_r * 0.7),
        fill=COLORS["mist"],
    )
    return Image.alpha_composite(img, overlay)


def rounded_rect(
    draw: ImageDraw.ImageDraw,
    box: tuple[float, float, float, float],
    radius: float,
    fill,
    outline=None,
    width: int = 1,
) -> None:
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def draw_editor_icon(size: int) -> Image.Image:
    img = draw_background(size)
    draw = ImageDraw.Draw(img)

    margin = size * 0.16
    grid_size = size - margin * 2
    cell = grid_size / 3.0
    gap = max(2, size * 0.018)
    radius = max(3, size * 0.045)
    stroke = max(2, int(size * 0.014))

    origin_x = margin
    origin_y = margin

    active = {(1, 1), (1, 0), (1, 2), (0, 1), (2, 1)}
    mob_cell = (0, 0)
    obj_cell = (2, 2)

    for row in range(3):
        for col in range(3):
            x0 = origin_x + col * cell + gap / 2
            y0 = origin_y + row * cell + gap / 2
            x1 = origin_x + (col + 1) * cell - gap / 2
            y1 = origin_y + (row + 1) * cell - gap / 2
            if (col, row) == (1, 1):
                fill = COLORS["gold"]
                outline = COLORS["gold_light"]
            elif (col, row) in active:
                fill = COLORS["grid_mid"]
                outline = COLORS["gold"]
            else:
                fill = COLORS["grid_dark"]
                outline = COLORS["grid_line"]
            rounded_rect(draw, (x0, y0, x1, y1), radius, fill=fill, outline=outline, width=stroke)

            cx = (x0 + x1) / 2
            cy = (y0 + y1) / 2
            marker = max(3, size * 0.035)
            if (col, row) == mob_cell:
                draw.ellipse((cx - marker, cy - marker, cx + marker, cy + marker), fill=COLORS["white"])
            elif (col, row) == obj_cell:
                half = marker * 0.85
                draw.rectangle((cx - half, cy - half, cx + half, cy + half), fill=COLORS["gold_light"])

    # Exit notch on east wall of center room.
    cx0 = origin_x + cell + gap / 2
    cy0 = origin_y + cell + gap / 2
    cx1 = origin_x + 2 * cell - gap / 2
    cy1 = origin_y + 2 * cell - gap / 2
    notch_h = (cy1 - cy0) * 0.22
    notch_y0 = (cy0 + cy1) / 2 - notch_h / 2
    notch_y1 = notch_y0 + notch_h
    draw.rectangle((cx1 - stroke * 0.5, notch_y0, cx1 + stroke * 1.5, notch_y1), fill=COLORS["bg_bottom"])

    # Small compass star — "world editor".
    star_r = size * 0.055
    sx = size * 0.82
    sy = size * 0.22
    draw.polygon(
        [
            (sx, sy - star_r),
            (sx + star_r * 0.28, sy - star_r * 0.28),
            (sx + star_r, sy),
            (sx + star_r * 0.28, sy + star_r * 0.28),
            (sx, sy + star_r),
            (sx - star_r * 0.28, sy + star_r * 0.28),
            (sx - star_r, sy),
            (sx - star_r * 0.28, sy - star_r * 0.28),
        ],
        fill=COLORS["gold_light"],
    )

    return img


def draw_text_bar(
    draw: ImageDraw.ImageDraw,
    x0: float,
    x1: float,
    y: float,
    height: float,
    color,
) -> None:
    draw.rounded_rectangle((x0, y - height / 2, x1, y + height / 2), radius=height / 2, fill=color)


def draw_translate_icon(size: int) -> Image.Image:
    img = draw_background(size, teal_tint=0.55)
    draw = ImageDraw.Draw(img)

    doc_w = size * 0.56
    doc_h = size * 0.62
    doc_x0 = (size - doc_w) / 2
    doc_y0 = (size - doc_h) / 2
    doc_x1 = doc_x0 + doc_w
    doc_y1 = doc_y0 + doc_h
    doc_radius = size * 0.06
    stroke = max(2, int(size * 0.016))

    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    shadow_draw = ImageDraw.Draw(shadow)
    rounded_rect(
        shadow_draw,
        (doc_x0 + size * 0.02, doc_y0 + size * 0.025, doc_x1 + size * 0.02, doc_y1 + size * 0.025),
        doc_radius,
        fill=(0, 0, 0, 70),
    )
    img = Image.alpha_composite(img, shadow)
    draw = ImageDraw.Draw(img)

    rounded_rect(draw, (doc_x0, doc_y0, doc_x1, doc_y1), doc_radius, fill=COLORS["paper"], outline=COLORS["gold"], width=stroke)

    pad_x = doc_w * 0.14
    bar_h = max(3, size * 0.028)
    bar_x0 = doc_x0 + pad_x
    bar_x1 = doc_x1 - pad_x
    mid_y = (doc_y0 + doc_y1) / 2

    top_lines = (doc_y0 + doc_h * 0.22, doc_y0 + doc_h * 0.34, doc_y0 + doc_h * 0.46)
    for idx, y in enumerate(top_lines):
        width_factor = 1.0 - idx * 0.12
        x1 = bar_x0 + (bar_x1 - bar_x0) * width_factor
        draw_text_bar(draw, bar_x0, x1, y, bar_h, COLORS["ink"])

    bottom_lines = (doc_y0 + doc_h * 0.58, doc_y0 + doc_h * 0.70, doc_y0 + doc_h * 0.82)
    for idx, y in enumerate(bottom_lines):
        width_factor = 0.88 - idx * 0.1
        x1 = bar_x0 + (bar_x1 - bar_x0) * width_factor
        draw_text_bar(draw, bar_x0, x1, y, bar_h, COLORS["gold"])

    # Bidirectional translate arrows in the divider band.
    arrow_y = mid_y
    arrow_w = doc_w * 0.16
    arrow_h = max(4, size * 0.035)
    cx = (doc_x0 + doc_x1) / 2
    band_x0 = cx - arrow_w
    band_x1 = cx + arrow_w
    rounded_rect(draw, (band_x0, arrow_y - arrow_h, band_x1, arrow_y + arrow_h), arrow_h, fill=COLORS["teal"])

    tip = arrow_w * 0.22
    draw.polygon(
        [
            (band_x0 + tip * 0.4, arrow_y),
            (band_x0 + tip * 1.6, arrow_y - arrow_h * 0.75),
            (band_x0 + tip * 1.6, arrow_y + arrow_h * 0.75),
        ],
        fill=COLORS["white"],
    )
    draw.polygon(
        [
            (band_x1 - tip * 0.4, arrow_y),
            (band_x1 - tip * 1.6, arrow_y - arrow_h * 0.75),
            (band_x1 - tip * 1.6, arrow_y + arrow_h * 0.75),
        ],
        fill=COLORS["white"],
    )

    # Language badge (A / 文 simplified as two glyphs).
    badge_r = size * 0.075
    bx = doc_x0 + badge_r * 1.05
    by = doc_y0 + badge_r * 1.05
    draw.ellipse((bx - badge_r, by - badge_r, bx + badge_r, by + badge_r), fill=COLORS["gold"], outline=COLORS["gold_light"], width=stroke)
    font_size = max(8, int(badge_r * 1.15))
    draw.text((bx - badge_r * 0.34, by - badge_r * 0.58), "A", fill=COLORS["ink"])

    badge2_r = badge_r * 0.82
    bx2 = doc_x1 - badge2_r * 1.15
    by2 = doc_y1 - badge2_r * 1.15
    draw.ellipse(
        (bx2 - badge2_r, by2 - badge2_r, bx2 + badge2_r, by2 + badge2_r),
        fill=COLORS["teal"],
        outline=COLORS["white"],
        width=max(1, stroke - 1),
    )
    # Simple "文" hint: cross + base stroke.
    cx2 = bx2
    cy2 = by2
    s = badge2_r * 0.42
    draw.line((cx2 - s, cy2 - s * 0.2, cx2 + s, cy2 - s * 0.2), fill=COLORS["white"], width=max(2, int(size * 0.012)))
    draw.line((cx2, cy2 - s * 0.85, cx2, cy2 + s * 0.75), fill=COLORS["white"], width=max(2, int(size * 0.012)))
    draw.line((cx2 - s * 0.85, cy2 + s * 0.55, cx2 + s * 0.85, cy2 + s * 0.55), fill=COLORS["white"], width=max(2, int(size * 0.012)))

    return img


def png_bytes(image: Image.Image, size: int) -> bytes:
    resized = image.resize((size, size), Image.Resampling.LANCZOS)
    buf = io.BytesIO()
    resized.save(buf, format="PNG")
    return buf.getvalue()


def build_icns(image: Image.Image) -> bytes:
    body = bytearray()
    for otype, slot_size in ICNS_SLOTS:
        data = png_bytes(image, slot_size)
        chunk = otype.encode("ascii") + struct.pack(">I", len(data) + 8) + data
        while len(chunk) % 4:
            chunk += b"\x00"
        body.extend(chunk)
    return b"icns" + struct.pack(">I", 8 + len(body)) + bytes(body)


def build_ico(image: Image.Image) -> bytes:
    entries = []
    for slot_size in ICO_SIZES:
        entries.append((slot_size, image.resize((slot_size, slot_size), Image.Resampling.LANCZOS)))

    offset = 6 + 16 * len(entries)
    header = struct.pack("<HHH", 0, 1, len(entries))
    directory = bytearray()
    blobs = bytearray()

    for slot_size, img in entries:
        buf = io.BytesIO()
        img.save(buf, format="PNG")
        data = buf.getvalue()
        width = 0 if slot_size >= 256 else slot_size
        height = 0 if slot_size >= 256 else slot_size
        directory.extend(struct.pack("<BBBBHHII", width, height, 0, 0, 1, 32, len(data), offset))
        blobs.extend(data)
        offset += len(data)

    return header + bytes(directory) + bytes(blobs)


def write_platform_assets(spec: AppIconSpec, image: Image.Image) -> None:
    spec.master_png.parent.mkdir(parents=True, exist_ok=True)
    image.save(spec.master_png, format="PNG")
    print(f"Wrote {spec.master_png}")

    spec.icns.write_bytes(build_icns(image))
    print(f"Wrote {spec.icns}")

    spec.ico.write_bytes(build_ico(image))
    print(f"Wrote {spec.ico}")

    for slot_size in LINUX_SIZES:
        target = spec.hicolor_dir / f"{slot_size}x{slot_size}" / "apps" / f"{spec.name}.png"
        target.parent.mkdir(parents=True, exist_ok=True)
        image.resize((slot_size, slot_size), Image.Resampling.LANCZOS).save(target, format="PNG")
        print(f"Wrote {target}")


def generate(app: str) -> None:
    if app not in APP_SPECS:
        raise SystemExit(f"Unknown app: {app}")
    spec = APP_SPECS[app]
    if app == "nebbieedit":
        image = draw_editor_icon(1024)
    else:
        image = draw_translate_icon(1024)
    write_platform_assets(spec, image)


def main(argv: list[str]) -> int:
    targets = ["nebbieedit", "nebbie-translate"] if len(argv) < 2 else argv[1:]
    for target in targets:
        if target == "all":
            for app in APP_SPECS:
                generate(app)
            return 0
        generate(target)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
