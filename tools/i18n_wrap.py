# -*- coding: utf-8 -*-
"""GenryTB i18n: the Russian words a person reads in the QML wrapped into qsTr("…") - in the bindings that show
text (text:, placeholderText:, hint:, title:, ToolTip.text:, the add bar's t:/h:/g:) and their ternaries and
joins; never a literal compared (=== "мысли") or a value the story writes (the chips' first element).
    python tools/i18n_wrap.py [--dry]      prints what it left (to be looked at by hand)"""
import glob
import os
import re
import sys

CYR = re.compile('[А-Яа-яЁё]')
LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')
KEYS = re.compile(r'^(\s*)(text|placeholderText|hint|title|ToolTip\.text|t|h|g|label|caption)\s*:\s*')
INLINE_KEYS = re.compile(r'\b(text|placeholderText|hint|title|ToolTip\.text|t|h|g|label|caption)\s*:\s*')

dry = '--dry' in sys.argv
left = []


def wrap_expr(expr):
    """every Cyrillic literal of a shown expression wrapped - unless compared"""
    out, pos = [], 0
    for m in LIT.finditer(expr):
        lit = m.group(0)
        out.append(expr[pos:m.start()])
        before = expr[:m.start()].rstrip()
        after = expr[m.end():].lstrip()
        compared = before.endswith(('===', '!==', '==', '!=')) or after.startswith(('===', '!==', '==', '!='))
        already = before.endswith('qsTr(')
        if CYR.search(m.group(1)) and not compared and not already:
            out.append('qsTr(' + lit + ')')
        else:
            out.append(lit)
        pos = m.end()
    out.append(expr[pos:])
    return ''.join(out)


for p in sorted(glob.glob(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'qml', '*.qml'))):
    lines = open(p, encoding='utf-8').read().split('\n')
    changed = False
    for i, line in enumerate(lines):
        if not CYR.search(line) or line.strip().startswith('//'):
            continue
        new = line
        # `key: expr` at the line's start, or `key: "…"` inside { … } on one line (several per line)
        m = KEYS.match(line)
        if m:
            head = line[:m.end()]
            body = line[m.end():]
            # the binding ends where the next `; key:` starts (one-liners like `Text { text: "x"; color: … }`)
            new = head + wrap_expr(body)
        else:
            # inline: { k: "say", t: "Реплика", h: "…" } / Text { text: "…"; … }
            def sub(mm):
                return mm.group(0)
            parts = []
            pos = 0
            for mm in INLINE_KEYS.finditer(line):
                parts.append(line[pos:mm.end()])
                # the value: up to the next ; or , that is outside quotes and brackets
                rest = line[mm.end():]
                depth, q, j = 0, False, 0
                while j < len(rest):
                    c = rest[j]
                    if c == '"' and (j == 0 or rest[j - 1] != '\\'):
                        q = not q
                    elif not q:
                        if c in '([{':
                            depth += 1
                        elif c in ')]}':
                            if depth == 0:
                                break
                            depth -= 1
                        elif c in ';,' and depth == 0:
                            break
                    j += 1
                parts.append(wrap_expr(rest[:j]))
                pos = mm.end() + j
            parts.append(line[pos:])
            new = ''.join(parts)
        if new != line:
            lines[i] = new
            changed = True
        # what is still bare
        for mm in LIT.finditer(lines[i]):
            if CYR.search(mm.group(1)) and not lines[i][:mm.start()].rstrip().endswith('qsTr('):
                left.append('%s:%d: %s' % (p.split('\\')[-1], i + 1, lines[i].strip()[:150]))
    if changed and not dry:
        open(p, 'w', encoding='utf-8').write('\n'.join(lines))

seen = set()
for l in left:
    if l not in seen:
        seen.add(l)
        print(l)
print('left', len(seen))
