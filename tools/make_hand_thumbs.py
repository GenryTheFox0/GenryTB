# -*- coding: utf-8 -*-
"""GenryTB: thumb.png of each hand set (its fist, R_03, cut to the hand) for the editor's picker; romka - the game's own."""
import os
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'work', 'suefa', 'images', 'interface', 'suefa')
OUT = os.path.join(ROOT, 'data', 'assets', 'hands')


def thumb(src, dst):
    im = Image.open(src).convert('RGBA')
    box = im.getchannel('A').point(lambda v: 255 if v > 100 else 0).getbbox()
    if not box:
        return
    l, t, r, b = box
    l = max(l, r - 700)                      # the fist and a bit of the sleeve, not the whole arm
    im = im.crop((l, max(0, t - 10), r + 10, b + 10))
    im.thumbnail((240, 140), Image.LANCZOS)
    im.save(dst)


os.makedirs(os.path.join(OUT, 'romka'), exist_ok=True)
thumb(os.path.join(SRC, 'R_03.png'), os.path.join(OUT, 'romka', 'thumb.png'))
for d in sorted(os.listdir(OUT)):
    f = os.path.join(OUT, d, 'R_03.png')
    if os.path.exists(f):
        thumb(f, os.path.join(OUT, d, 'thumb.png'))
        print(d)
