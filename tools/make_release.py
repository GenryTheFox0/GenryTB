"""GenryTB - the release for people (GitHub): nothing of the developer's (no pdb, no tb_cli, no work files, no game
scripts), the Qt runtime cut to what GenryTB uses, the Visual C++ runtime app-local.

    python tools/make_release.py

1. builds (Release) and runs tb_selftest
2. stages dist/stage/GenryTB: app/ (GenryTB.exe + runtime), data/, projects/ (the sample mods), ПРОЧТИ.txt - and proves
   with dumpbin that every DLL any exe/dll of it needs is inside or a part of Windows itself
3. dist/github/: GenryTB_Setup.exe (the Win32 installer + the zipped program + footer), GenryTB_portable.zip, README.md
"""
import hashlib
import os
import shutil
import struct
import subprocess
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
QT = os.environ.get('GENRYTB_QT') or next((q for q in (r'C:\Qt\6.8.3\msvc2022_64', r'E:\Qt\6.8.3\msvc2022_64')
                                         if os.path.isdir(q)), r'C:\Qt\6.8.3\msvc2022_64')
BUILD = os.path.join(ROOT, 'build_release')      # the public edition: no TB_DEV (the developer's own things off)
DIST = os.path.join(ROOT, 'dist')
STAGE = os.path.join(DIST, 'stage', 'GenryTB')
OUT = os.path.join(DIST, 'github')
MAGIC = b'GENRYTB1'
SAMPLES = ['first_mod', 'alisa_night', 'motion_test', 'spasti_katyu']
# the Controls styles GenryTB never uses (main.cpp sets Basic): their QML and DLLs go
UNUSED_STYLES = ['Material', 'Universal', 'Imagine', 'Fusion', 'FluentWinUI3', 'Windows']


