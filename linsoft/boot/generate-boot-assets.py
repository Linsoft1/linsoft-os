"""Generate original Linsoft boot artwork and a GRUB-compatible DejaVu font."""

import math
import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


def make_splash() -> None:
    root = Path(__file__).resolve().parent
    font_path = root.parents[2] / "linsoft-desktop-drm-backup" / "assets" / "DejaVuSans.ttf"
    image = Image.new("RGB", (960, 540), (9, 16, 32))
    draw = ImageDraw.Draw(image)
    cx, cy = 480, 220
    for degree in range(360):
        angle = math.radians(degree)
        blend = (math.sin(angle) + 1) / 2
        colour = tuple(round(a + (b - a) * blend)
                       for a, b in zip((65, 202, 211), (152, 113, 255)))
        x, y = cx + math.cos(angle) * 61, cy + math.sin(angle) * 61
        draw.ellipse((x - 2, y - 2, x + 2, y + 2), fill=colour)
    draw.line((cx - 15, cy - 25, cx - 15, cy + 23, cx + 24, cy + 23),
              fill="#e2f2ff", width=9, joint="curve")
    for text, size, y, colour in (
        ("Linsoft OS", 28, 305, "#edf6ff"),
        ("Spusta sa system", 14, 425, "#93b7d7"),
        ("Esc  |  Preskocit animaciu", 11, 500, "#607a94"),
    ):
        draw.text((480, y), text, font=ImageFont.truetype(str(font_path), size),
                  fill=colour, anchor="mt")
    image.save(root / "splash.ppm")
    print("Generated original Linsoft startup logo; animation is rendered at boot.")


def section(name: bytes, data: bytes) -> bytes:
    return name + struct.pack(">I", len(data)) + data


def make_font(font_path: Path, output: Path) -> None:
    font = ImageFont.truetype(str(font_path), 20)
    ascent, descent = font.getmetrics()
    glyphs: list[bytes] = []
    for code in range(32, 127):
        char = chr(code)
        left, top, right, bottom = font.getbbox(char)
        width, height = max(1, right - left), max(1, bottom - top)
        image = Image.new("L", (width, height))
        ImageDraw.Draw(image).text((-left, -top), char, font=font, fill=255)
        values = image.tobytes()
        bitmap = bytearray((width * height + 7) // 8)
        for index, value in enumerate(values):
            if value >= 128:
                bitmap[index // 8] |= 0x80 >> (index % 8)
        glyphs.append(
            struct.pack(">HHhhh", width, height, left, ascent - bottom,
                        round(font.getlength(char))) + bitmap
        )
    header = section(b"FILE", b"PFF2")
    for name, value in ((b"NAME", b"Linsoft Sans Regular 20"),
                        (b"FAMI", b"Linsoft Sans"), (b"WEIG", b"normal"),
                        (b"SLAN", b"normal")):
        header += section(name, value + b"\0")
    for name, value in ((b"PTSZ", 20), (b"MAXW", 32), (b"MAXH", 32),
                        (b"ASCE", ascent), (b"DESC", descent)):
        header += section(name, struct.pack(">H", value))
    offset = len(header) + 8 + 95 * 9 + 8
    index = bytearray()
    for code, glyph in zip(range(32, 127), glyphs):
        index.extend(struct.pack(">IBI", code, 0, offset))
        offset += len(glyph)
    output.write_bytes(header + section(b"CHIX", bytes(index)) +
                       b"DATA\xff\xff\xff\xff" + b"".join(glyphs))


def main() -> None:
    root = Path(__file__).resolve().parent
    font_path = root.parents[2] / "linsoft-desktop-drm-backup" / "assets" / "DejaVuSans.ttf"
    width, height = 1280, 720
    image = Image.new("RGB", (width, height))
    pixels = image.load()
    for y in range(height):
        for x in range(width):
            glow = max(0.0, 1 - math.hypot((x / width - .72) * 1.4, y / height - .38))
            pixels[x, y] = (int(9 + 9 * glow), int(16 + 20 * glow), int(32 + 35 * glow))
    draw = ImageDraw.Draw(image)
    draw.arc((620, -150, 1450, 670), 80, 280, fill="#283f65", width=2)
    draw.arc((680, -80, 1390, 600), 100, 300, fill="#223756", width=1)
    cx, cy = 640, 170
    for degree in range(360):
        angle = math.radians(degree)
        blend = (math.sin(angle) + 1) / 2
        ink = tuple(round(a + (b - a) * blend)
                    for a, b in zip((65, 202, 211), (152, 113, 255)))
        x, y = cx + math.cos(angle) * 70, cy + math.sin(angle) * 54
        draw.ellipse((x - 4, y - 4, x + 4, y + 4), fill=ink)
    draw.line((cx - 23, cy - 25, cx - 23, cy + 26, cx + 27, cy + 26),
              fill="#e2f2ff", width=10, joint="curve")
    for text, size, y, colour in (
        ("Linsoft OS", 40, 252, "#edf6ff"),
        ("Vas priestor. Vas system.", 17, 310, "#93b7d7"),
        ("LINSOFT  /  START", 13, 46, "#69c9e5"),
    ):
        font = ImageFont.truetype(str(font_path), size)
        draw.text((width / 2, y), text, font=font, fill=colour, anchor="mt")
    draw.line((410, 350, 870, 350), fill="#2e4766")
    image.save(root / "background.png")
    make_font(font_path, root / "linsoft.pf2")
    print("Generated original 1280x720 artwork and PF2 font (ASCII, size 20)")


if __name__ == "__main__":
    if sys.argv[1:] == ["--splash-only"]:
        make_splash()
    elif not sys.argv[1:]:
        main()
    else:
        raise SystemExit("Usage: generate-boot-assets.py [--splash-only]")
