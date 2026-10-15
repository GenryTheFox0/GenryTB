# -*- coding: utf-8 -*-
"""GenryTB: hand sets for камень-ножницы-бумага - the game's own opponent hands (the left ones, Romka's: R_01 scissors,
R_02 paper, R_03 rock)
redrawn by Qwen-Image-Edit 2511 for each character (the sleeve, the hand itself), the gesture and the game's pencil
look kept. Run on the ComfyUI of GENRY Нейроарт (127.0.0.1:8188):
    python tools/gen_hands.py [set ...]        (no names: every set; a set already made is skipped, --force redoes)
Out: data/assets/hands/<set>/R_01..03.png (1920x1080 RGBA, as the game's) + data/assets/hands/<set>/preview.png"""
import io, json, os, sys, time, urllib.request, uuid

from PIL import Image, ImageFilter

# graphs.py - the ComfyUI graphs (Qwen-Image-Edit 2511 + Lightning) of GENRY Нейро Lite; its folder in GENRY_NEURO_LITE
sys.path.insert(0, os.environ.get('GENRY_NEURO_LITE', ''))
import graphs  # noqa: E402

COMFY = 'http://127.0.0.1:8188'
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'work', 'suefa', 'images', 'interface', 'suefa')   # the game's own hands, out of its archive
OUT = os.path.join(ROOT, 'data', 'assets', 'hands')
BOX = (0, 190, 1060, 770)            # where the opponent's (left) hand is on the 1920x1080 screen (all three poses inside)
GREEN = (0, 255, 0)

KEEP = ('Keep exactly the same hand gesture, the same fingers, the same position, size and angle, the arm coming '
        'from the left edge. Keep the same grayscale pencil and charcoal sketch drawing style with dark ink outlines. '
        'Keep the plain flat bright green background.')
FEM = ('A clearly feminine hand: slender and delicate, long thin fingers, a narrow wrist, soft smooth skin, neat oval '
       'nails.')
MASC = ('A clearly masculine hand: broad and strong, thick fingers, big knuckles, visible veins, a hairy wrist, short '
        'square nails.')
SETS = {
    'katya':  ('Катя', "Make it a slender teenage girl's hand with thin fingers and neat short nails, in the long sleeve of a dark "
                       "school uniform dress with a white lace cuff."),
    'polina': ('Полина', FEM + " A teenage girl's hand with a thin hair tie on the wrist, in the long sleeve of a thick "
                               "hand-knitted wool sweater with a ribbed cuff."),
    'olya':   ('Оля', "Make it a small young girl's hand, tiny short fingers, in the sleeve of a puffy winter jacket with a "
                      "fur-trimmed cuff."),
    'byasha': ('Бяша', "Make it a boy's rough hand with scratched knuckles and dirty fingernails, in the sleeve of a worn quilted "
                       "padded jacket."),
    'mama':   ('Мама', FEM + " A grown woman's hand with a thin gold wedding ring and light nail polish, in the sleeve of a "
                             "fluffy plain terry-cloth bathrobe with a wide soft cuff, no pattern."),
    'papa':   ('Папа', MASC + " A grown man's hand with a wristwatch on a metal bracelet, in the rolled-up sleeve of a "
                              "checkered flannel shirt."),
    'devushka': ('Девушка', FEM + " A young woman's elegant hand with dark nail polish and thin bracelets, in the sleeve of a "
                                  "fitted black turtleneck."),
    'muzhchina': ('Мужчина', MASC + " A grown man's hand with a small tattoo on the back of the hand, in the sleeve of a black "
                                    "leather jacket."),
    'babushka': ('Бабушка', "Make it an old woman's thin wrinkled hand with age spots and swollen knuckles, in the sleeve of a "
                            "knitted cardigan."),
    'alisa':  ('Алиса', FEM + " A teenage girl's hand with a bracelet of small beads, in the sleeve of a light knitted cardigan."),
    'tikhonov': ('Тихонов', MASC + " A grown man's hand, in the sleeve of a grey Soviet militia uniform greatcoat with a "
                                  "buttoned cuff."),
    'semyon': ('Семён', "Make it a skinny boy's hand with bony knuckles and bitten nails, in the sleeve of a dark school jacket."),
    'uchitelnica': ('Учительница', FEM + " A grown woman's hand with chalk dust on the fingers and a thin ring, in the sleeve "
                                         "of a strict grey suit jacket."),
    'hariton': ('Харитон', "Make it an old man's big gnarled hand, rough deeply wrinkled skin, thick yellowed nails, in the "
                           "sleeve of a worn sheepskin coat."),
    'dedmoroz': ('Дед Мороз', MASC + " In the sleeve of a festive velvet coat with a thick white fur cuff."),
    'vrach':  ('Врач', "Make it a clean adult hand with short neat nails, in the sleeve of a white medical coat."),
    'killer': ('Киллер', "Make it a hand in a tight black leather glove, in the sleeve of a long dark coat."),
    'lisa':   ('Лиса', "Make it a fox's paw shaped like a hand making the same gesture: rusty fur, small dark claws, the arm "
                       "covered in fox fur."),
    'volchik': ('Волчик', "Make it a wolf's paw shaped like a hand making the same gesture: thick grey fur, dark claws, the arm "
                          "covered in shaggy wolf fur."),
    'medvezhutka': ('Медвежутка', "Make it a bear's big paw shaped like a hand making the same gesture: dense brown fur, "
                                  "thick claws, the arm covered in bear fur."),
    'zaichik': ('Зайчик', "Make it a monstrous black furry paw with long sharp curved claws instead of fingernails, the arm "
                          "covered in dark matted fur."),
    'skelet': ('Скелет', "Make it a bony skeleton hand, bare bones and joints, in the torn sleeve of a rotten old coat."),
}


