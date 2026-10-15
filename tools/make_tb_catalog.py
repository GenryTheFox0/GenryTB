"""Tiny Bunny's catalog for GenryTB (data/tb_catalog.json): what a mod can put on the screen and play,
read out of the game itself - its decompiled scripts (tools/tb_rpyc, unrpyc) and its archive's index
(game/archive.rpa, RPA-3.0, key 0x42424242).

  characters  every Character the game defines: its variable, name, colour, quote style
  sprites     every generate_img() the game makes (scripts/sprites/*): the image's name as `show` takes
              it ("Mom Normal m_day 01 norm aside") and its layers in the archive - body, eyes (the open
              frame), mouth, tears - in the order they are drawn; how often the game shows it
  backgrounds every plain `image name = "file"` / colour of the game's
  positions   the transforms the game puts sprites at (`at mom_right`), how often
  music       music/*, sounds/* (by folder), voice/<who>/*

    python make_tb_catalog.py
"""
import ast
import sys
import json
import pickle
import re
import zlib
from collections import Counter, defaultdict
from pathlib import Path

def _find_game():
    """the game's game/ folder: TB_GAME, else Steam's own place, else a SteamLibrary on any drive"""
    import os
    cands = [os.environ.get('TB_GAME', '')]
    cands += [os.path.expandvars(r'%ProgramFiles(x86)%\Steam\steamapps\common\TinyBunny')]
    cands += [d + r':\SteamLibrary\steamapps\common\TinyBunny' for d in 'CDEFGH']
    for c in cands:
        if c and (Path(c) / 'game').is_dir():
            return Path(c) / 'game'
    sys.exit('Tiny Bunny not found: set TB_GAME to its folder')


GAME = _find_game()
SRC = Path(__file__).parent / 'tb_rpyc'
sys.path.insert(0, str(Path(__file__).parent))
OUT = Path(__file__).parent.parent / 'data' / 'tb_catalog.json'
SPRITES_DIR = 'sprite'                      # sys_scripts.rpy: sprites_dir


class Names(set):
    """the game's files - its archive and the loose ones under game/ - found as Ren'Py finds them:
    whatever the letters' case ("h1" for a folder "H1")"""
    def __init__(self, items):
        super().__init__(items)
        self.lower = {}
        for n in items:
            self.lower.setdefault(n.lower(), n)

    def __contains__(self, p):
        return set.__contains__(self, p) or p.lower() in self.lower

    def real(self, p):
        return p if set.__contains__(self, p) else self.lower.get(p.lower())


def archive_names():
    with open(GAME / 'archive.rpa', 'rb') as f:
        head = f.readline().decode().split()
        f.seek(int(head[1], 16))
        idx = pickle.loads(zlib.decompress(f.read()), encoding='bytes')
    names = {(k.decode() if isinstance(k, bytes) else k) for k in idx}
    for p in GAME.rglob('*'):                       # the loose ones (game/images, music, voice...)
        if p.is_file() and p.suffix.lower() in ('.png', '.jpg', '.jpeg', '.webp', '.ogg', '.mp3', '.wav', '.ttf', '.otf'):
            names.add(p.relative_to(GAME).as_posix())
    return Names(names)


def find(names, path):
    """a file as Ren'Py finds it: as given, or under images/ (any case)"""
    for p in (path, 'images/' + path):
        r = names.real(p)
        if r:
            return r
    return None


def call_args(src, fname):
    """every fname(...) call in src: (args, kwargs) of literals"""
    out = []
    for m in re.finditer(r'\b' + fname + r'\((.*)\)\s*$', src, re.M):
        try:
            node = ast.parse(f'f({m.group(1)})', mode='eval').body
            args = [ast.literal_eval(a) for a in node.args]
            kw = {k.arg: ast.literal_eval(k.value) for k in node.keywords}
            out.append((args, kw))
        except Exception:
            pass
    return out


