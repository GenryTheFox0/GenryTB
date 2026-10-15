// GenryTB V1 self-test: the core against the real game (Tiny Bunny must be installed).
#include "Builder.h"
#include "Catalog.h"
#include "Compiler.h"
#include "Motion.h"
#include "Renderer.h"
#include "Rpa.h"
#include "Story.h"

#include <QFile>
#include <algorithm>
#include <QGuiApplication>
#include <QTextStream>

using namespace tb;

static int g_pass = 0, g_fail = 0;
static QTextStream& out()
{
    static QTextStream s(stdout);
    s.setEncoding(QStringConverter::Utf8);
    return s;
}
#define CHECK(cond, what)                                                         \
    do {                                                                          \
        if (cond) { ++g_pass; }                                                   \
        else { ++g_fail; out() << "FAIL: " << what << " (" << #cond << ")\n"; }  \
    } while (0)

static QString readText(const QString& p)
{
    QFile f(p);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    Catalog cat;
    QString err;
    CHECK(cat.load(Catalog::findData(QStringLiteral("tb_catalog.json")), &err), "каталог: " + err);
    CHECK(cat.sprites().size() > 5000, "спрайтов в каталоге");
    CHECK(cat.sprite(QStringLiteral("Mom Normal m_day 01 norm aside")) != nullptr, "спрайт мамы");
    CHECK(cat.spriteWhoFor(QStringLiteral("алиса")) == QLatin1String("Fox"), "Алиса = Fox");
    CHECK(cat.spriteWhoFor(QStringLiteral("Семен")) == QLatin1String("Semen"), "ё/е");
    CHECK(cat.speaker(QStringLiteral("Мама")) && cat.speaker(QStringLiteral("Мама"))->var == QLatin1String("Mam"), "говорящий Мама");
    CHECK(cat.musicFile(QStringLiteral("Menu_Tiny_Bunny")) == QLatin1String("music/Menu_Tiny_Bunny.ogg"), "музыка по короткому имени");

    const QString game = Vfs::findGame(qEnvironmentVariable("TB_GAME"));
    Vfs vfs;
    CHECK(!game.isEmpty() && vfs.mount(game, &err), "игра: " + err);
    CHECK(vfs.files().size() > 19000, "файлов в архиве");
    CHECK(vfs.has(QStringLiteral("sprite/Mom/Normal/m_day/1_body/01.png")), "путь без images/");

    // the starter story: no problems, the right scene at its lines
    const QString text = readText(Catalog::findData(QStringLiteral("starter_story.txt")));
    const Story st = parseStory(text, cat);
    for (const Step& s : st.problems()) out() << "  problem line " << s.line << ": " << s.error << "\n";
    CHECK(st.problems().isEmpty(), "стартовый мод без проблем");
    CHECK(st.labels == (QStringList{"start", "believe", "ignore", "night", "brave"}), "сцены");
    CHECK(st.vars == QStringList{QStringLiteral("смелость")}, "переменные");
    const SceneState s14 = stateAt(st, cat, 15);
    CHECK(s14.bg == QLatin1String("bg kitchen1_1"), "фон на 15 строке: " + s14.bg);
    CHECK(s14.sprites.size() == 1 && s14.who == QStringLiteral("Мама"), "мама говорит");
    const SceneState s16 = stateAt(st, cat, 17);
    CHECK(s16.sprites.size() == 2 && s16.sprites[1].second == QLatin1String("genry_left"), "оля слева");
    const SceneState s9 = stateAt(st, cat, 10);
    CHECK(s9.thoughts && s9.who.isEmpty(), "мысли курсивом без имени");

    // problems are found, not swallowed
    const Story badSt = parseStory(QStringLiteral(
        "@mod_id t\n: start\nфон nothing_here\nпоказать Мама Happy xxx\nпоказать Кто-то\nвыбор\n- без стрелки\nконецвыбора\nпереход nowhere\n"),
        cat);
    CHECK(badSt.problems().size() >= 5, QStringLiteral("проблем найдено: %1").arg(badSt.problems().size()));

    // the compiled script
    const Compiled c = compileStory(st, cat);
    CHECK(c.ok, "компиляция: " + c.error);
    CHECK(c.startLabel == QLatin1String("first_mod__start"), "стартовая метка " + c.startLabel);
    CHECK(c.rpy.contains(QStringLiteral("show Mom Normal m_day 01 norm aside at genry_center, genry_mv_still, genry_mv_none with dissolve")), "show мамы");
    CHECK(c.rpy.contains(QStringLiteral("show Olya Serious m_day 00 good ahead 01 at genry_left, genry_mv_still, genry_mv_none with Dissolve(0.3)")), "show оли слева");
    CHECK(c.rpy.contains(QStringLiteral("voice \"voice/karina/21 K.ogg\"")), "голос");
    CHECK(c.rpy.contains(QStringLiteral("Mam \"Не подначивай ребёнка.\"")), "реплика мамы");
    CHECK(c.rpy.contains(QStringLiteral("Ant_m \"Мне ведь уже двенадцать. Чего бояться?\"")), "мысли");
    CHECK(c.rpy.contains(QStringLiteral("call screen genry_tb_choice([(u\"Поверить Оле\", \"first_mod__believe\"), ")), "выбор");
    CHECK(c.rpy.contains(QStringLiteral("default first_mod_smelost = 0")), "переменная транслитом");
    CHECK(c.rpy.contains(QStringLiteral("if first_mod_smelost >= 1:")), "если");
    CHECK(c.rpy.contains(QStringLiteral("play music \"music/1_Melancholy(Tiny_Bunny).ogg\" fadein 2.0")), "музыка");
    const Story esc = parseStory(QStringLiteral("@mod_id e\n: start\nОля: Он сказал \"стой\" [тихо] 100%\n"), cat);
    const Compiled ce = compileStory(esc, cat);
    CHECK(ce.rpy.contains(QStringLiteral("Oly \"Он сказал \\\"стой\\\" [[тихо] 100%%\"")), "экранирование: " + ce.rpy.section('\n', -3, -3));

    CHECK(c.rpy.contains(QStringLiteral("scenes=5, choices=1)")), "регистрация со статистикой");
    CHECK(c.rpy.contains(QStringLiteral("jump genry_tb_the_end")), "конец мода");
    CHECK(hubScript().contains(QStringLiteral("screen genry_tb_mods():")) && hubScript().contains(QStringLiteral("hub %1").arg(hubVersion())), "хаб");
    const SceneState cover = coverState(st, cat);
    CHECK(cover.bg == QLatin1String("bg kitchen1_1") && cover.sprites.size() == 1 && cover.text.isEmpty(), "обложка: кухня с мамой");

    // the preview draws the game's picture
    Renderer r(vfs, cat);
    const QImage f = r.frame(s16);
    CHECK(f.size() == QSize(1920, 1080), "кадр 1920x1080");
    const QColor mid = f.pixelColor(960, 400);
    CHECK(mid.value() > 10, "кадр не чёрный");
    CHECK(!r.spriteThumb(QStringLiteral("Mom Normal m_day 01 norm aside"), 300).isNull(), "миниатюра спрайта");
    if (qEnvironmentVariableIsSet("TB_SHOT")) f.save(qEnvironmentVariable("TB_SHOT"));

    // the pseudo-motion: the story's words, the scene's state, the game's script, the preview's player
    {
        const Story mv = parseStory(QStringLiteral(
            "@mod_id mv\n: start\nфон bg kitchen1_1\nпоказать Мама Normal m_day 01 norm aside слева\n"   // 3, 4
            "камера наезд | 960 400\nдвижение Мама вздрог\nМама: Ой.\nдвижение Мама вздрог\n"                // 5, 6, 7, 8
            "движение Мама дрожит\nпогода метель\nплёнка vhs\nМама: Холодно.\n"                             // 9..12
            "камера рывок\nкамера рывок\nфон bg kitchen1_1\nМама: Тихо.\nкамера летит\n"), cat);          // 13..17
        CHECK(mv.problems().size() == 1 && mv.problems().first().line == 17, "движение: одна ошибка — «камера летит»");
        const SceneState a = stateAt(mv, cat, 6);
        CHECK(a.camera == QLatin1String("zoomin") && a.cameraAt == QPoint(960, 400), "камера: наезд в точку");
        CHECK(a.once.value(QStringLiteral("Mom")) == QLatin1String("scare") && a.motions.isEmpty(), "вздрог на своей карточке");
        CHECK(stateAt(mv, cat, 7).once.isEmpty(), "вздрог — один раз");
        const SceneState b = stateAt(mv, cat, 12);
        CHECK(b.motions.value(QStringLiteral("Mom")) == QLatin1String("fear") && b.weather == QLatin1String("blizzard") && b.film == QLatin1String("vhs"),
              "дрожит, метель, плёнка держатся");
        const SceneState e = stateAt(mv, cat, 16);
        CHECK(e.camera.isEmpty() && e.weather.isEmpty() && e.motions.isEmpty() && e.film == QLatin1String("vhs"), "новый фон — чистый кадр, плёнка остаётся");

        Story ok = mv;
        ok.steps.erase(std::remove_if(ok.steps.begin(), ok.steps.end(), [](const Step& s) { return s.kind == Step::Bad; }), ok.steps.end());
        const Compiled cm = compileStory(ok, cat);
        CHECK(cm.ok, "движение собирается");
        CHECK(cm.rpy.contains(QStringLiteral("show layer master at genry_cam_zoomin(0.5, 0.3704), genry_film_none\n")), "камера в скрипте");
        CHECK(cm.rpy.contains(QStringLiteral("show Mom Normal m_day 01 norm aside at genry_left, genry_mv_still, genry_mv_scare\n")), "вздрог в скрипте");
        CHECK(cm.rpy.contains(QStringLiteral("at genry_left, genry_mv_still, genry_mv_scare_b\n")), "второй вздрог подряд — близнец (играет заново)");
        CHECK(cm.rpy.contains(QStringLiteral("at genry_left, genry_mv_fear, genry_mv_none\n")), "дрожит в скрипте");
        CHECK(cm.rpy.contains(QStringLiteral("show genry_tb_blizzard zorder 80 at genry_tb_weather")), "метель в скрипте");
        CHECK(cm.rpy.contains(QStringLiteral("show layer master at genry_cam_zoomin(0.5, 0.3704), genry_film_vhs")), "плёнка поверх камеры");
        CHECK(cm.rpy.contains(QStringLiteral("genry_cam_jerk(0.5, 0.5), genry_film_vhs")) && cm.rpy.contains(QStringLiteral("genry_cam_jerk_b(0.5, 0.5), genry_film_vhs")),
              "рывок дважды");
        CHECK(cm.rpy.contains(QStringLiteral("scene bg kitchen1_1\n    show layer master at genry_cam_none, genry_film_vhs\n")), "новый фон сбрасывает камеру");
        const QString hub = hubScript();
        CHECK(hub.contains(QStringLiteral("transform genry_cam_walk(ax=0.5, ay=0.5):")) && hub.contains(QStringLiteral("transform genry_mv_scare_b:")) &&
                  hub.contains(QStringLiteral("image genry_tb_snow")) && hub.contains(QStringLiteral("genry_film_vhs")),
              "хаб знает движения");

        const MotionDef* zi = motionById(QStringLiteral("zoomin"), true);
        const MotionDef* wk = motionById(QStringLiteral("walk"), true);
        const MotionDef* pl = motionById(QStringLiteral("panl"), true);
        const MotionDef* sc = motionByWord(QStringLiteral("ВЗДРОГ"), false);
        const MotionDef* la = motionByWord(QStringLiteral("смеется"), false);
        CHECK(zi && wk && pl && sc && la, "движения по id и по слову");
        if (zi && wk && pl && sc && la) {
            auto near = [](double x, double y) { return qAbs(x - y) < 0.01; };
            CHECK(near(motionAt(*zi, 0, 0.5, 0.37).value("zoom"), 1.0) && near(motionAt(*zi, 30, 0.5, 0.37).value("zoom"), 1.25) &&
                      near(motionAt(*zi, 5, 0.5, 0.37).value("yalign"), 0.37),
                  "наезд: от 1.0 к 1.25, в свою точку");
            CHECK(near(motionAt(*wk, 0.32).value("yoffset"), 9) && near(motionAt(*wk, 0.64 * 7 + 0.32).value("yoffset"), 9) &&
                      motionAt(*wk, 0.64 * 7 + 0.01).value("yoffset") < 1.0,
                  "шаги: по кругу");
            CHECK(near(motionAt(*pl, 100).value("xalign"), 0.0) && near(motionAt(*pl, 100).value("zoom"), 1.2), "панорама: две дорожки рядом");
            CHECK(near(motionAt(*sc, 0.1).value("yoffset"), 20) && near(motionAt(*sc, 5).value("yoffset"), 0), "вздрог: вниз и назад");
            CHECK(near(motionAt(*la, 0.1).value("yoffset"), 15) && near(motionAt(*la, 0.3 * 11 + 0.1).value("yoffset"), 15), "смех: по кругу с паузой");
            CHECK(hub.contains(QStringLiteral("transform genry_mv_none:")) && hub.contains(QStringLiteral("transform genry_cam_none:")) &&
                      hub.contains(QStringLiteral("transform genry_film_none:")) && hub.contains(QStringLiteral("on replace:")),
                  "хаб: пустые места и переход между местами");
        }
        CHECK(!r.spriteLayer(QStringLiteral("Mom Normal m_day 01 norm aside"), QStringLiteral("genry_left"), 100, QSize(1280, 720)).isNull() &&
                  r.ui(s16, QSize(1280, 720)).size() == QSize(1280, 720),
              "слои кадра");
    }

    // the things of the 90s, the size, the choice against the clock
    {
        setHeadBox([&r](const QString& sprite) { return r.spriteHead(sprite); });
        const Story n = parseStory(QStringLiteral(
            "@mod_id n\n: start\nфон bg kitchen1_1\nпоказать Мама Normal m_day 01 norm aside слева рост 90 плавно\n"      // 3, 4
            "записка Я вижу тебя.\nобъявление Мама | ПРОПАЛА ЖЕНЩИНА | Ушла и не вернулась.\n"                              // 5, 6
            "плёнка телевизор\nплёнка воспоминание\nплёнка нет\n"                                                          // 7, 8, 9
            "выбор время 8\n- Бежать -> start\n- Стоять -> start\nконецвыбора\n"), cat);                                  // 10..13
        for (const Step& s : n.problems()) out() << "  problem line " << s.line << ": " << s.error << "\n";
        CHECK(n.problems().isEmpty(), "90-е без проблем");
        const SceneState a4 = stateAt(n, cat, 4);
        CHECK(a4.sizes.value(QStringLiteral("Mom")) == 90 && a4.sprites.size() == 1 && a4.sprites[0].second == QLatin1String("genry_left"), "рост 90 слева");
        const SceneState a5 = stateAt(n, cat, 5);
        CHECK(a5.paper == QStringLiteral("Я вижу тебя.") && stateAt(n, cat, 6).paper.isEmpty(), "записка на своей карточке");
        const SceneState a6 = stateAt(n, cat, 6);
        CHECK(a6.posterTitle == QStringLiteral("ПРОПАЛА ЖЕНЩИНА") && a6.posterSprite == QLatin1String("Mom Normal m_day 01 norm aside") &&
                  a6.posterText == QStringLiteral("Ушла и не вернулась."),
              "объявление: её фото, заголовок, текст");
        CHECK(stateAt(n, cat, 7).film == QLatin1String("tv") && stateAt(n, cat, 8).film == QLatin1String("memory") && stateAt(n, cat, 9).film.isEmpty(), "плёнка: телевизор, воспоминание");
        const SceneState a10 = stateAt(n, cat, 13);
        CHECK(a10.timed == 8 && a10.choices.size() == 2, "выбор на время");
        const Compiled cn = compileStory(n, cat);
        CHECK(cn.ok, "90-е собираются");
        CHECK(cn.rpy.contains(QStringLiteral("show Mom Normal m_day 01 norm aside at genry_at(-480, 0.9), genry_mv_still, genry_mv_none with dissolve")), "рост в скрипте");
        CHECK(cn.rpy.contains(QStringLiteral("call screen genry_tb_note(u\"Я вижу тебя.\")")), "записка в скрипте");
        CHECK(cn.rpy.contains(QStringLiteral("call screen genry_tb_poster(u\"ПРОПАЛА ЖЕНЩИНА\", u\"Ушла и не вернулась.\", \"Mom Normal m_day 01 norm aside\", (")), "объявление в скрипте");
        const QRect head = r.spriteHead(QStringLiteral("Mom Normal m_day 01 norm aside"));
        CHECK(head.width() > 100 && head.width() == head.height() &&
                  cn.rpy.contains(QStringLiteral("(%1, %2, %3, %4))").arg(head.x()).arg(head.y()).arg(head.width()).arg(head.height())),
              "фото — её голова");
        CHECK(cn.rpy.contains(QStringLiteral("show layer master at genry_cam_none, genry_film_tv\n    show screen genry_tb_film(\"tv\")")), "телевизор в скрипте");
        CHECK(cn.rpy.contains(QStringLiteral("show layer master\n    show screen genry_tb_film(\"memory\")")), "воспоминание в скрипте");
        CHECK(cn.rpy.contains(QStringLiteral("show layer master\n    hide screen genry_tb_film")), "плёнка выключена");
        CHECK(cn.rpy.contains(QStringLiteral("call screen genry_tb_choice([(u\"Бежать\", \"n__start\"), (u\"Стоять\", \"n__start\")], 8)")), "выбор на время в скрипте");
        const QString hub2 = hubScript();
        CHECK(hub2.contains(QStringLiteral("screen genry_tb_note(body):")) && hub2.contains(QStringLiteral("screen genry_tb_poster(title, body, photo, box):")) &&
                  hub2.contains(QStringLiteral("screen genry_tb_choice(options, seconds=0):")) && hub2.contains(QStringLiteral("screen genry_tb_film(kind):")) &&
                  hub2.contains(QStringLiteral("transform genry_at(dx=0, z=1.0):")),
              "хаб: записка, объявление, таймер, плёнка, рост");
        const QImage note = r.ui(a5, QSize(1920, 1080)), post = r.ui(a6, QSize(1920, 1080));
        CHECK(note.pixelColor(960, 600).lightness() > 180 && post.pixelColor(960, 980).lightness() > 180 && post.pixelColor(960, 440).lightness() < 235, "бумага в превью");
        if (qEnvironmentVariableIsSet("TB_SHOT")) {
            note.save(qEnvironmentVariable("TB_SHOT") + QStringLiteral(".note.png"));
            post.save(qEnvironmentVariable("TB_SHOT") + QStringLiteral(".poster.png"));
        }
    }

    out() << "selftest: " << g_pass << " ok, " << g_fail << " failed\n";
    return g_fail ? 1 : 0;
}