def post(path, data, ctype):
    req = urllib.request.Request(COMFY + path, data=data, headers={'Content-Type': ctype})
    return json.loads(urllib.request.urlopen(req, timeout=60).read())


def upload(img, name):
    b = io.BytesIO()
    img.save(b, 'PNG')
    bnd = uuid.uuid4().hex
    body = (('--%s\r\nContent-Disposition: form-data; name="image"; filename="%s"\r\nContent-Type: image/png\r\n\r\n' % (bnd, name))
            .encode() + b.getvalue() + ('\r\n--%s\r\nContent-Disposition: form-data; name="overwrite"\r\n\r\ntrue\r\n--%s--\r\n'
                                        % (bnd, bnd)).encode())
    return post('/upload/image', body, 'multipart/form-data; boundary=' + bnd)['name']


def run(nodes):
    pid = post('/prompt', json.dumps({'prompt': nodes, 'client_id': 'genry_tb_hands'}).encode(), 'application/json')['prompt_id']
    while True:
        time.sleep(2)
        h = json.loads(urllib.request.urlopen(COMFY + '/history/' + pid, timeout=30).read())
        if pid in h:
            st = h[pid].get('status', {})
            if st.get('status_str') == 'error':
                raise RuntimeError(json.dumps(st)[:800])
            for out in h[pid]['outputs'].values():
                for im in out.get('images', []):
                    q = 'filename=%s&subfolder=%s&type=%s' % (im['filename'], im['subfolder'], im['type'])
                    return Image.open(io.BytesIO(urllib.request.urlopen(COMFY + '/view?' + q, timeout=60).read())).convert('RGB')
            raise RuntimeError('no image')


def key_green(img):
    """the flat green back off: alpha from how green a pixel is; the art is grayscale, so the spill goes with the colour"""
    out = Image.new('RGBA', img.size)
    src = img.load()
    dst = out.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            r, g, b = src[x, y]
            green = g - max(r, b)
            a = 255 if green < 40 else 0 if green > 120 else int(255 * (120 - green) / 80)
            lum = int(0.5 * r + 0.5 * b) if green > 10 else int(0.299 * r + 0.587 * g + 0.114 * b)
            dst[x, y] = (lum, lum, lum, a)
    # the 1-2 px green fringe: the alpha shrunk a hair and softened
    a = out.getchannel('A').filter(ImageFilter.MinFilter(3)).filter(ImageFilter.GaussianBlur(0.7))
    out.putalpha(a)
    return out


POSE = {1: 'The gesture is SCISSORS: only the index and middle fingers stretched out in a V, the other fingers folded.',
        2: 'The gesture is PAPER: the open flat palm, all five fingers straight and held together, nothing pointing.',
        3: 'The gesture is ROCK: a tight closed fist, all fingers curled in.'}


def make(set_id, force=False, only=None, seed=0):
    who, text = SETS[set_id]
    dst = os.path.join(OUT, set_id)
    os.makedirs(dst, exist_ok=True)
    for n in (1, 2, 3):
        fn = os.path.join(dst, 'R_0%d.png' % n)
        if only and n not in only:
            continue
        if os.path.exists(fn) and not force and not only:
            continue
        hand = Image.open(os.path.join(SRC, 'R_0%d.png' % n)).convert('RGBA').crop(BOX)
        base = Image.new('RGBA', hand.size, GREEN + (255,))
        base.alpha_composite(hand)
        name = upload(base.convert('RGB'), 'genry_tb_hand_R0%d.png' % n)
        instr = (text if text.startswith('Make') else 'Make it this hand. ' + text) + ' ' + POSE[n] + ' ' + KEEP
        nodes, _ = graphs.edit({'instruction': instr}, name, 1000 + n + seed)   # one seed per pose: the same sleeve
        t = time.time()
        res = run(nodes).resize(hand.size, Image.LANCZOS)
        cut = key_green(res)
        full = Image.new('RGBA', (1920, 1080), (0, 0, 0, 0))
        full.paste(cut, BOX[:2])
        full.save(fn)
        print('%s R_0%d  %.0f s' % (set_id, n, time.time() - t), flush=True)
    # a preview: the three poses on the game's own field
    fon = Image.open(os.path.join(SRC, 'fon.png')).convert('RGBA')
    sheet = Image.new('RGB', (3 * 640, 360))
    for i, n in enumerate((3, 1, 2)):
        f = fon.copy()
        f.alpha_composite(Image.open(os.path.join(dst, 'R_0%d.png' % n)))
        f.alpha_composite(Image.open(os.path.join(SRC, 'A_0%d.png' % n)).convert('RGBA'))
        sheet.paste(f.convert('RGB').resize((640, 360), Image.LANCZOS), (i * 640, 0))
    sheet.save(os.path.join(dst, 'preview.png'))


if __name__ == '__main__':
    # names; "set:2" - only that pose (1 scissors, 2 paper, 3 rock) again; --seed=N another try
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    seed = next((int(a[7:]) for a in sys.argv if a.startswith('--seed=')), 0)
    for s in args or list(SETS):
        name, _, poses = s.partition(':')
        make(name, '--force' in sys.argv, [int(c) for c in poses] if poses else None, seed)
