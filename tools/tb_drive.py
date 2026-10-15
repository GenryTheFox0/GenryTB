"""Drive Tiny Bunny's window in the game's own 1920x1080 coordinates (my test runs; um = universal-modder):
    python tb_drive.py shot out.png | move X Y | click X Y | key 0x0D | wait S   (any number, in order)
"""
import subprocess
import sys
import time


def um(*cmds):
    r = subprocess.run(['um', 'win', 'drive', '--proc', 'TinyBunny', *cmds], capture_output=True, text=True,
                       encoding='utf-8', errors='replace', shell=True)
    return r.stdout + r.stderr


def rect():
    out = um('rect')
    for line in out.splitlines():
        if line.startswith('rect ->'):
            p = line.split('->')[1].split()
            if len(p) == 4:
                return [int(x) for x in p]
    return None


def to_client(x, y):
    r = rect()
    if not r:
        raise SystemExit('no game window')
    _, _, cw, ch = r
    k = min(cw / 1920.0, ch / 1080.0)
    return int((cw - 1920 * k) / 2 + x * k), int((ch - 1080 * k) / 2 + y * k)


def main():
    a = sys.argv[1:]
    i = 0
    while i < len(a):
        c = a[i]
        if c == 'shot':
            subprocess.run(['um', 'win', 'shot', '--exe', 'TinyBunny.exe', a[i + 1], '--scale', '0.6'], capture_output=True, shell=True)
            i += 2
        elif c in ('move', 'click'):
            x, y = to_client(int(a[i + 1]), int(a[i + 2]))
            um('focus', f'{c} {x} {y}')
            i += 3
        elif c == 'key':
            um('focus', f'key {a[i + 1]}')
            i += 2
        elif c == 'wait':
            time.sleep(float(a[i + 1]))
            i += 2
        else:
            raise SystemExit('?' + c)


if __name__ == '__main__':
    main()
