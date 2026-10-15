"""The game's images that a mod can use as a scene (make_tb_catalog.py): every `image` statement of every
script that comes down to plain pictures -

    image name = "file.jpg"                      one picture (or "#colour")
    image name = LiveComposite((w, h), (x, y), "file or image", ...)   layers, each where it goes
    image name:                                  an ATL block that opens on one picture
        "file.jpg"
    image name:                                  contains: blocks - each the picture it opens on
        contains:
            "a.jpg"
        contains:
            "b.png"

A layer may name another image ("bg school_corner_day"): it is that image's layers, wherever it is
defined (resolved over passes). Anything else (animations, transforms of displayables) is left out.

    images(texts, names) -> {name: {'layers': [[x, y, path], ...], 'size': [w, h]} | {'color': '#rrggbb'}}
"""
import re

STMT = re.compile(r'^(?P<ind>[ \t]*)image\s+(?P<name>[\w ]+?)\s*(?P<op>=|:)', re.M)


def call_body(t, start):
    """the text inside the parentheses that open just before start"""
    i, depth = start, 1
    while i < len(t) and depth:
        depth += {'(': 1, ')': -1}.get(t[i], 0)
        i += 1
    return t[start:i - 1]


def block(t, start, ind):
    """the lines of an indented block after a 'name:' line"""
    lines = []
    for line in t[start:].split('\n')[1:]:
        if line.strip() == '':
            lines.append(line)
            continue
        if len(line) - len(line.lstrip()) <= len(ind):
            break
        lines.append(line)
    return lines


def raw_images(texts):
    """name -> ('file', str) | ('composite', (w, h), [(x, y, ref)]) | ('contains', [ref, ...])"""
    out = {}
    for t in texts:
        for m in STMT.finditer(t):
            name, op, ind = m.group('name').strip(), m.group('op'), m.group('ind')
            rest = t[m.end():]
            if op == '=':
                s = re.match(r'\s*"([^"]+)"\s*$', rest.split('\n')[0])
                if s:
                    out.setdefault(name, ('file', s.group(1)))
                    continue
                c = re.match(r'\s*(?:LiveComposite|im\.Composite|Composite)\(', rest)
                if c:
                    body = call_body(t, m.end() + c.end())
                    size = re.match(r'\s*\((\d+)\s*,\s*(\d+)\)', body)
                    parts = re.findall(r'\(\s*(-?\d+)\s*,\s*(-?\d+)\s*\)\s*,\s*"([^"]+)"', body)
                    tuples = len(re.findall(r'\(\s*-?\d+\s*,\s*-?\d+\s*\)\s*,', body)) - 1
                    if size and parts and len(parts) == tuples:
                        out.setdefault(name, ('composite', (int(size.group(1)), int(size.group(2))),
                                              [(int(x), int(y), f) for x, y, f in parts]))
                continue
            lines = [l.strip() for l in block(t, m.start(), ind) if l.strip()]
            if not lines:
                continue
            s = re.match(r'^"([^"]+)"$', lines[0])
            if s:
                out.setdefault(name, ('file', s.group(1)))
                continue
            if lines[0] == 'contains:':
                refs, i = [], 0
                while i < len(lines):
                    if lines[i] == 'contains:' and i + 1 < len(lines):
                        r = re.match(r'^"([^"]+)"$', lines[i + 1])
                        if not r:
                            refs = None
                            break
                        refs.append(r.group(1))
                    i += 1
                if refs:
                    out.setdefault(name, ('contains', refs))
    return out


def images(texts, names):
    def find(path):
        for p in (path, 'images/' + path):
            r = names.real(p) if hasattr(names, 'real') else (p if p in names else None)
            if r:
                return r
        return None

    raw = raw_images(texts)
    # every image name any script defines, of any kind: a layer naming one that is no picture (snow,
    # an animation) is left out of the still - the rest of it is the scene
    defined = set()
    for t in texts:
        for m in STMT.finditer(t):
            defined.add(m.group('name').strip())
    SKIP = []
    done = {'black': {'color': '#000000'}, 'white': {'color': '#ffffff'}}

    def layers_of(ref):
        """a layer's reference -> [[x, y, path]] (a picture, or an image already resolved) | None"""
        if ref.startswith('#'):
            return None
        p = find(ref)
        if p:
            return [[0, 0, p]]
        r = done.get(ref)
        if r and 'layers' in r:
            return r['layers']
        if ref in defined and ref not in raw:
            return SKIP                              # (an animation and the like: not in the still)
        return None

    for _ in range(6):                               # passes: images naming images
        grew = False
        for name, r in raw.items():
            if name in done:
                continue
            res = None
            if r[0] == 'file':
                if r[1].startswith('#'):
                    res = {'color': r[1][:7]}
                else:
                    p = find(r[1])
                    if p:
                        res = {'layers': [[0, 0, p]], 'size': None}
                    elif r[1] in done:
                        res = done[r[1]]
            elif r[0] == 'composite':
                ls = []
                for x, y, ref in r[2]:
                    sub = layers_of(ref)
                    if sub is None:
                        ls = None
                        break
                    ls += [[x + a, y + b, p] for a, b, p in sub]
                if ls:
                    res = {'layers': ls, 'size': list(r[1])}
            elif r[0] == 'contains':
                ls = []
                for ref in r[1]:
                    sub = layers_of(ref)
                    if sub is None:
                        ls = None
                        break
                    ls += sub
                if ls:
                    res = {'layers': ls, 'size': None}
            if res:
                done[name] = res
                grew = True
        if not grew:
            break
    return done
