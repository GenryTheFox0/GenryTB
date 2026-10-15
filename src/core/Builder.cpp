#include "Builder.h"
#include "Catalog.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace tb {
namespace {

bool writeUtf8(const QString& path, const QString& text, QString* err)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (err) *err = QStringLiteral("не записать %1").arg(path);
        return false;
    }
    f.write(text.toUtf8());
    return true;
}

// the game gets this computer's environment minus the Qt settings of whoever started it (tb_cli draws offscreen)
QProcessEnvironment gameEnvironment()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const QString& k : env.keys())
        if (k.startsWith(QLatin1String("QT_"), Qt::CaseInsensitive) || k.startsWith(QLatin1String("QML"), Qt::CaseInsensitive))
            env.remove(k);
    return env;
}

QString modsDir(const QString& gameRoot) { return QDir(gameRoot).filePath(QStringLiteral("game/mods")); }
QString hookDir(const QString& gameRoot) { return QDir(gameRoot).filePath(QStringLiteral("game/mods/_genry_tb_preview")); }

bool writeHub(const QString& gameRoot, QString* err)
{
    installFonts(gameRoot);
    const QString path = QDir(modsDir(gameRoot)).filePath(QStringLiteral("genry_tb_hub.rpy"));
    QFile f(path);
    if (f.open(QIODevice::ReadOnly) && QString::fromUtf8(f.readAll()) == hubScript()) return true;
    f.close();
    QFile::remove(path + QLatin1Char('c'));
    return writeUtf8(path, hubScript(), err);
}

} // namespace

int installFonts(const QString& gameRoot)
{
    const QString data = QFileInfo(Catalog::findData(QStringLiteral("tb_catalog.json"))).absolutePath();
    const QDir src(QDir(data).filePath(QStringLiteral("fonts")));
    if (!src.exists() || gameRoot.isEmpty()) return -1;
    int copied = 0;
    auto copyDir = [&](const QString& from, const QString& to) {
        const QDir in(src.filePath(from));
        if (!in.exists()) return;
        QDir().mkpath(to);
        for (const QFileInfo& f : in.entryInfoList(QDir::Files)) {
            const QString dst = QDir(to).filePath(f.fileName());
            const QFileInfo have(dst);
            if (have.exists() && have.size() == f.size()) continue;
            QFile::remove(dst);
            if (QFile::copy(f.absoluteFilePath(), dst)) ++copied;
        }
    };
    copyDir(QStringLiteral("game"), QDir(gameRoot).filePath(QStringLiteral("game")));
    const QString own = QDir(gameRoot).filePath(QStringLiteral("game/mods/genry_fonts"));
    copyDir(QStringLiteral("mods"), own);
    copyDir(QStringLiteral("intl"), own);
    // (the constructor's pictures: the peephole's mask...) - data/assets
    {
        const QString assets = QDir(data).filePath(QStringLiteral("assets"));
        const QString to = QDir(gameRoot).filePath(QStringLiteral("game/mods/genry_assets"));
        if (QDir(assets).exists()) {
            QDir().mkpath(to);
            QDirIterator walk(assets, QDir::Files, QDirIterator::Subdirectories);
            while (walk.hasNext()) {
                const QFileInfo f(walk.next());
                if (f.fileName() == QLatin1String("preview.png") || f.fileName() == QLatin1String("sheet.jpg")) continue;
                const QString rel = QDir(assets).relativeFilePath(f.absoluteFilePath());
                const QString dst = QDir(to).filePath(rel);
                if (QFileInfo(dst).exists() && QFileInfo(dst).size() == f.size()) continue;
                QDir().mkpath(QFileInfo(dst).absolutePath());
                QFile::remove(dst);
                if (QFile::copy(f.absoluteFilePath(), dst)) ++copied;
            }
        }
    }
    if (QFileInfo(src.filePath(QStringLiteral("OFL.txt"))).exists() && !QFileInfo(QDir(own).filePath(QStringLiteral("OFL.txt"))).exists())
        QFile::copy(src.filePath(QStringLiteral("OFL.txt")), QDir(own).filePath(QStringLiteral("OFL.txt")));
    return copied;
}

