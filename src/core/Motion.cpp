#include "Motion.h"

#include <QMutex>
#include <QStringList>
#include <QtMath>
#include <cmath>

namespace tb {
namespace {

QString lower(const QString& s)
{
    QString t = s.trimmed().toLower();
    t.replace(QChar(0x0451), QChar(0x0435));       // ё -> е
    return t;
}

MotionDef cam(const char* id, const char* word, const char* title, const char* group, const char* hint, bool point,
              bool once, const char* atl)
{
    MotionDef m;
    m.id = QString::fromUtf8(id);
    m.word = QString::fromUtf8(word);
    m.title = QString::fromUtf8(title);
    m.group = QString::fromUtf8(group);
    m.hint = QString::fromUtf8(hint);
    m.camera = true;
    m.point = point;
    m.once = once;
    m.atl = QString::fromUtf8(atl);
    return m;
}

MotionDef spr(const char* id, const char* word, const char* title, const char* group, const char* hint, bool once,
              const char* atl)
{
    MotionDef m = cam(id, word, title, group, hint, false, once, atl);
    m.camera = false;
    return m;
}

// ---- the ATL the table is written in: read once into a tree, walked for a moment of time ----

struct Node {
    enum Kind { Set, Interp, Pause, Repeat, Block, Parallel } kind = Set;
    char warp = 'l';                             // l linear, e ease, i easein, o easeout
    double dur = 0;
    QVector<QPair<QString, QString>> props;      // name, value (a number, or ax / ay)
    QVector<Node> kids;                          // Block: its statements; Parallel: its branches (Blocks)
};

struct Line {
    int indent = 0;
    QString text;
};

QVector<QPair<QString, QString>> readProps(const QStringList& w, int from)
{
    QVector<QPair<QString, QString>> out;
    for (int i = from; i < w.size(); ++i) {
        const QString name = w[i];
        if (name == QLatin1String("align") && i + 2 < w.size()) {     // align (ax, ay)
            QString a = w[i + 1], b = w[i + 2];
            a.remove(QLatin1Char('(')).remove(QLatin1Char(','));
            b.remove(QLatin1Char(')'));
            out.push_back({QStringLiteral("xalign"), a});
            out.push_back({QStringLiteral("yalign"), b});
            i += 2;
        } else if (i + 1 < w.size()) {
            out.push_back({name, w[i + 1]});
            ++i;
        }
    }
    return out;
}

QVector<Node> readBlock(const QVector<Line>& lines, int& i, int indent)
{
    QVector<Node> out;
    while (i < lines.size() && lines[i].indent >= indent) {
        const QString t = lines[i].text;
        const QStringList w = t.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        ++i;
        Node n;
        if (t == QLatin1String("block:") || t == QLatin1String("parallel:")) {
            Node b;
            b.kind = Node::Block;
            const int inner = i < lines.size() ? lines[i].indent : indent + 4;
            if (inner > indent) b.kids = readBlock(lines, i, inner);
            if (t == QLatin1String("block:")) {
                out.push_back(b);
            } else if (!out.isEmpty() && out.last().kind == Node::Parallel) {
                out.last().kids.push_back(b);            // parallel blocks in a row run side by side
            } else {
                n.kind = Node::Parallel;
                n.kids.push_back(b);
                out.push_back(n);
            }
            continue;
        }
        if (w[0] == QLatin1String("repeat")) {
            n.kind = Node::Repeat;
        } else if (w[0] == QLatin1String("pause")) {
            n.kind = Node::Pause;
            n.dur = w.value(1).toDouble();
        } else if (w[0] == QLatin1String("linear") || w[0] == QLatin1String("ease") || w[0] == QLatin1String("easein") ||
                   w[0] == QLatin1String("easeout")) {
            n.kind = Node::Interp;
            n.warp = w[0] == QLatin1String("linear") ? 'l' : w[0] == QLatin1String("ease") ? 'e' : w[0] == QLatin1String("easein") ? 'i' : 'o';
            n.dur = w.value(1).toDouble();
            n.props = readProps(w, 2);
        } else if (w[0] == QLatin1String("subpixel") || w[0] == QLatin1String("rotate_pad") || w[0] == QLatin1String("transform_anchor") ||
                   w[0] == QLatin1String("function")) {
            continue;                                    // (how Ren'Py draws it; nothing to play)
        } else {
            n.kind = Node::Set;
            n.props = readProps(w, 0);
        }
        out.push_back(n);
    }
    return out;
}

QVector<Node> readAtl(const QString& atl)
{
    QVector<Line> lines;
    for (const QString& raw : atl.split(QLatin1Char('\n'))) {
        if (raw.trimmed().isEmpty()) continue;
        int ind = 0;
        while (ind < raw.size() && raw[ind] == QLatin1Char(' ')) ++ind;
        lines.push_back({ind, raw.trimmed()});
    }
    int i = 0;
    return readBlock(lines, i, 0);
}

struct Run {
    QHash<QString, double> v;
    double ax = 0.5, ay = 0.5;
    double value(const QString& s) const
    {
        if (s == QLatin1String("ax")) return ax;
        if (s == QLatin1String("ay")) return ay;
        return s.toDouble();
    }
    double get(const QString& name) const
    {
        const auto it = v.constFind(name);
        if (it != v.constEnd()) return *it;
        return name.endsWith(QLatin1String("zoom")) || name == QLatin1String("alpha") ? 1.0
               : name.endsWith(QLatin1String("align")) ? 0.5 : 0.0;
    }
};

double warped(char w, double f)
{
    switch (w) {
    case 'e': return 0.5 - qCos(M_PI * f) / 2.0;
    case 'i': return qCos((1.0 - f) * M_PI / 2.0);        // Ren'Py's easein: fast, then slowing
    case 'o': return 1.0 - qCos(f * M_PI / 2.0);          // easeout: slow, then faster
    }
    return f;
}

// plays the statements for t seconds; what is left of t when they end, or -1 while they still run
double play(const QVector<Node>& nodes, double t, Run& r)
{
    const double start = t;
    for (int pass = 0; pass < 3; ++pass) {
        bool again = false;
        for (const Node& n : nodes) {
            switch (n.kind) {
            case Node::Set:
                for (const auto& p : n.props) r.v.insert(p.first, r.value(p.second));
                break;
            case Node::Pause:
                if (t < n.dur) return -1;
                t -= n.dur;
                break;
            case Node::Interp:
                if (t < n.dur) {
                    const double f = warped(n.warp, n.dur > 0 ? t / n.dur : 1.0);
                    for (const auto& p : n.props) {
                        const double from = r.get(p.first);
                        r.v.insert(p.first, from + (r.value(p.second) - from) * f);
                    }
                    return -1;
                }
                for (const auto& p : n.props) r.v.insert(p.first, r.value(p.second));
                t -= n.dur;
                break;
            case Node::Block:
                t = play(n.kids, t, r);
                if (t < 0) return -1;
                break;
            case Node::Parallel: {
                double left = t;
                bool running = false;
                for (const Node& b : n.kids) {
                    const double l = play(b.kids, t, r);
                    if (l < 0) running = true;
                    else left = qMin(left, l);
                }
                if (running) return -1;
                t = left;
                break;
            }
            case Node::Repeat: {
                // one turn of the block took (start - t): the turns that fit whole are skipped, the last
                // one is played from its beginning
                const double len = start - t;
                if (pass > 0 || len <= 1e-6) return -1;   // (nothing takes time: it would never end)
                t = std::fmod(t, len);
                again = true;
                break;
            }
            }
            if (again) break;
        }
        if (!again) return t;
    }
    return -1;
}

const QVector<Node>& tree(const MotionDef& m)
{
    static QMutex lock;
    static QHash<QString, QVector<Node>> cache;
    QMutexLocker l(&lock);
    const QString key = (m.camera ? QStringLiteral("c:") : QStringLiteral("s:")) + m.id;
    auto it = cache.find(key);
    if (it == cache.end()) it = cache.insert(key, readAtl(m.atl));
    return *it;
}

} // namespace

const QVector<MotionDef>& motions()
{
    // Ren'Py hands a transform the state of the one it replaces (a zoom left by the last move would stay):
    // so every move first sets everything its kind touches.
#define CAM0 "subpixel True\nxoffset 0\nyoffset 0\nrotate None\n"
#define ONCE0 "subpixel True\nxoffset 0\nyoffset 0\n"
#define LAST0 "subpixel True\nxoffset 0\nyoffset 0\nyzoom 1.0\n"
    static const QVector<MotionDef> all = {
        // ---- the camera: the whole picture moves (the game: show layer master: ...) ----
        cam("zoomin", "наезд", "наезд", "объектив", "камера медленно приближается к точке", true, false,
            CAM0 "align (ax, ay)\nzoom 1.0\neasein 10.0 zoom 1.25"),
        cam("zoomout", "отъезд", "отъезд", "объектив", "камера медленно отъезжает", true, false,
            CAM0 "align (ax, ay)\nzoom 1.25\nease 10.0 zoom 1.0"),
        cam("close", "крупно", "крупный план", "объектив", "резко и близко: лицо, рука, записка", true, false,
            CAM0 "align (ax, ay)\nzoom 1.0\neasein 0.4 zoom 1.4"),
        cam("jerk", "рывок", "рывок", "объектив", "кадр дёргается вперёд и назад — испуг", true, true,
            CAM0 "align (ax, ay)\nzoom 1.0\neasein 0.25 zoom 1.2\neaseout 0.35 zoom 1.0"),
        cam("panl", "влево", "влево", "объектив", "камера плывёт влево", false, false,
            CAM0 "align (0.5, 0.5)\nzoom 1.0\nparallel:\n    easein 2.0 zoom 1.2\nparallel:\n    ease 12.0 xalign 0.0"),
        cam("panr", "вправо", "вправо", "объектив", "камера плывёт вправо", false, false,
            CAM0 "align (0.5, 0.5)\nzoom 1.0\nparallel:\n    easein 2.0 zoom 1.2\nparallel:\n    ease 12.0 xalign 1.0"),
        cam("walk", "шаги", "шаги", "живой кадр", "кадр покачивается: герой идёт", false, false,
            CAM0 "align (0.5, 0.5)\nzoom 1.04\nblock:\n    ease 0.32 yoffset 9\n    ease 0.32 yoffset 0\n    repeat"),
        cam("run", "бег", "бег", "живой кадр", "кадр трясётся: герой бежит", false, false,
            CAM0 "align (0.5, 0.5)\nzoom 1.06\nblock:\n    linear 0.12 yoffset 12 xoffset -4\n    linear 0.12 yoffset 0 xoffset 4\n    repeat"),
        cam("breathe", "дыхание", "дыхание", "живой кадр", "кадр еле заметно дышит", false, false,
            CAM0 "align (0.5, 0.5)\nzoom 1.0\nblock:\n    ease 2.5 zoom 1.03\n    ease 2.5 zoom 1.0\n    repeat"),
        cam("sway", "качка", "качка", "живой кадр", "кадр плавно качает: голова кружится", false, false,
            CAM0 "align (0.5, 0.5)\nzoom 1.1\nrotate -0.8\nblock:\n    ease 3.0 rotate 0.8\n    ease 3.0 rotate -0.8\n    repeat"),
        cam("tremble", "дрожь", "дрожь", "живой кадр", "кадр мелко дрожит: страх, холод", false, false,
            CAM0 "align (0.5, 0.5)\nzoom 1.02\nblock:\n    linear 0.05 yoffset 4 xoffset 4\n    linear 0.05 yoffset 0 xoffset 0\n"
            "    linear 0.05 yoffset 4 xoffset 4\n    linear 0.05 yoffset 0 xoffset 0\n    linear 0.05 yoffset 4 xoffset 4\n    linear 0.05 yoffset 0 xoffset 0\n"
            "    pause 0.9\n    repeat"),
        cam("mouse", "мышь", "за мышью", "живой кадр", "кадр плывёт за мышкой, как меню игры", false, false,
            CAM0 "align (0.5, 0.5)\nzoom 1.06\nfunction genry_cam_follow"),
        cam("heart", "сердце", "сердце", "живой кадр", "кадр бьётся, как сердце", true, false,
            CAM0 "align (ax, ay)\nzoom 1.0\nblock:\n    easein 0.12 zoom 1.035\n    easeout 0.2 zoom 1.0\n    easein 0.1 zoom 1.02\n"
            "    easeout 0.25 zoom 1.0\n    pause 0.7\n    repeat"),

        // ---- a sprite: the game's own moves (sys_animation_sprite.rpy), then the constructor's ----
        spr("scare", "вздрог", "вздрогнул", "один раз", "вздрагивает (как в игре)", true,
            ONCE0 "linear 0.1 yoffset 20\nlinear 0.1 yoffset 0"),
        spr("happy", "радуется", "радуется", "один раз", "приседает от радости (как в игре)", true,
            ONCE0 "linear 0.2 yoffset 30\nlinear 0.2 yoffset 0"),
        spr("hop", "прыжок", "подпрыгнул", "один раз", "подпрыгивает на месте", true,
            ONCE0 "easein 0.16 yoffset -34\neaseout 0.16 yoffset 0"),
        spr("nod", "кивок", "кивнул", "один раз", "кивает", true,
            ONCE0 "ease 0.15 yoffset 14\nease 0.2 yoffset 0"),
        spr("no", "мотает", "мотает головой", "один раз", "«нет»: влево-вправо", true,
            ONCE0 "ease 0.1 xoffset -14\nease 0.2 xoffset 14\nease 0.2 xoffset -10\nease 0.15 xoffset 0"),
        spr("recoil", "отпрянул", "отпрянул", "один раз", "резко назад и обратно", true,
            ONCE0 "easein 0.12 xoffset 36 yoffset -8\nease 0.5 xoffset 0 yoffset 0"),
        spr("laugh", "смеётся", "смеётся", "всё время", "трясётся от смеха (как в игре)", false,
            LAST0 "block:\n    linear 0.1 yoffset 15\n    linear 0.1 yoffset 0\n    pause 0.1\n    repeat"),
        spr("fear", "дрожит", "дрожит", "всё время", "мелко дрожит (как в игре)", false,
            LAST0 "block:\n    linear 0.05 yoffset 2 xoffset 2\n    linear 0.05 yoffset 0 xoffset 0\n    linear 0.05 yoffset 2 xoffset 2\n"
            "    linear 0.05 yoffset 0 xoffset 0\n    linear 0.05 yoffset 2 xoffset 2\n    linear 0.05 yoffset 0 xoffset 0\n    linear 0.05 yoffset 2 xoffset 2\n"
            "    linear 0.05 yoffset 0 xoffset 0\n    pause 1.2\n    repeat"),
        spr("step", "идёт", "идёт", "всё время", "покачивается на ходу (как в игре)", false,
            LAST0 "block:\n    linear 0.15 yoffset 5\n    linear 0.15 yoffset 0\n    repeat"),
        spr("breath", "дышит", "дышит", "всё время", "еле заметно дышит — живой", false,
            LAST0 "block:\n    ease 2.2 yzoom 1.012\n    ease 2.2 yzoom 1.0\n    repeat"),
        spr("sob", "плачет", "плачет", "всё время", "плечи вздрагивают", false,
            LAST0 "block:\n    ease 0.18 yoffset 6\n    ease 0.22 yoffset 0\n    ease 0.18 yoffset 5\n    ease 0.22 yoffset 0\n    pause 0.9\n    repeat"),
    };
#undef CAM0
#undef ONCE0
#undef LAST0
    return all;
}

const MotionDef* motionByWord(const QString& word, bool camera)
{
    const QString w = lower(word);
    for (const MotionDef& m : motions())
        if (m.camera == camera && lower(m.word) == w) return &m;
    return nullptr;
}

const MotionDef* motionById(const QString& id, bool camera)
{
    for (const MotionDef& m : motions())
        if (m.camera == camera && m.id == id) return &m;
    return nullptr;
}

QString motionTransform(const MotionDef& m)
{
    return (m.camera ? QStringLiteral("genry_cam_") : QStringLiteral("genry_mv_")) + m.id;
}

QString motionsHub()
{
    QStringList out;
    // nothing: what stands in a move's place when there is none (and brings the picture back)
    out << QStringLiteral("transform genry_cam_none:\n    subpixel True\n    rotate None\n    ease 0.4 zoom 1.0 xoffset 0 yoffset 0\n");
    out << QStringLiteral("transform genry_mv_none:\n    xoffset 0\n    yoffset 0\n");
    out << QStringLiteral("transform genry_mv_still:\n    xoffset 0\n    yoffset 0\n    yzoom 1.0\n");
    for (const MotionDef& m : motions()) {
        out << QStringLiteral("transform %1%2:").arg(motionTransform(m), m.camera ? QStringLiteral("(ax=0.5, ay=0.5)") : QString());
        for (const QString& l : m.atl.split(QLatin1Char('\n'))) out << QStringLiteral("    ") + l;
        out << QString();
        if (m.once) {                                    // its twin: so that it plays again given twice in a row
            out << QStringLiteral("transform %1_b%2:").arg(motionTransform(m), m.camera ? QStringLiteral("(ax=0.5, ay=0.5)") : QString());
            for (const QString& l : m.atl.split(QLatin1Char('\n'))) out << QStringLiteral("    ") + l;
            out << QString();
        }
    }
    return out.join(QLatin1Char('\n'));
}

QHash<QString, double> motionAt(const MotionDef& m, double t, double ax, double ay)
{
    Run r;
    r.ax = ax;
    r.ay = ay;
    play(tree(m), qMax(0.0, t), r);
    return r.v;
}

} // namespace tb
