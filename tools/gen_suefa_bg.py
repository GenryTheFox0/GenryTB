# -*- coding: utf-8 -*-
"""GenryTB: places for камень-ножницы-бумага - the game's own field (interface/suefa/fon.png) redrawn by
Qwen-Image-Edit 2511 into the places of the game (home, school, the yard, the garages, the forest, the village, the
field) at three times of the day, its grey painted look and the calm middle (where the hands are) kept.
    python tools/gen_suefa_bg.py [place_time ...]     (no names: all; made ones skipped, --force redoes)
Out: data/assets/suefa_bg/<place>_<time>.jpg (1920x1080) + data/assets/suefa_bg/sheet.jpg"""
import os, sys, time

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_hands as gh  # noqa: E402  (ComfyUI upload/run, the Qwen edit graph)

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data', 'assets', 'suefa_bg')
PLACES = {
    'dom':     'the inside of a small old Soviet apartment room in the 1990s: patterned wallpaper, a carpet hanging on the '
               'wall, an old sideboard, a window with lace curtains',
    'shkola':  'an old Soviet school classroom: rows of wooden desks, a blackboard on the wall, tall windows',
    'dvor':    'a snowy courtyard between grey five-storey Soviet apartment blocks, bare trees, an old playground',
    'garazhi': 'a row of rusty metal garages in deep snow, a crooked fence, bare bushes',
    'les':     'a dark winter pine forest, deep snow on the ground, tall pine trunks',
    'derevnya': 'a snowy village street, small wooden houses with smoking chimneys, a wooden fence',
    'pole':    'the same empty snowy field under the sky',
}
TIMES = {
    'den':   'on an overcast winter day, soft grey daylight',
    'vecher': 'at winter dusk, dim bluish light, a few warm lit windows or lamps',
    'noch':  'at night, deep darkness, faint moonlight and one weak lamp light, heavy shadows',
}
KEEP = ('Keep the same grayscale painted charcoal style of the original picture, muted grey tones, light fog, the whole '
        'picture slightly soft like an out of focus background. Keep the middle of the picture calm and empty: no objects, '
        'no people and no animals in the centre. No text.')


def make(key, force=False):
    place, when = key.split('_')
    fn = os.path.join(OUT, key + '.jpg')
    if os.path.exists(fn) and not force:
        return
    os.makedirs(OUT, exist_ok=True)
    src = Image.open(os.path.join(gh.SRC, 'fon.png')).convert('RGB')
    name = gh.upload(src, 'genry_tb_suefa_fon.png')
    text = 'Turn this picture into %s, %s. %s' % (PLACES[place], TIMES[when], KEEP)
    nodes, _ = gh.graphs.edit({'instruction': text}, name, 2000 + sum(map(ord, key)))
    t = time.time()
    img = gh.run(nodes).convert('L').convert('RGB').resize((1920, 1080), Image.LANCZOS)   # grey, as the game's field
    img.save(fn, quality=92)
    print('%s  %.0f s' % (key, time.time() - t), flush=True)


def sheet():
    keys = [p + '_' + t for p in PLACES for t in TIMES]
    have = [k for k in keys if os.path.exists(os.path.join(OUT, k + '.jpg'))]
    cols = 3
    s = Image.new('RGB', (cols * 480, ((len(have) + cols - 1) // cols) * 270))
    for i, k in enumerate(have):
        s.paste(Image.open(os.path.join(OUT, k + '.jpg')).resize((480, 270)), ((i % cols) * 480, (i // cols) * 270))
    s.save(os.path.join(OUT, 'sheet.jpg'), quality=85)


if __name__ == '__main__':
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    for k in args or [p + '_' + t for p in PLACES for t in TIMES]:
        make(k, '--force' in sys.argv)
    sheet()
