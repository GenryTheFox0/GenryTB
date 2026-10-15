# -*- coding: utf-8 -*-
"""GenryTB's icon, drawn here (none of the game's art): the black bunny in its jacket against the grey fog of the
game's menu, two white eyes glowing. -> data/genrytb.ico (16..256) + data/genrytb_icon.png (1024)."""
import math
import os
import random

from PIL import Image, ImageChops, ImageDraw, ImageFilter

S = 1024
OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data')
random.seed(7)


def noise(size, scale, amp):
    small = Image.effect_noise((max(1, size // scale), max(1, size // scale)), amp)
    return small.resize((size, size), Image.BICUBIC)


# ---- the fog: a grey that is lighter low and to the sides, painted streaks running down (the menu's rain)
bg = Image.new('L', (S, S))
px = bg.load()
for y in range(S):
    for x in range(S):
        d = math.hypot((x - S / 2) / S, (y - S * 0.62) / S)
        px[x, y] = int(max(0, min(255, 150 - 40 * (1 - y / S) + 60 * d)))
streaks = Image.new('L', (S, S), 0)
dr = ImageDraw.Draw(streaks)
for i in range(140):
    x = random.randint(0, S)
    y = random.randint(-200, S)
    dr.line([(x, y), (x + random.randint(-6, 6), y + random.randint(150, 420))], fill=random.randint(30, 90), width=random.randint(2, 9))
streaks = streaks.filter(ImageFilter.GaussianBlur(3))
bg = ImageChops.subtract(bg, streaks.point(lambda v: v // 2))
bg = ImageChops.add(bg, noise(S, 2, 40).point(lambda v: (v - 128) // 8 + 0 if v > 128 else 0))
bg = bg.filter(ImageFilter.GaussianBlur(1.2))
# the corners and the top sink into the dark
vig = Image.new('L', (S, S), 0)
vp = vig.load()
for y in range(0, S):
    for x in range(0, S):
        r = math.hypot((x - S / 2) / (S / 2), (y - S * 0.58) / (S / 2))
        vp[x, y] = int(max(0, min(255, (r - 0.55) * 230)))
bg = ImageChops.subtract(bg, vig)

# ---- the bunny: ears, the head, the jacket's shoulders - one black shape with a furry edge
shape = Image.new('L', (S, S), 0)
d = ImageDraw.Draw(shape)


def ellipse_rot(cx, cy, rx, ry, ang, fill=255):
    pts = []
    for k in range(120):
        t = 2 * math.pi * k / 120
        x, y = rx * math.cos(t), ry * math.sin(t)
        a = math.radians(ang)
        pts.append((cx + x * math.cos(a) - y * math.sin(a), cy + x * math.sin(a) + y * math.cos(a)))
    d.polygon(pts, fill=fill)


ellipse_rot(365, 300, 78, 250, -14)        # the left ear
ellipse_rot(668, 285, 80, 262, 16)          # the right ear
ellipse_rot(512, 600, 255, 225, 0)          # the head
ellipse_rot(512, 690, 290, 170, 0)          # the cheeks, wide and low
d.polygon([(150, S), (250, 820), (512, 760), (774, 820), (874, S)], fill=255)   # the jacket's shoulders
# the fur: the edge broken by noise, then hardened again
edge = shape.filter(ImageFilter.GaussianBlur(11))
fur = noise(S, 1, 160).filter(ImageFilter.GaussianBlur(0.8))
shape = ImageChops.add(edge, fur.point(lambda v: (v - 110) // 2 if v > 110 else 0)).point(lambda v: 255 if v > 150 else 0)
# tufts: short strokes sticking out of the outline (the game's bunny is shaggy)
tuft = ImageDraw.Draw(shape)
outline = ImageChops.subtract(shape.filter(ImageFilter.MaxFilter(3)), shape)
ox = [(x, y) for y in range(0, S, 3) for x in range(0, S, 3) if outline.getpixel((x, y)) > 0]
random.shuffle(ox)
cx0, cy0 = 512, 600
for (x, y) in ox[:900]:
    a = math.atan2(y - cy0, x - cx0) + random.uniform(-0.5, 0.5)
    L = random.uniform(6, 22)
    tuft.line([(x, y), (x + L * math.cos(a), y + L * math.sin(a))], fill=255, width=random.choice([2, 3]))
shape = shape.filter(ImageFilter.GaussianBlur(1.4))

# ---- the eyes: slanted, angry, glowing white
eyes = Image.new('L', (S, S), 0)
e = ImageDraw.Draw(eyes)
e.polygon([(330, 585), (455, 618), (440, 655), (360, 640)], fill=255)     # left
e.polygon([(694, 585), (569, 618), (584, 655), (664, 640)], fill=255)     # right
eyes = eyes.filter(ImageFilter.GaussianBlur(2.5))
glow = eyes.filter(ImageFilter.GaussianBlur(28)).point(lambda v: min(255, v * 2))

# ---- together
img = Image.merge('RGB', (bg, bg, bg))
black = Image.new('RGB', (S, S), (8, 8, 9))
img = Image.composite(black, img, shape)
white = Image.new('RGB', (S, S), (255, 255, 255))
img = Image.composite(white, img, glow.point(lambda v: v * 6 // 10))
img = Image.composite(white, img, eyes)
# a thin rim of light on the fur against the fog
rim = ImageChops.subtract(shape.filter(ImageFilter.MaxFilter(5)), shape).filter(ImageFilter.GaussianBlur(2))
img = Image.composite(Image.new('RGB', (S, S), (120, 120, 124)), img, rim.point(lambda v: v // 3))

# ---- the plate: rounded corners, a dark frame
mask = Image.new('L', (S, S), 0)
ImageDraw.Draw(mask).rounded_rectangle([0, 0, S - 1, S - 1], radius=190, fill=255)
frame = Image.new('L', (S, S), 0)
ImageDraw.Draw(frame).rounded_rectangle([10, 10, S - 11, S - 11], radius=182, outline=255, width=18)
img = Image.composite(Image.new('RGB', (S, S), (14, 14, 16)), img, frame.filter(ImageFilter.GaussianBlur(2)))
out = img.convert('RGBA')
out.putalpha(mask)
os.makedirs(OUT, exist_ok=True)
out.save(os.path.join(OUT, 'genrytb_icon.png'))
# small sizes: sharpened a little so the eyes stay two points
sizes = [16, 24, 32, 48, 64, 128, 256]
out.save(os.path.join(OUT, 'genrytb.ico'), sizes=[(s, s) for s in sizes])
prev = Image.new('RGBA', (sum(sizes) + 20 * len(sizes), 256), (40, 40, 40, 255))
x = 0
for s in sizes:
    prev.alpha_composite(out.resize((s, s), Image.LANCZOS), (x, 256 - s))
    x += s + 20
os.makedirs(os.path.join(OUT, '..', 'work'), exist_ok=True)
prev.save(os.path.join(OUT, '..', 'work', 'icon_sizes.png'))
print('ok')