def vswhere():
    vw = os.path.expandvars(r'%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe')
    return subprocess.check_output([vw, '-latest', '-products', '*', '-requires',
                                    'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()


def vs_env():
    vcvars = os.path.join(vswhere(), r'VC\Auxiliary\Build\vcvars64.bat')
    out = subprocess.check_output(f'call "{vcvars}" >nul && set', shell=True, text=True, errors='replace')
    env = dict(l.split('=', 1) for l in out.splitlines() if '=' in l)
    env['PATH'] = env.get('PATH', env.get('Path', '')) + os.pathsep + os.path.join(os.environ.get('LOCALAPPDATA', ''), r'Programs\Python\Python310\Scripts')
    return env


def windows_has(name):
    n = name.lower()
    if n.startswith('api-ms-win-') or n.startswith('ext-ms-'):
        return True
    if n.startswith(('msvcp1', 'vcruntime1', 'concrt1', 'vccorlib1', 'mfc1', 'vcomp1')):
        return False                                    # the Visual C++ runtime is not a part of Windows
    return os.path.exists(os.path.join(os.environ.get('SystemRoot', r'C:\Windows'), 'System32', name))


def check_dependencies(app, env):
    inside = {f.lower() for f in os.listdir(app)}
    dumpbin = shutil.which('dumpbin', path=env.get('PATH') or env.get('Path') or '')
    if not dumpbin:
        sys.exit('dumpbin not found in the Visual Studio environment')
    missing, pes = {}, []
    for base, _, files in os.walk(app):
        pes += [os.path.join(base, f) for f in files if f.lower().endswith(('.exe', '.dll'))]
    for pe in pes:
        out = subprocess.run([dumpbin, '/nologo', '/dependents', pe], env=env, capture_output=True, text=True, errors='replace').stdout
        deps, on = [], False
        for line in out.splitlines():
            t = line.strip()
            if t.startswith('Image has the following dependencies'):
                on = True
                continue
            if on and t.lower().endswith('.dll'):
                deps.append(t)
            elif on and t.startswith('Summary'):
                break
        for d in deps:
            if d.lower() not in inside and not windows_has(d):
                missing.setdefault(d, []).append(os.path.relpath(pe, app))
    return len(pes), missing


def copy_tree(src, dst, skip=()):
    for base, _, files in os.walk(src):
        for f in files:
            if f in skip:
                continue
            s = os.path.join(base, f)
            d = os.path.join(dst, os.path.relpath(s, src))
            os.makedirs(os.path.dirname(d), exist_ok=True)
            shutil.copy2(s, d)


def sha(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def main():
    env = vs_env()
    print('== build (Release)')
    subprocess.check_call(f'cmake -S "{ROOT}" -B "{BUILD}" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="{QT}" -DTB_DEV=OFF >nul && '
                          f'cmake --build "{BUILD}"', shell=True, env=env)
    st_env = dict(env)
    st_env['PATH'] = os.path.join(QT, 'bin') + os.pathsep + st_env['PATH']
    r = subprocess.run([os.path.join(BUILD, 'tb_selftest.exe')], env=st_env, capture_output=True, text=True, errors='replace')
    last = (r.stdout.strip().splitlines() or ['?'])[-1]
    print('   ' + last)
    if r.returncode != 0 or ' 0 failed' not in last:
        sys.exit('selftest failed - no release')

    print('== stage dist/stage/GenryTB')
    shutil.rmtree(os.path.join(DIST, 'stage'), ignore_errors=True)
    app = os.path.join(STAGE, 'app')
    os.makedirs(app)
    shutil.copy2(os.path.join(BUILD, 'GenryTB.exe'), app)
    subprocess.check_call([os.path.join(QT, 'bin', 'windeployqt.exe'), '--release', '--qmldir', os.path.join(ROOT, 'qml'), '--no-translations',
                           '--no-system-d3d-compiler', '--no-opengl-sw', '--skip-plugin-types', 'qmltooling,generic',
                           os.path.join(app, 'GenryTB.exe')], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env)
    cut = 0
    for style in UNUSED_STYLES:
        d = os.path.join(app, 'qml', 'QtQuick', 'Controls', style)
        if os.path.isdir(d):
            shutil.rmtree(d)
            cut += 1
        for f in os.listdir(app):
            if f.startswith('Qt6QuickControls2' + style) and f.endswith('.dll'):
                os.remove(os.path.join(app, f))
                cut += 1
    print(f'   unused Controls styles cut: {cut} pieces')
    redist = env.get('VCToolsRedistDir', '')
    crt = []
    for base, _, files in os.walk(os.path.join(redist, 'x64')):
        if os.path.basename(base).lower().startswith('microsoft.vc') and base.lower().endswith('.crt'):
            crt += [os.path.join(base, f) for f in files if f.lower().endswith('.dll')]
    if not crt:
        sys.exit('the Visual C++ runtime DLLs were not found in ' + redist)
    for f in crt:
        shutil.copy2(f, app)
    print('   VC++ runtime:', ', '.join(sorted(os.path.basename(f) for f in crt)))
    n, missing = check_dependencies(app, env)
    if missing:
        for d, who in sorted(missing.items()):
            print(f'   MISSING {d}  <- {", ".join(who[:4])}')
        sys.exit('the program would not start on a clean Windows - no release')
    print(f'   dependencies: {n} exe/dll checked, all inside or a part of Windows')

    data = os.path.join(STAGE, 'data')
    os.makedirs(data)
    for f in ('tb_catalog.json', 'tb_voice.json', 'starter_story.txt', 'example_alisa.txt', 'genrytb.ico'):
        shutil.copy2(os.path.join(ROOT, 'data', f), data)
    copy_tree(os.path.join(ROOT, 'data', 'fonts'), os.path.join(data, 'fonts'))
    copy_tree(os.path.join(ROOT, 'data', 'assets'), os.path.join(data, 'assets'), skip=('preview.png', 'sheet.jpg'))
    copy_tree(os.path.join(ROOT, 'data', 'flags'), os.path.join(data, 'flags'))
    copy_tree(os.path.join(ROOT, 'data', 'i18n'), os.path.join(data, 'i18n'), skip=('_source.json',))
    langs = [f for f in os.listdir(os.path.join(data, 'i18n')) if f.endswith('.json')]
    if len(langs) < 19:
        sys.exit(f'only {len(langs)} translations in data/i18n - no release')
    for s in SAMPLES:
        src = os.path.join(ROOT, 'projects', s, 'story.txt')
        if os.path.isfile(src):
            os.makedirs(os.path.join(STAGE, 'projects', s), exist_ok=True)
            shutil.copy2(src, os.path.join(STAGE, 'projects', s))
    shutil.copy2(os.path.join(ROOT, 'release_files', 'ПРОЧТИ.txt'), STAGE)

    print('== pack')
    os.makedirs(OUT, exist_ok=True)
    shutil.rmtree(OUT, ignore_errors=True)
    os.makedirs(OUT, exist_ok=True)
    payload = os.path.join(DIST, 'payload.zip')
    with zipfile.ZipFile(payload, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for base, _, files in os.walk(STAGE):
            for f in sorted(files):
                p = os.path.join(base, f)
                z.write(p, os.path.join('GenryTB', os.path.relpath(p, STAGE)))
    setup = os.path.join(OUT, 'GenryTB_Setup.exe')
    with open(setup, 'wb') as out:
        with open(os.path.join(BUILD, 'GenryTB_Setup.exe'), 'rb') as f:
            out.write(f.read())
        size = os.path.getsize(payload)
        with open(payload, 'rb') as f:
            shutil.copyfileobj(f, out)
        out.write(struct.pack('<Q', size) + MAGIC)
    shutil.move(payload, os.path.join(OUT, 'GenryTB_portable.zip'))
    shutil.copy2(os.path.join(ROOT, 'README.md'), OUT)
    copy_tree(os.path.join(ROOT, 'docs', 'screens'), os.path.join(OUT, 'docs', 'screens'))     # the README's pictures
    print('== ' + os.path.relpath(OUT, ROOT).replace(os.sep, '/'))
    for f in sorted(os.listdir(OUT)):
        p = os.path.join(OUT, f)
        if os.path.isfile(p):
            print(f'   {f:28} {os.path.getsize(p) / 1048576:8.1f} MB  sha256 {sha(p)[:16]}')


if __name__ == '__main__':
    main()
