"""Collect every string GenryTB shows, for data/i18n/<code>.json (GenryBL's way).

    python tools/i18n_extract.py            -> data/i18n/_source.json (the Russian originals, sorted)
                                              + what each language still misses

Sources: qsTr("…") in qml/*.qml, tbTr("…") in src/app, the camera's and the sprites' moves (Motion.cpp: their
title, row, what it looks like), and the names shown in the pickers (суефа's hands, places, times of day).
"""
import glob, io, json, os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CYR = re.compile(r'[А-Яа-яЁё]')
BS = chr(92)


def unq(s):
    return s.replace(BS + 'n', '\n').replace(BS + '"', '"').replace(BS + BS, BS)


def qml_strings():
    out = set()
    rx = re.compile(r'qsTr\("((?:[^"\\]|\\.)*)"\)')
    for f in glob.glob(os.path.join(ROOT, 'qml', '*.qml')):
        for m in rx.finditer(io.open(f, encoding='utf-8').read()):
            out.add(unq(m.group(1)))
    return out


def cpp_strings():
    out = set()
    rx = re.compile(r'tbTr\("((?:[^"\\]|\\.)*)"\)')
    for f in glob.glob(os.path.join(ROOT, 'src', '**', '*.cpp'), recursive=True):
        for m in rx.finditer(io.open(f, encoding='utf-8', errors='replace').read()):
            out.add(unq(m.group(1)))
    return out


def motion_strings():
    out = set()
    src = io.open(os.path.join(ROOT, 'src', 'core', 'Motion.cpp'), encoding='utf-8').read()
    for m in re.finditer(r'\b(?:cam|spr)\("[^"]*",\s*"[^"]*",\s*"((?:[^"\\]|\\.)*)",\s*"((?:[^"\\]|\\.)*)",\s*"((?:[^"\\]|\\.)*)"', src):
        for g in m.groups():
            if CYR.search(g):
                out.add(unq(g))
    return out


def picker_strings():
    """names shown translated but written into the story in Russian (qsTr(modelData[0]) in BlockCard.qml)"""
    out = set()
    src = io.open(os.path.join(ROOT, 'qml', 'BlockCard.qml'), encoding='utf-8').read()
    for name in ('hands', 'places', 'times'):
        m = re.search(r'readonly property var ' + name + r': \[(.*?)\]\n', src, re.S)
        if m:
            for w in re.findall(r'\["([^"]+)", "[^"]*"\]', m.group(1)):
                out.add(w)
    return out


def name_strings():
    """the characters' names the constructor shows (Catalog.cpp aliases, the catalog's speakers): shown translated,
    written into the story in Russian"""
    out = set()
    src = io.open(os.path.join(ROOT, 'src', 'core', 'Catalog.cpp'), encoding='utf-8').read()
    for m in re.finditer(r'\{"[A-Za-z_]+", "([^"]+)", "[^"]*"\}', src):
        out.add(m.group(1))
    cat = json.load(io.open(os.path.join(ROOT, 'data', 'tb_catalog.json'), encoding='utf-8'))
    for ch in cat.get('characters', []):
        if ch.get('name') and CYR.search(ch['name']) and '{' not in ch['name']:      # ({sc}Антон{/sc}: an effect, not a name)
            out.add(ch['name'])
    return out


if __name__ == '__main__':
    q, c, mo, pk, nm = qml_strings(), cpp_strings(), motion_strings(), picker_strings(), name_strings()
    src = sorted(q | c | mo | pk | nm)
    d = os.path.join(ROOT, 'data', 'i18n')
    os.makedirs(d, exist_ok=True)
    json.dump(src, io.open(os.path.join(d, '_source.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=0)
    print(f'qml {len(q)}  cpp {len(c)}  motion {len(mo)}  pickers {len(pk)}  -> {len(src)} strings, {sum(len(s) for s in src)} chars')
    for p in sorted(glob.glob(os.path.join(d, '*.json'))):
        name = os.path.basename(p)
        if name.startswith('_'):
            continue
        tr = json.load(io.open(p, encoding='utf-8'))
        miss = [s for s in src if s not in tr]
        print(f'  {name:12} {len(src) - len(miss)}/{len(src)}')