InstallReport install(const QString& gameRoot, const Story& story, const Catalog& cat, const QImage& cover)
{
    InstallReport rep;
    const Compiled c = compileStory(story, cat);
    if (!c.ok) {
        rep.error = c.error;
        return rep;
    }
    QString err;
    if (!writeHub(gameRoot, &err)) {
        rep.error = err;
        return rep;
    }
    static const QRegularExpression okId(QStringLiteral("^[A-Za-z][A-Za-z0-9_]{0,40}$"));
    if (!okId.match(story.id).hasMatch()) {
        rep.error = QStringLiteral("@mod_id латиницей, с буквы: %1").arg(story.id);
        return rep;
    }
    rep.modDir = QDir(modsDir(gameRoot)).filePath(story.id);
    QDir(rep.modDir).removeRecursively();                // a scene renamed or gone is gone from the game too
    if (!writeUtf8(QDir(rep.modDir).filePath(story.id + QStringLiteral(".rpy")), c.rpy, &err)) {
        rep.error = err;
        return rep;
    }
    if (!cover.isNull())
        cover.scaled(960, 540, Qt::KeepAspectRatio, Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_RGB32)
            .save(QDir(rep.modDir).filePath(QStringLiteral("cover.jpg")), "JPG", 88);
    rep.startLabel = c.startLabel;
    rep.labels = c.labels;
    rep.ok = true;
    return rep;
}

bool uninstall(const QString& gameRoot, const QString& modId, QString* err)
{
    const QString d = QDir(modsDir(gameRoot)).filePath(modId);
    if (!QFileInfo(d).isDir()) return true;
    if (!QDir(d).removeRecursively()) {
        if (err) *err = QStringLiteral("не удалить %1 (игра запущена?)").arg(d);
        return false;
    }
    return true;
}

QStringList installedMods(const QString& gameRoot)
{
    QStringList out;
    for (const QString& d : QDir(modsDir(gameRoot)).entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        if (!d.startsWith(QLatin1Char('_')) && QFileInfo::exists(QDir(modsDir(gameRoot)).filePath(d + QLatin1Char('/') + d + QStringLiteral(".rpy"))))
            out << d;
    return out;
}

void removeHook(const QString& gameRoot)
{
    QDir(hookDir(gameRoot)).removeRecursively();
}

qint64 runAt(const QString& gameRoot, const QString& label, const QString& saveDir, QString* err)
{
    removeHook(gameRoot);
    // label_overrides instead of redefining the game's labels: no "label defined twice"; a scene that is not
    // there starts the game as usual rather than an exception
    const QString hook = QStringLiteral(
        "# -*- coding: utf-8 -*-\n"
        "# GenryTB V1 test run hook. Temporary: removed when the game closes.\n\n"
        "init 999 python:\n"
        "    config.label_overrides['splashscreen'] = '_genry_tb_autostart'\n"
        "    config.label_overrides['main_menu'] = '_genry_tb_autostart'\n\n"
        "label _genry_tb_autostart:\n"
        "    python:\n"
        "        config.label_overrides.pop('main_menu', None)\n"
        "        config.label_overrides.pop('splashscreen', None)\n"
        "        _preferences.fullscreen = False\n"       // (a test run in a window; its saves and settings are its own)
        "    if renpy.has_label('%1'):\n"
        "        jump %1\n"
        "    return\n").arg(label);
    QString e;
    if (!writeUtf8(QDir(hookDir(gameRoot)).filePath(QStringLiteral("_genry_tb_preview.rpy")), hook, &e)) {
        if (err) *err = e;
        return 0;
    }
    QDir().mkpath(saveDir);
    qint64 pid = 0;
    QProcess game;
    game.setProgram(QDir(gameRoot).filePath(QStringLiteral("TinyBunny.exe")));
    game.setArguments({QStringLiteral("--savedir"), QDir::toNativeSeparators(saveDir)});
    game.setWorkingDirectory(gameRoot);
    game.setProcessEnvironment(gameEnvironment());
    if (!game.startDetached(&pid)) {
        removeHook(gameRoot);
        if (err) *err = QStringLiteral("TinyBunny.exe не запустился");
        return 0;
    }
    return pid;
}

bool isRunning(qint64 pid)
{
#ifdef Q_OS_WIN
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, DWORD(pid));
    if (!h) return false;
    const DWORD r = WaitForSingleObject(h, 0);
    CloseHandle(h);
    return r == WAIT_TIMEOUT;
#else
    Q_UNUSED(pid);
    return false;
#endif
}

} // namespace tb
