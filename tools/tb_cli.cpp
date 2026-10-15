// GenryTB V1 command line: the core without the window - for checks and scripts.
//   tb_cli frame <story.txt> <line> <out.png>      the scene after that line, as the preview draws it
//   tb_cli sprite "<sprite name>" <out.png> [h]    one of the game's sprites (h: cut to the character)
//   tb_cli bg "<background>" <out.png>
//   tb_cli check <story.txt>                       the story's problems
//   tb_cli compile <story.txt> [out.rpy]           the Ren'Py script
//   tb_cli install <story.txt>                     into the game (game/mods/<id>/)
//   tb_cli run <story.txt> [scene]                 install + start the game in the scene (own saves)
#include "Builder.h"
#include "Catalog.h"
#include "Compiler.h"
#include "Renderer.h"
#include "Rpa.h"
#include "Story.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QTextStream>

using namespace tb;

static QTextStream& out()
{
    static QTextStream s(stdout);
    s.setEncoding(QStringConverter::Utf8);
    return s;
}

static QString readText(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    const QStringList a = app.arguments();
    if (a.size() < 3) {
        out() << "tb_cli frame|sprite|bg|check|compile|install|run ...\n";
        return 2;
    }
    Catalog cat;
    QString err;
    if (!cat.load(Catalog::findData(QStringLiteral("tb_catalog.json")), &err)) {
        out() << "catalog: " << err << "\n";
        return 1;
    }
    const QString game = Vfs::findGame(qEnvironmentVariable("TB_GAME"));
    Vfs vfs;
    const bool needGame = a[1] != QLatin1String("check") && a[1] != QLatin1String("compile");
    if (needGame && (game.isEmpty() || !vfs.mount(game, &err))) {
        out() << "game: " << (game.isEmpty() ? QStringLiteral("Tiny Bunny не найден") : err) << "\n";
        return 1;
    }
    Renderer r(vfs, cat);
    setHeadBox([&r](const QString& sprite) { return r.spriteHead(sprite); });
    const QString cmd = a[1];
    if (cmd == QLatin1String("sprite") && a.size() >= 4) {
        const QImage img = a.size() >= 5 ? r.spriteThumb(a[2], a[4].toInt()) : r.sprite(a[2]);
        if (img.isNull()) { out() << "нет спрайта " << a[2] << "\n"; return 1; }
        return img.save(a[3]) ? 0 : 1;
    }
    if (cmd == QLatin1String("bg") && a.size() >= 4) return r.background(a[2]).save(a[3]) ? 0 : 1;
    const Story st = parseStory(readText(a[2]), cat);
    if (cmd == QLatin1String("check")) {
        for (const Step& s : st.problems()) out() << "строка " << s.line << ": " << s.error << "\n";
        out() << st.steps.size() << " шагов, " << st.labels.size() << " сцен, проблем " << st.problems().size() << "\n";
        return st.problems().isEmpty() ? 0 : 1;
    }
    if (cmd == QLatin1String("frame") && a.size() >= 5) {
        const SceneState s = stateAt(st, cat, a[3].toInt());
        return r.frame(s).save(a[4]) ? 0 : 1;
    }
    if (cmd == QLatin1String("compile")) {
        const Compiled c = compileStory(st, cat);
        if (!c.ok) { out() << c.error << "\n"; return 1; }
        if (a.size() >= 4) {
            QFile f(a[3]);
            if (!f.open(QIODevice::WriteOnly)) return 1;
            f.write(c.rpy.toUtf8());
        } else {
            out() << c.rpy << "\n";
        }
        return 0;
    }
    if (cmd == QLatin1String("install") || cmd == QLatin1String("run")) {
        const InstallReport rep = install(game, st, cat, st.problems().isEmpty() ? r.frame(coverState(st, cat)) : QImage());
        if (!rep.ok) { out() << rep.error << "\n"; return 1; }
        out() << "установлен в " << rep.modDir << ", старт " << rep.startLabel << "\n";
        if (cmd == QLatin1String("run")) {
            const QString label = a.size() >= 4 ? labelName(st.id, a[3]) : rep.startLabel;
            const QString saves = QDir(Catalog::findData(QString())).filePath(QStringLiteral("../work/tb_saves"));
            const qint64 pid = runAt(game, label, QDir::cleanPath(saves), &err);
            if (!pid) { out() << err << "\n"; return 1; }
            out() << "игра запущена, pid " << pid << ", сцена " << label << "\n";
        }
        return 0;
    }
    out() << "непонятная команда\n";
    return 2;
}