def sprite(names, args, kw):
    keys = ['npc', 'emotion', 'view', 'body', 'view2', 'eyes', 'mouth', 'drop', 'blur']
    a = dict(zip(keys, args))
    a.update({k: v for k, v in kw.items() if k in keys})
    name = ' '.join(x for x in (a['npc'], a['emotion'], a['view'], a['body']) if x)
    if a.get('eyes'):
        name += ' ' + a['view2'] + ' ' + a['eyes']
    if a.get('mouth'):
        name += ' ' + a['mouth']
    if a.get('blur'):
        name += ' ' + a['blur']
    base = f"{SPRITES_DIR}/{a['npc']}/{a['emotion']}/{a['view']}"
    layers = [find(names, f"{base}/1_body/{a['body']}.png")]
    if a.get('eyes'):
        e = a['eyes']
        first = {'ahead_to_aside': 'ahead', 'aside_to_ahead': 'aside', 'aside_to_dark': 'aside', 'ahead_to_dark': 'ahead',
                 'dark_to_aside': 'dark', 'dark_to_ahead': 'dark', 'ahead_shock': 'ahead', 'aside_shock': 'aside'}.get(e, e)
        layers.append(find(names, f"{base}/2_eyes/{a['view2']}/{first}/01.png"))
    if a.get('mouth'):
        layers.append(find(names, f"{base}/3_mouth/{a['mouth']}.png"))
    if a.get('drop'):
        layers.append(find(names, f"{base}/4_drop/01.png"))
    size = kw.get('size', (1920, 1080))
    eyes_size = kw.get('eyes_size', (1920, 1080))
    return {'name': name, 'who': a['npc'], 'emotion': a['emotion'], 'clothes': a['view'], 'pose': a['body'],
            'look': a.get('view2'), 'eyes': a.get('eyes'), 'mouth': a.get('mouth'), 'blur': bool(a.get('blur')),
            'size': list(size), 'eyes_dy': size[1] - eyes_size[1], 'layers': layers,
            'ok': all(layers)}


