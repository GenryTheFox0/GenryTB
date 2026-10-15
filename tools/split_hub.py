# -*- coding: utf-8 -*-
"""The hub's raw strings (src/core/*.inc: one R"HUB(...)HUB" each) re-cut into pieces under 12 KB - MSVC cannot take a
string literal much over 16 KB (C2026); adjacent literals are joined by the compiler. Run before a build when an .inc grew."""
import glob
import os

for p in glob.glob(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'src', 'core', '*.inc')):
    s = open(p, encoding='utf-8').read()
    if 'R"HUB(' not in s:
        continue
    start = s.index('R"HUB(') + len('R"HUB(')
    end = s.rindex(')HUB"')
    body = s[start:end].replace(')HUB"\nR"HUB(', '')
    pieces, cur = [], ''
    for line in body.split('\n'):
        cand = cur + line + '\n'
        if len(cand.encode('utf-8')) > 12000 and cur:
            pieces.append(cur)
            cur = line + '\n'
        else:
            cur = cand
    pieces.append(cur[:-1] if cur.endswith('\n') else cur)
    new = s[:start] + ')HUB"\nR"HUB('.join(pieces) + s[end:]
    if new != s:
        open(p, 'w', encoding='utf-8').write(new)
    print(os.path.basename(p), len(pieces), [len(x.encode('utf-8')) for x in pieces])
