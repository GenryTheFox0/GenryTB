#include "Compiler.h"
#include "Catalog.h"
#include "Motion.h"

#include <algorithm>

namespace tb {
namespace {

// a line's text for a Ren'Py string: its quotes and backslashes escaped, [ ] kept literal
// (Ren'Py interpolates [x]); {i}...{/i} and the like stay the text tags they are
// the game's living text in a mod's words: {дрожь} {прыг} {появление} {хаос} {кругом} {курсив} (and their {/...});
// (the game's own {chaos} wants 12 fonts the game never shipped: GenryTB puts them in, the hub picks by language)
QString livingText(const QString& s)
{
    static const QPair<const char*, const char*> tags[] = {
        {"дрожь", "sc"}, {"прыг", "bt"}, {"появление", "fi"}, {"хаос", "chaos"}, {"кругом", "rotat"}, {"курсив", "i"}};
    static const QPair<const char*, const char*> open[] = {
        {"дрожь", "sc=3"}, {"прыг", "bt=4"}, {"появление", "fi"}, {"хаос", "chaos"}, {"кругом", "rotat"}, {"курсив", "i"}};
    QString t = s;
    for (const auto& p : tags) {
        const QString ru = QString::fromUtf8(p.first), en = QString::fromLatin1(p.second);
        t.replace(QStringLiteral("{/") + ru + QLatin1Char('}'), QStringLiteral("{/") + en + QLatin1Char('}'));
        t.replace(QLatin1Char('{') + ru + QLatin1Char('='), QLatin1Char('{') + en + QLatin1Char('='));
    }
    for (const auto& p : open)
        t.replace(QLatin1Char('{') + QString::fromUtf8(p.first) + QLatin1Char('}'), QLatin1Char('{') + QString::fromLatin1(p.second) + QLatin1Char('}'));
    return t;
}

QString renpyText(const QString& s)
{
    QString t = livingText(s);
    t.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    t.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    t.replace(QLatin1Char('['), QStringLiteral("[["));
    t.replace(QLatin1Char('%'), QStringLiteral("%%"));
    // {словарь=пояснение}слово{/словарь}: a link the hub shows as a note on paper
    t.replace(QStringLiteral("{словарь="), QStringLiteral("{a=genry_dict:"));
    t.replace(QStringLiteral("{/словарь}"), QStringLiteral("{/a}"));
    return t;
}

QString quoted(const QString& s)
{
    return QLatin1Char('"') + renpyText(s) + QLatin1Char('"');
}

QString pyString(const QString& s)
{
    QString t = s;
    t.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    t.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QLatin1Char('"') + t + QLatin1Char('"');
}

// a mod's name, author, description as the «Моды» screen shows them: its text takes [x] and {tag} for
// its own - shown as written
QString shownString(const QString& s)
{
    QString t = s;
    t.replace(QLatin1Char('['), QStringLiteral("[["));
    t.replace(QLatin1Char('{'), QStringLiteral("{{"));
    return QStringLiteral("u") + pyString(t);
}

} // namespace

static std::function<QRect(const QString&)> g_headBox;
void setHeadBox(std::function<QRect(const QString& sprite)> fn) { g_headBox = std::move(fn); }

int hubVersion() { return 8; }

QString hubScript()
{
    static const char* const text =
#include "Hub.inc"
        ;
    static const char* const motion =
#include "HubMotion.inc"
        ;
    static const char* const words =             // the hub's words in the game's language (genry_tb_t)
#include "HubTl.inc"
        ;
    return QString::fromUtf8(text).replace(QStringLiteral("@VERSION@"), QString::number(hubVersion())).trimmed() + QLatin1Char('\n') +
           QString::fromUtf8(words).trimmed() + QLatin1Char('\n') +
           QString::fromUtf8(motion).replace(QStringLiteral("@MOTIONS@"), motionsHub()).trimmed() + QLatin1Char('\n');
}

Compiled compileStory(const Story& story, const Catalog& cat)
{
    Compiled out;
    if (!story.problems().isEmpty()) {
        const Step& b = story.problems().first();
        out.error = QStringLiteral("строка %1: %2").arg(b.line).arg(b.error);
        return out;
    }
    const QString id = story.id;
    auto lab = [&](const QString& scene) { return labelName(id, scene); };
    // a story mod's points live in persistent: won in episode 2, still there in episode 4 (each episode is its own start)
    auto var = [&](const QString& v) { return (story.hooked() ? QStringLiteral("persistent.") : QString()) + varName(id, v); };
    QStringList L;
    L << QStringLiteral("# -*- coding: utf-8 -*-")
      << QStringLiteral("# GenryTB V1 mod «%1» by %2. Made from story.txt - edits here are overwritten.")
             .arg(story.name.isEmpty() ? id : story.name, story.author.isEmpty() ? QStringLiteral("?") : story.author)
      << QString();
    out.startLabel = lab(story.labels.first());
    L << QStringLiteral("init 5 python:")
      << QStringLiteral("    genry_tb_register(%1, %2, %3, %4, desc=%5, scenes=%6, choices=%7%8)")
             .arg(pyString(id), shownString(story.name.isEmpty() ? id : story.name), shownString(story.author),
                  pyString(out.startLabel), shownString(story.description))
             .arg(story.labels.size())
             .arg(std::count_if(story.steps.begin(), story.steps.end(), [](const Step& x) { return x.kind == Step::Menu; }))
             .arg([&] {                          // its achievements, when it has any (the mods menu shows ★ got/all)
                 const auto n = std::count_if(story.steps.begin(), story.steps.end(), [](const Step& x) { return x.kind == Step::Achievement; });
                 return (n ? QStringLiteral(", ach=%1").arg(n) : QString()) + (story.adult ? QStringLiteral(", adult=True") : QString());
             }());
    for (const auto& h : story.inserts)
        L << QStringLiteral("    genry_tb_hook_insert(%1, %2)").arg(pyString(h.first), pyString(lab(h.second)));
    for (const auto& h : story.replaces)
        L << QStringLiteral("    genry_tb_hook_replace(%1, %2)").arg(pyString(h.first), pyString(lab(h.second)));
    L << QString();
    for (const Speaker& s : story.extraSpeakers)
        L << QStringLiteral("define %1 = Character(%2, what_prefix=char_what_prefix_quote, what_suffix=char_what_suffix_quote)")
                 .arg(var(s.var), pyString(s.name));
    for (const QString& v : story.vars) L << QStringLiteral("default %1 = 0").arg(var(v));
    if (!story.extraSpeakers.isEmpty() || !story.vars.isEmpty()) L << QString();

    bool inLabel = false, first = true;
    // Tiny Bunny shows its textbox only when told (window show before its lines, window hide before a new
    // scene): so does a mod - else its lines play with no box at all
    bool window = false;
    auto ensureLabel = [&]() {
        if (!inLabel) {                                  // commands before the first scene: into the first scene
            L << QStringLiteral("label %1:").arg(out.startLabel);
            inLabel = true;
        }
    };
    auto with = [](const QString& tr) { return tr.isEmpty() ? QString() : QStringLiteral(" with ") + tr; };
    const QString in = QStringLiteral("    ");
    // the pseudo-motion, as the story stands line by line: who is shown where and how they move, the camera
    // and the tape on the whole picture (the master layer), the weather
    struct Shown {
        QString sprite, pos, size, motion, last;      // motion: a lasting move; last: the transform given last
    };
    QHash<QString, Shown> shown;                      // (motion: the lasting move's id)
    QString cam, camAt, camLast, film, weather;
    auto num = [](double v) { return QString::number(v, 'g', 4); };
    // a move that plays once would not play again given twice in a row (Ren'Py goes on with the same ATL
    // where it stopped): its twin is given instead
    auto moveName = [](const MotionDef& m, const QString& last) {
        const QString n = motionTransform(m);
        return m.once && last == n ? n + QStringLiteral("_b") : n;
    };
    auto layerLine = [&](bool newMove = false) {
        QStringList at;
        if (const MotionDef* m = motionById(cam, true)) {
            const QPoint p = camAt.isEmpty() ? QPoint(960, 540) : spotPlace(camAt, 0, 1);
            if (newMove || camLast.isEmpty()) camLast = moveName(*m, camLast);
            at << QStringLiteral("%1(%2, %3)").arg(camLast, num(p.x() / 1920.0), num(p.y() / 1080.0));
        }
        // always the camera's place, then the tape's (a transform takes the state of the one in its place:
        // with places that shift, a zoom or a shader would wander from one to another)
        if (at.isEmpty() && (film.isEmpty() || film == QLatin1String("memory"))) return QStringLiteral("show layer master");
        if (at.isEmpty()) at << QStringLiteral("genry_cam_none");
        // (a memory is a frame over the picture - a screen; the picture itself is as it is)
        at << (film.isEmpty() || film == QLatin1String("memory") ? QStringLiteral("genry_film_none") : QStringLiteral("genry_film_") + film);
        return QStringLiteral("show layer master at ") + at.join(QStringLiteral(", "));
    };
    // a sprite's three places: where it stands, the move that lasts, the move played once
    // where a sprite stands; taller or smaller - the same place with its size
    auto place = [&](const Shown& s) {
        if (s.size.isEmpty()) return s.pos.isEmpty() ? QStringLiteral("genry_center") : s.pos;
        double z = s.size.toInt() / 100.0;
        int dx = 0;
        if (s.pos == QLatin1String("genry_left")) dx = -480;
        else if (s.pos == QLatin1String("genry_right")) dx = 480;
        else if (s.pos == QLatin1String("genry_far")) z *= 0.8;
        else if (s.pos == QLatin1String("genry_close")) z *= 1.25;
        else if (!s.pos.isEmpty() && s.pos != QLatin1String("genry_center")) return s.pos;      // (the game's own place: as it is)
        return QStringLiteral("genry_at(%1, %2)").arg(dx).arg(num(z));
    };
    auto atList = [&](Shown& s, const MotionDef* once) {
        QStringList at{place(s)};
        const MotionDef* lasting = motionById(s.motion, false);
        at << (lasting ? motionTransform(*lasting) : QStringLiteral("genry_mv_still"));
        if (once) {
            s.last = moveName(*once, s.last);
            at << s.last;
        } else {
            s.last.clear();
            at << QStringLiteral("genry_mv_none");
        }
        return QStringLiteral(" at ") + at.join(QStringLiteral(", "));
    };
    for (const Step& s : story.steps) {
        if (s.kind == Step::Label) {
            if (inLabel && s.a == story.labels.first()) continue;   // (already opened by commands before it)
            L << QString() << QStringLiteral("label %1:").arg(lab(s.a));
            out.labels << lab(s.a);
            inLabel = true;
            window = false;                              // (a scene can be jumped to: its box shown anew)
            if (first) {
                // the mod starts here as the game's own story does: the menu's music away, a clean screen
                L << in + QStringLiteral("stop music fadeout 2.0") << in + QStringLiteral("scene black")
                  << in + QStringLiteral("show layer master") << in + QStringLiteral("$ renpy.block_rollback()");
                first = false;
            }
            continue;
        }
        ensureLabel();
        switch (s.kind) {
        case Step::Background:
            if (window && !s.transition.isEmpty()) {
                L << in + QStringLiteral("window hide");
                window = false;
            }
            // a new place is a clean shot: nobody on it, no snow, the camera still (the change itself is
            // part of the transition)
            if (s.a.startsWith(QLatin1String("images/"))) {      // a picture file (or its layers)
                const QStringList layers = s.a.split(QLatin1Char('+'));
                if (layers.size() == 1) {
                    L << in + QStringLiteral("scene expression ") + pyString(layers.first());
                } else {
                    QStringList args;
                    for (const QString& x : layers) args << QStringLiteral("(0, 0), ") + pyString(x);
                    L << in + QStringLiteral("scene expression Composite((1920, 1080), %1)").arg(args.join(QStringLiteral(", ")));
                }
            } else {
                L << in + QStringLiteral("scene ") + s.a;
            }
            shown.clear();
            weather.clear();
            if (!cam.isEmpty()) {
                cam.clear();
                camLast.clear();
                L << in + layerLine();
            }
            if (!s.transition.isEmpty()) L << in + QStringLiteral("with ") + s.transition;
            break;
        case Step::Show: {
            const QString who = s.a.section(QLatin1Char(' '), 0, 0);
            Shown& sh = shown[who];
            sh.sprite = s.a;
            sh.pos = s.b;
            sh.size = s.c;
            L << in + QStringLiteral("show ") + s.a + atList(sh, nullptr) + with(s.transition);
            break;
        }
        case Step::Hide:
            shown.remove(s.a);
            L << in + QStringLiteral("hide ") + s.a + with(s.transition);
            break;
        case Step::Camera:
            cam = s.a;
            camAt = s.b;
            if (cam.isEmpty()) camLast.clear();
            L << in + layerLine(true);
            break;
        case Step::Film:
            film = s.a;
            L << in + layerLine();
            L << in + (film == QLatin1String("memory") || film == QLatin1String("tv") || film == QLatin1String("peephole")
                           ? QStringLiteral("show screen genry_tb_film(\"%1\")").arg(film)
                                                                                      : QStringLiteral("hide screen genry_tb_film"));
            break;
        case Step::Paper:
            if (window) L << in + QStringLiteral("window hide");
            window = false;
            L << in + QStringLiteral("call screen genry_tb_note(%1)").arg(shownString(s.a));
            break;
        case Step::Poster: {
            QString sprite = s.transition.isEmpty() ? shown.value(s.a).sprite : s.transition;
            if (sprite.isEmpty()) {
                const auto all = cat.spritesOf(s.a);
                if (!all.isEmpty()) sprite = all.first()->name;
            }
            const QRect head = g_headBox ? g_headBox(sprite) : QRect();
            const QRect box = head.isValid() ? head : QRect(660, 60, 600, 600);
            if (window) L << in + QStringLiteral("window hide");
            window = false;
            L << in + QStringLiteral("call screen genry_tb_poster(%1, %2, %3, (%4, %5, %6, %7))")
                          .arg(shownString(s.b), shownString(s.c), pyString(sprite))
                          .arg(box.x()).arg(box.y()).arg(box.width()).arg(box.height());
            break;
        }
        case Step::Move: {
            const MotionDef* m = motionById(s.b, false);
            const bool known = shown.contains(s.a);
            Shown& sh = shown[s.a];
            if (!m || !m->once) sh.motion = m ? m->id : QString();
            // (not shown by this scene's own lines: by its name alone - the game keeps the look it has)
            L << in + QStringLiteral("show ") + (known ? sh.sprite : s.a) + atList(sh, m && m->once ? m : nullptr);
            if (!known) shown.remove(s.a);
            break;
        }
        case Step::Weather:
            if (!weather.isEmpty() && weather != s.a) L << in + QStringLiteral("hide genry_tb_") + weather;
            if (!s.a.isEmpty() && weather != s.a) L << in + QStringLiteral("show genry_tb_%1 zorder 80 at genry_tb_weather").arg(s.a);
            weather = s.a;
            break;
        case Step::Say: {
            if (!window) {
                L << in + QStringLiteral("window show");
                window = true;
            }
            QString who = s.a;
            if (who.startsWith(QLatin1Char('@'))) who = var(who.mid(1));
            L << in + (who.isEmpty() ? QString() : who + QLatin1Char(' ')) + quoted(s.b);
            break;
        }
        case Step::Voice: L << in + QStringLiteral("voice ") + pyString(s.a); break;
        case Step::Music:
            L << in + (s.a.isEmpty() ? QStringLiteral("stop music fadeout 2.0")
                                     : QStringLiteral("play music %1 fadein 2.0").arg(pyString(s.a)));
            break;
        case Step::Ambience:
            L << in + (s.a.isEmpty() ? QStringLiteral("stop fon fadeout 2.0")
                                     : QStringLiteral("play fon %1 loop fadein 2.0").arg(pyString(s.a)));
            break;
        case Step::Sound: L << in + (s.a.isEmpty() ? QStringLiteral("stop sound") : QStringLiteral("play sound ") + pyString(s.a)); break;
        case Step::Pause: L << in + QStringLiteral("$ renpy.pause(%1)").arg(s.a); break;
        case Step::Window:
            L << in + QStringLiteral("window ") + s.a;
            window = s.a == QLatin1String("show");
            break;
        case Step::Menu:
            if (s.a == QLatin1String("spots")) {
                // the game's own picture choice: glowing spots on the scene, each with its word
                QStringList opts;
                for (int k = 0; k < s.choices.size(); ++k) {
                    const QPoint at = spotPlace(s.places.value(k), k, int(s.choices.size()));
                    opts << QStringLiteral("(%1, %2, %3, %4)").arg(shownString(s.choices[k]), pyString(lab(s.targets[k]))).arg(at.x()).arg(at.y());
                }
                if (window) L << in + QStringLiteral("window hide");
                window = false;
                L << in + QStringLiteral("call screen genry_tb_spots([%1])").arg(opts.join(QStringLiteral(", ")));
                break;
            }
            {
                // the hub's own list (the game's is small words in a corner); with seconds - against the clock
                QStringList opts;
                for (int k = 0; k < s.choices.size(); ++k)
                    opts << QStringLiteral("(%1, %2)").arg(shownString(s.choices[k]), pyString(lab(s.targets[k])));
                if (window) L << in + QStringLiteral("window hide");
                window = false;
                L << in + QStringLiteral("call screen genry_tb_choice([%1], %2)")
                              .arg(opts.join(QStringLiteral(", ")), s.a == QLatin1String("timed") ? s.b : QStringLiteral("0"));
            }
            break;
        case Step::Jump: L << in + QStringLiteral("jump ") + lab(s.target); break;
        case Step::Set: L << in + QStringLiteral("$ %1 = %2").arg(var(s.a), s.b); break;
        case Step::Add: L << in + QStringLiteral("$ %1 += %2").arg(var(s.a), s.b); break;
        case Step::If:
            L << in + QStringLiteral("if %1 %2 %3:").arg(var(s.a), s.b, s.c) << in + in + QStringLiteral("jump ") + lab(s.target);
            break;
        case Step::End: L << in + QStringLiteral("jump genry_tb_the_end"); break;
        case Step::GameJump: L << in + QStringLiteral("jump ") + s.target; break;
        case Step::Return:
            if (window) L << in + QStringLiteral("window hide");
            window = false;
            L << in + QStringLiteral("return");
            break;
        case Step::Code: L << in + QStringLiteral("$ ") + s.a; break;
        case Step::Title:
            L << in + QStringLiteral("window hide");
            window = false;
            L << in + QStringLiteral("show screen genry_tb_title(%1)").arg(shownString(s.a)) << in + QStringLiteral("with Dissolve(0.6)")
              << in + QStringLiteral("$ renpy.pause(%1)").arg(s.b) << in + QStringLiteral("hide screen genry_tb_title")
              << in + QStringLiteral("with Dissolve(0.6)");
            break;
        case Step::Epilogue: {
            L << in + QStringLiteral("window hide");
            window = false;
            QStringList layers;
            if (!s.transition.isEmpty())
                for (const QString& layer : s.transition.split(QLatin1Char('+'))) layers << pyString(layer);
            L << in + QStringLiteral("call screen genry_tb_epilogue(%1, %2, [%3], %4)")
                          .arg(shownString(s.c), shownString(s.a), layers.join(QStringLiteral(", ")),
                               s.b.isEmpty() ? QStringLiteral("BTN_END") : shownString(s.b));
            break;
        }
        case Step::Button:
            if (window) L << in + QStringLiteral("window hide");
            window = false;
            L << in + QStringLiteral("call screen d5_end(caption=%1)").arg(shownString(s.a));
            break;
        case Step::Struggle:
            if (window) L << in + QStringLiteral("window hide");
            window = false;
            L << in + QStringLiteral("call screen genry_tb_struggle(%1, %2)").arg(shownString(s.a), s.b)
              << in + QStringLiteral("if _return:") << in + QStringLiteral("    jump ") + lab(s.target)
              << in + QStringLiteral("jump ") + lab(s.c);
            break;
        case Step::Suefa: {
            L << in + QStringLiteral("window hide");
            window = false;
            QString place;                                       // None - the game's field; else blurred into fog
            if (s.b.isEmpty()) {
                L << in + QStringLiteral("$ renpy.pause(0.1, hard=True)") << in + QStringLiteral("$ genry_tb_take_backdrop()");
                place = QStringLiteral("GenryTbShots.backdrop");
            } else if (s.b == QLatin1String("field")) {
                place = QStringLiteral("None");
            } else if (s.b.startsWith(QLatin1String("mods/"))) {
                place = pyString(s.b) + QStringLiteral(", False");     // a drawn place: as it is, not blurred
            } else if (s.b.contains(QLatin1Char('+'))) {
                QStringList args;
                for (const QString& x : s.b.split(QLatin1Char('+'))) args << QStringLiteral("(0, 0), ") + pyString(x.trimmed());
                place = QStringLiteral("Composite((1920, 1080), %1)").arg(args.join(QStringLiteral(", ")));
            } else {
                place = pyString(s.b);
            }
            L << in + QStringLiteral("$ genry_tb_r = \"draw\"") << in + QStringLiteral("while genry_tb_r == \"draw\":")
              << in + QStringLiteral("    call screen genry_tb_suefa(%1, %2)").arg(pyString(s.a), place)
              << in + QStringLiteral("    $ genry_tb_r = _return")
              << in + QStringLiteral("if genry_tb_r == \"win\":") << in + QStringLiteral("    jump ") + lab(s.target)
              << in + QStringLiteral("jump ") + lab(s.c);
            break;
        }
        case Step::Snapshot:
            L << in + QStringLiteral("window hide");
            window = false;
            L << in + QStringLiteral("$ renpy.pause(0.1, hard=True)") << in + QStringLiteral("$ genry_tb_take_snap()")
              << in + QStringLiteral("call screen genry_tb_polaroid(GenryTbShots.snap, %1)").arg(shownString(s.a));
            break;
        case Step::Riddle: {
            L << in + QStringLiteral("window hide");
            window = false;
            QStringList answers;
            for (const QString& x : s.b.split(QLatin1Char('/'))) answers << shownString(x.trimmed());
            L << in + QStringLiteral("$ genry_tb_answer = renpy.input(%1, length=40, screen=\"genry_tb_riddle\") or u\"\"").arg(shownString(s.a))
              << in + QStringLiteral("if genry_tb_check(genry_tb_answer, [%1]):").arg(answers.join(QStringLiteral(", ")))
              << in + QStringLiteral("    jump ") + lab(s.target) << in + QStringLiteral("jump ") + lab(s.c);
            break;
        }
        case Step::Explore: {
            L << in + QStringLiteral("window hide");
            window = false;
            QStringList items;
            for (int k = 0; k < s.choices.size(); ++k) {
                const QPoint at = spotPlace(s.places.value(k), k, int(s.choices.size()));
                const QString to = s.targets.value(k);
                items << QStringLiteral("(%1, %2, %3, %4, %5)")
                             .arg(shownString(s.choices[k])).arg(at.x()).arg(at.y())
                             .arg(s.notes.value(k).isEmpty() ? QStringLiteral("None") : shownString(s.notes.value(k)),
                                  to.isEmpty() ? QStringLiteral("None") : pyString(lab(to)));
            }
            L << in + QStringLiteral("$ genry_tb_seen = set()") << in + QStringLiteral("while True:")
              << in + QStringLiteral("    call screen genry_tb_explore([%1])").arg(items.join(QStringLiteral(", ")))
              << in + QStringLiteral("    if _return[1]:") << in + QStringLiteral("        $ renpy.jump(_return[1])")
              << in + QStringLiteral("    window show") << in + QStringLiteral("    $ renpy.say(None, _return[0])")
              << in + QStringLiteral("    window hide");
            break;
        }
        case Step::Puzzle: {
            L << in + QStringLiteral("window hide");
            window = false;
            QString pic = pyString(s.a);
            if (s.a.contains(QLatin1Char('+'))) {
                QStringList args;
                for (const QString& x : s.a.split(QLatin1Char('+'))) args << QStringLiteral("(0, 0), ") + pyString(x.trimmed());
                pic = QStringLiteral("Composite((1920, 1080), %1)").arg(args.join(QStringLiteral(", ")));
            }
            L << in + QStringLiteral("call screen genry_tb_puzzle(GenryTbPuzzle(%1, %2, %3))").arg(pic, s.b.section(QLatin1Char('x'), 0, 0), s.b.section(QLatin1Char('x'), 1, 1))
              << in + QStringLiteral("jump ") + lab(s.target);
            break;
        }
        case Step::InHand:
            L << in + QStringLiteral("window hide");
            window = false;
            {
                QString pic = pyString(s.a);
                if (s.a.contains(QLatin1Char('+'))) {          // layers of a picture (an ending's art: bg + chara)
                    QStringList args;
                    for (const QString& x : s.a.split(QLatin1Char('+'))) args << QStringLiteral("(0, 0), ") + pyString(x.trimmed());
                    pic = QStringLiteral("Composite((1920, 1080), %1)").arg(args.join(QStringLiteral(", ")));
                }
                L << in + QStringLiteral("call screen genry_tb_inhand(%1, %2)").arg(pic, shownString(s.b));
            }
            break;
        case Step::Achievement:
            L << in + QStringLiteral("$ genry_tb_achieve(%1, %2, %3)").arg(pyString(story.id), shownString(s.a), shownString(s.b));
            break;
        case Step::Phone: {
            // the game's own: the phone, the receiver up, the camera close; dial - each number its scene
            L << in + QStringLiteral("window hide");
            window = false;
            L << in + QStringLiteral("scene d5_polina_phone_bg") << in + QStringLiteral("show d5_polina_phone 2")
              << in + QStringLiteral("show layer master:") << in + QStringLiteral("    align (.5, .5)")
              << in + QStringLiteral("    zoom 1.3") << in + QStringLiteral("with Dissolve(.5)")
              << in + QStringLiteral("play sound \"sounds/sfx_phone_take.ogg\"")
              << in + QStringLiteral("$ genry_tb_phone_number = \"\"")
              << in + QStringLiteral("while True:")
              << in + QStringLiteral("    show screen genry_tb_phone_display()")
              << in + QStringLiteral("    call screen genry_tb_phone_keys()")
              << in + QStringLiteral("    if _return == \"reset\":")
              << in + QStringLiteral("        $ genry_tb_phone_number = \"\"")
              << in + QStringLiteral("    elif len(genry_tb_phone_number) >= %1:").arg(s.choices.value(0).size());
            for (int k = 0; k < s.choices.size(); ++k)
                L << in + QStringLiteral("        if genry_tb_phone_number == \"%1\":").arg(s.choices[k])
                  << in + QStringLiteral("            hide screen genry_tb_phone_display")
                  << in + QStringLiteral("            show layer master")
                  << in + QStringLiteral("            jump ") + lab(s.targets.value(k));
            if (!s.c.isEmpty()) {
                L << in + QStringLiteral("        hide screen genry_tb_phone_display") << in + QStringLiteral("        show layer master")
                  << in + QStringLiteral("        jump ") + lab(s.c);
            } else {
                L << in + QStringLiteral("        play sound \"voice/episod/tel/Rong.ogg\" loop")
                  << in + QStringLiteral("        $ renpy.say(Operator, genry_tb_t(u\"Неправильно набран номер. Неправильно набран номер. Неправильно набран...\"))")
                  << in + QStringLiteral("        stop sound")
                  << in + QStringLiteral("        window hide")
                  << in + QStringLiteral("        $ genry_tb_phone_number = \"\"");
            }
            break;
        }
        case Step::Credits: {
            if (window) L << in + QStringLiteral("window hide");
            window = false;
            QStringList items;
            for (const QString& x : s.a.split(QLatin1Char('\n'))) items << shownString(x);
            const double sec = qBound(8.0, 2.2 * items.size() + 4.0, 60.0);
            L << in + QStringLiteral("call screen genry_tb_credits([%1], %2)").arg(items.join(QStringLiteral(", "))).arg(sec);
            break;
        }
        case Step::Shake: L << in + QStringLiteral("with vpunch"); break;
        case Step::Flash: L << in + QStringLiteral("with Fade(0.1, 0.0, 0.5, color=\"#fff\")"); break;
        default: break;
        }
    }
    ensureLabel();
    L << in + QStringLiteral("jump genry_tb_the_end") << QString();    // the story ran out: КОНЕЦ, the menu
    out.rpy = L.join(QLatin1Char('\n'));
    if (!out.labels.contains(out.startLabel)) out.labels.prepend(out.startLabel);
    out.ok = true;
    return out;
}

} // namespace tb