def main():
    names = archive_names()
    texts = {p.relative_to(SRC).as_posix(): p.read_text(encoding='utf-8', errors='replace') for p in SRC.rglob('*.rpy')}
    scenes = '\n'.join(t for k, t in texts.items() if k.startswith('scripts/scene/'))

    # characters
    chars = []
    for m in re.finditer(r"^define (\w+) = Character\((.*?)\)\s*$", texts['script.rpy'], re.M | re.S):
        var, body = m.group(1), m.group(2)
        nm = re.match(r"\s*(?:_\()?\s*(None|'[^']*'|\"[^\"]*\")", body)
        name = None if not nm or nm.group(1) == 'None' else nm.group(1)[1:-1]
        color = re.search(r'who_color\s*=\s*"(#[0-9A-Fa-f]{6})"', body)
        quote = ('bunny_quote' if 'bunny_quote' in body else 'bunny' if 'bunny' in body
                 else 'quote' if 'quote' in body else 'none')       # italic thoughts with / without quotes
        used = len(re.findall(r'^\s*' + var + r'\s+"', scenes, re.M))
        chars.append({'var': var, 'name': name, 'color': color.group(1) if color else None, 'quote': quote, 'lines': used})

    # sprites
    sprites = []
    seen = set()
    for k, t in texts.items():
        if not k.startswith('scripts/sprites/'):
            continue
        for args, kw in call_args(t, 'generate_img'):
            s = sprite(names, args, kw)
            if s['name'] in seen:
                continue
            seen.add(s['name'])
            sprites.append(s)
    shown = Counter()
    positions = Counter()
    for m in re.finditer(r'^\s*show\s+(.+?)(?:\s+at\s+(\w+))?(?:\s+zorder\s+\d+)?(?:\s+behind\s+\w+)?(?:\s+with\s+.+)?\s*$',
                         scenes, re.M):
        shown[m.group(1).strip()] += 1
        if m.group(2):
            positions[m.group(2)] += 1
    for s in sprites:
        s['shown'] = shown.get(s['name'], 0)
    sprites.sort(key=lambda s: (s['who'], -s['shown'], s['name']))

    # backgrounds: every image of every script that comes down to plain pictures (tb_images.py)
    import tb_images
    bgs = {}
    for name, v in tb_images.images(list(texts.values()), names).items():
        if 'color' in v:
            bgs[name] = {'color': v['color']}
        elif len(v['layers']) == 1 and v['layers'][0][:2] == [0, 0] and not v.get('size'):
            bgs[name] = {'file': v['layers'][0][2]}
        else:
            bgs[name] = {'layers': v['layers'], 'size': v.get('size') or [1920, 1080]}
    owners = {sp['who'] for sp in sprites}
    for k in [k for k in bgs if k.split(' ')[0] in owners]:
        del bgs[k]
    for b in bgs:
        bgs[b]['shown'] = len(re.findall(r'^\s*(?:scene|show)\s+' + re.escape(b) + r'(?:\s|$)', scenes, re.M))

    # positions: their definitions where simple
    pos = []
    alltext = '\n'.join(texts.values())
    for p, n in positions.most_common():
        m = re.search(r'^\s*transform\s+' + p + r'\b[^:]*:\s*\n((?:\s{8,}.*\n){1,8})', alltext, re.M)
        pos.append({'name': p, 'used': n, 'def': m.group(1).strip() if m else None})

    # audio
    music = sorted(n for n in names if n.startswith('music/') and n.endswith('.ogg'))
    sounds = defaultdict(list)
    for n in sorted(names):
        if n.startswith('sounds/') and n.endswith('.ogg'):
            parts = n.split('/')
            sounds[parts[1] if len(parts) > 2 else ''].append(n)
    voice = defaultdict(int)
    for n in names:
        if n.startswith('voice/'):
            voice[n.split('/')[1]] += 1
    channels = sorted(set(re.findall(r'^\s*play\s+(\w+)\s', scenes, re.M)))

    # the voiced lines: every `voice "file"` of the story with the line said under it (who, what) -
    # data/tb_voice.json, for finding a recorded line by its words
    lines_v, seen_v = [], set()
    say = re.compile(r'^\s*(?:(\w+)\s+)?"((?:[^"\\]|\\.)*)"\s*$')
    for k, t in texts.items():
        if not k.startswith('scripts/scene/'):
            continue
        rows = t.split('\n')
        for i, row in enumerate(rows):
            m = re.match(r'^\s*voice\s+"([^"]+)"', row)
            if not m:
                continue
            f = names.real(m.group(1))
            if not f or f in seen_v:
                continue
            for nxt in rows[i + 1:i + 6]:
                sm = say.match(nxt)
                if sm and sm.group(1) not in ('voice', 'play', 'show', 'scene', 'queue', 'with', 'jump', 'call'):
                    text = re.sub(r'\{[^}]*\}', '', sm.group(2)).replace('\\"', '"').strip()
                    if text:
                        seen_v.add(f)
                        lines_v.append([f, sm.group(1) or '', text])
                    break
    (OUT.parent / 'tb_voice.json').write_text(json.dumps(lines_v, ensure_ascii=False), encoding='utf-8')
    print(f'{len(lines_v)} voiced lines with their words -> tb_voice.json')

    cat = {'game': 'Tiny Bunny', 'version': (GAME.parent / 'log.txt').read_text(encoding='utf-8', errors='replace').split('\n')[5].strip(),
           'characters': chars, 'sprites': sprites, 'backgrounds': bgs, 'positions': pos, 'music': music,
           'sounds': dict(sounds), 'voice': dict(voice), 'channels': channels}
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(cat, ensure_ascii=False, indent=1), encoding='utf-8')
    bad = [s['name'] for s in sprites if not s['ok']]
    print(f"{len(chars)} characters, {len(sprites)} sprites ({len(bad)} with a layer missing), {len(bgs)} backgrounds, "
          f"{len(pos)} positions, {len(music)} music, {sum(len(v) for v in sounds.values())} sounds in {len(sounds)} folders, "
          f"voice {dict(voice)}, channels {channels} -> {OUT}")
    if bad:
        print('missing layers e.g.:', bad[:8])


if __name__ == '__main__':
    main()
