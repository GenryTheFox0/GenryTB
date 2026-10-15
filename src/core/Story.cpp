#include "Story.h"

#include <QCoreApplication>
// a message for the person (Russian, the source): its translation in the app (data/i18n), as it is in tb_cli
#define tbTr(s) QCoreApplication::translate("GenryTB", s)
#include "Catalog.h"
#include "Motion.h"

#include <QRegularExpression>

namespace tb {
namespace {

QString lower(const QString& s)
{
    QString t = s.trimmed().toLower();
    t.replace(QChar(0x0451), QChar(0x0435));       // ё -> е
    return t;
}

// a name a Ren'Py label / variable can carry: Russian spelled in latin, the rest '_'
QString safeName(const QString& s)
{
    static const QHash<QChar, QString> tr = [] {
        QHash<QChar, QString> m;
        const QString ru = QStringLiteral("абвгдеёжзийклмнопрстуфхцчшщъыьэюя");
        const QStringList la = {"a", "b", "v", "g", "d", "e", "e", "zh", "z", "i", "y", "k", "l", "m", "n", "o", "p",
                                "r", "s", "t", "u", "f", "h", "ts", "ch", "sh", "sch", "", "y", "", "e", "yu", "ya"};
        for (int i = 0; i < ru.size(); ++i) m.insert(ru[i], la[i]);
        return m;
    }();
    QString out;
    for (const QChar c : s.trimmed().toLower()) {
        if (tr.contains(c)) out += tr.value(c);
        else if ((c >= QLatin1Char('a') && c <= QLatin1Char('z')) || (c >= QLatin1Char('0') && c <= QLatin1Char('9'))) out += c;
        else out += QLatin1Char('_');
    }
    while (out.contains(QStringLiteral("__"))) out.replace(QStringLiteral("__"), QStringLiteral("_"));
    out = out.trimmed();
    while (out.startsWith(QLatin1Char('_'))) out.remove(0, 1);
    while (out.endsWith(QLatin1Char('_'))) out.chop(1);
    if (out.isEmpty()) out = QStringLiteral("x");
    if (out[0].isDigit()) out.prepend(QLatin1Char('n'));
    return out;
}

const QHash<QString, QString>& transitions()
{
    static const QHash<QString, QString> t = {
        {QStringLiteral("dissolve"), QStringLiteral("dissolve")}, {QStringLiteral("растворение"), QStringLiteral("dissolve")},
        {QStringLiteral("плавно"), QStringLiteral("dissolve")},
        {QStringLiteral("fade"), QStringLiteral("fade")}, {QStringLiteral("затемнение"), QStringLiteral("fade")},
        {QStringLiteral("быстро"), QStringLiteral("Dissolve(0.3)")}, {QStringLiteral("медленно"), QStringLiteral("Dissolve(1.5)")},
        {QStringLiteral("мгновенно"), QString()}, {QStringLiteral("сразу"), QString()}, {QStringLiteral("none"), QString()},
        {QStringLiteral("вспышка"), QStringLiteral("Fade(0.1, 0.0, 0.5, color=\"#fff\")")},
    };
    return t;
}

const QHash<QString, QString>& positions()
{
    static const QHash<QString, QString> p = {
        {QStringLiteral("слева"), QStringLiteral("genry_left")}, {QStringLiteral("влево"), QStringLiteral("genry_left")},
        {QStringLiteral("справа"), QStringLiteral("genry_right")}, {QStringLiteral("вправо"), QStringLiteral("genry_right")},
        {QStringLiteral("центр"), QStringLiteral("genry_center")}, {QStringLiteral("вцентре"), QStringLiteral("genry_center")},
        {QStringLiteral("посередине"), QStringLiteral("genry_center")},
        {QStringLiteral("далеко"), QStringLiteral("genry_far")}, {QStringLiteral("близко"), QStringLiteral("genry_close")},
    };
    return p;
}

// the first word of a command line, if it is one
enum Cmd { None, CBg, CShow, CHide, CVoice, CMusic, CAmb, CSound, CPause, CWindow, CMenu, CMenuEnd, CJump, CSet, CAdd,
           CSub, CIf, CEnd, CShake, CFlash, CCamera, CMove, CWeather, CFilm, CPaper, CPoster, CGame, CReturn, CCode,
           CTitle, CEpilogue, CButton, CStruggle, CCredits, CSuefa, CSnap, CRiddle, CExplore, CExploreEnd,
           CPhone, CPhoneEnd, CAch, CInHand, CPuzzle };
Cmd command(const QString& w)
{
    static const QHash<QString, Cmd> m = {
        {QStringLiteral("фон"), CBg}, {QStringLiteral("сцена"), CBg},
        {QStringLiteral("показать"), CShow}, {QStringLiteral("покажи"), CShow},
        {QStringLiteral("убрать"), CHide}, {QStringLiteral("скрыть"), CHide}, {QStringLiteral("убери"), CHide},
        {QStringLiteral("голос"), CVoice}, {QStringLiteral("озвучка"), CVoice},
        {QStringLiteral("музыка"), CMusic}, {QStringLiteral("фонзвук"), CAmb}, {QStringLiteral("атмосфера"), CAmb},
        {QStringLiteral("звук"), CSound}, {QStringLiteral("пауза"), CPause}, {QStringLiteral("окно"), CWindow},
        {QStringLiteral("выбор"), CMenu}, {QStringLiteral("конецвыбора"), CMenuEnd},
        {QStringLiteral("переход"), CJump}, {QStringLiteral("прыжок"), CJump},
        {QStringLiteral("установить"), CSet}, {QStringLiteral("прибавить"), CAdd}, {QStringLiteral("убавить"), CSub},
        {QStringLiteral("если"), CIf}, {QStringLiteral("конецигры"), CEnd}, {QStringLiteral("конец"), CEnd},
        {QStringLiteral("тряска"), CShake}, {QStringLiteral("вспышка"), CFlash},
        {QStringLiteral("камера"), CCamera}, {QStringLiteral("движение"), CMove}, {QStringLiteral("погода"), CWeather},
        {QStringLiteral("пленка"), CFilm}, {QStringLiteral("кассета"), CFilm},
        {QStringLiteral("записка"), CPaper}, {QStringLiteral("объявление"), CPoster},
        {QStringLiteral("оригинал"), CGame}, {QStringLiteral("вернуться"), CReturn},
        {QStringLiteral("код"), CCode}, {QStringLiteral("код:"), CCode},
        {QStringLiteral("надпись"), CTitle}, {QStringLiteral("титул"), CTitle},
        {QStringLiteral("эпилог"), CEpilogue}, {QStringLiteral("кнопка"), CButton},
        {QStringLiteral("борьба"), CStruggle}, {QStringLiteral("титры"), CCredits},
        {QStringLiteral("суефа"), CSuefa}, {QStringLiteral("камень-ножницы-бумага"), CSuefa},
        {QStringLiteral("снимок"), CSnap}, {QStringLiteral("полароид"), CSnap},
        {QStringLiteral("загадка"), CRiddle}, {QStringLiteral("осмотр"), CExplore}, {QStringLiteral("конецосмотра"), CExploreEnd},
        {QStringLiteral("телефон"), CPhone}, {QStringLiteral("конецтелефона"), CPhoneEnd},
        {QStringLiteral("достижение"), CAch}, {QStringLiteral("ачивка"), CAch},
        {QStringLiteral("предмет"), CInHand}, {QStringLiteral("вкладыш"), CInHand},
        {QStringLiteral("пазл"), CPuzzle}, {QStringLiteral("собрать"), CPuzzle},
    };
    return m.value(lower(w), None);
}

Step bad(int line, const QString& why)
{
    Step s;
    s.kind = Step::Bad;
    s.line = line;
    s.error = why;
    return s;
}

// "Мама Normal m_day 01 norm aside справа dissolve" -> the game's sprite, where, how
bool resolveSprite(const QStringList& words, const Catalog& cat, Step& out, QString* why)
{
    if (words.isEmpty()) {
        *why = QStringLiteral("кого показать?");
        return false;
    }
    // the owner may be two words (Лилия Павловна, Катя кукла)
    QString who;
    int used = 0;
    if (words.size() >= 2) {
        who = cat.spriteWhoFor(words[0] + QLatin1Char(' ') + words[1]);
        if (!who.isEmpty()) used = 2;
    }
    if (who.isEmpty()) {
        who = cat.spriteWhoFor(words[0]);
        used = 1;
    }
    if (who.isEmpty()) {
        *why = QStringLiteral("нет такого персонажа: %1").arg(words[0]);
        return false;
    }
    // the longest run of words that names one of the game's sprites
    QString sprite;
    int end = used;
    for (int k = words.size(); k > used; --k) {
        const QString name = (QStringList{who} + words.mid(used, k - used)).join(QLatin1Char(' '));
        if (cat.sprite(name)) {
            sprite = name;
            end = k;
            break;
        }
    }
    if (sprite.isEmpty()) {
        const auto all = cat.spritesOf(who);
        if (all.isEmpty()) {
            *why = QStringLiteral("у %1 нет спрайтов").arg(cat.ruName(who));
            return false;
        }
        if (words.size() > used) {
            *why = QStringLiteral("у %1 нет спрайта «%2» (выбери в карточке персонажа)")
                       .arg(cat.ruName(who), words.mid(used).join(QLatin1Char(' ')));
            return false;
        }
        sprite = all.first()->name;               // just the name: the one the game shows most
    }
    out.a = sprite;
    const QStringList tail = words.mid(end);
    for (int i = 0; i < tail.size(); ++i) {
        const QString w = tail[i];
        const QString l = lower(w);
        if (l == QStringLiteral("рост") && i + 1 < tail.size()) {            // рост 90: percent of its own size
            bool ok = false;
            const int v = QString(tail[i + 1]).remove(QLatin1Char('%')).toInt(&ok);
            if (!ok) {
                *why = QStringLiteral("рост — число в процентах: рост 90");
                return false;
            }
            out.c = v == 100 ? QString() : QString::number(qBound(50, v, 160));
            ++i;
            continue;
        }
        if (positions().contains(l)) out.b = positions().value(l);
        else if (transitions().contains(l)) out.transition = transitions().value(l);
        else if (QRegularExpression(QStringLiteral("^[a-z_][a-z0-9_]*$")).match(w).hasMatch()) out.b = w;   // the game's own transform
        else {
            *why = QStringLiteral("непонятно «%1» (позиция: слева/справа/центр; переход: плавно/быстро/затемнение)").arg(w);
            return false;
        }
    }
    return true;
}

QString cleanFile(const QString& s, const QString& folder)
{
    QString f = s.trimmed();
    if (f.startsWith(QLatin1Char('"')) && f.endsWith(QLatin1Char('"')) && f.size() > 1) f = f.mid(1, f.size() - 2);
    f.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (!folder.isEmpty() && !f.contains(QLatin1Char('/'))) f = folder + QLatin1Char('/') + f;
    else if (!folder.isEmpty() && !f.startsWith(folder + QLatin1Char('/')) && !f.startsWith(QLatin1String("sounds/")) &&
             !f.startsWith(QLatin1String("music/")) && !f.startsWith(QLatin1String("voice/")))
        f = folder + QLatin1Char('/') + f;
    if (!f.endsWith(QLatin1String(".ogg"), Qt::CaseInsensitive) && !f.endsWith(QLatin1String(".mp3"), Qt::CaseInsensitive) &&
        !f.endsWith(QLatin1String(".wav"), Qt::CaseInsensitive))
        f += QStringLiteral(".ogg");
    return f;
}

} // namespace

QString renpyTransition(const QString& t)
{
    return transitions().value(lower(t), t);
}

QString labelName(const QString& modId, const QString& scene)
{
    return safeName(modId) + QStringLiteral("__") + safeName(scene);
}

QString varName(const QString& modId, const QString& var)
{
    return safeName(modId) + QLatin1Char('_') + safeName(var);
}

QString transitionWord(const QString& renpy)
{
    if (renpy.isEmpty()) return {};
    if (renpy == QLatin1String("dissolve")) return QStringLiteral("плавно");
    if (renpy == QLatin1String("fade")) return QStringLiteral("затемнение");
    if (renpy == QLatin1String("Dissolve(0.3)")) return QStringLiteral("быстро");
    if (renpy == QLatin1String("Dissolve(1.5)")) return QStringLiteral("медленно");
    if (renpy.startsWith(QLatin1String("Fade(0.1"))) return QStringLiteral("вспышка");
    return renpy;
}

QString positionWord(const QString& transform)
{
    if (transform == QLatin1String("genry_left")) return QStringLiteral("слева");
    if (transform == QLatin1String("genry_right")) return QStringLiteral("справа");
    if (transform == QLatin1String("genry_center")) return QStringLiteral("центр");
    if (transform == QLatin1String("genry_far")) return QStringLiteral("далеко");
    if (transform == QLatin1String("genry_close")) return QStringLiteral("близко");
    return transform;
}

bool isCommandWord(const QString& word)
{
    return command(word) != None;
}

QPoint spotPlace(const QString& place, int i, int n)
{
    const QStringList p = place.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (p.size() == 2) return QPoint(qBound(0, p[0].toInt(), 1920), qBound(0, p[1].toInt(), 1080));
    return QPoint(1920 * (i + 1) / (n + 1), 520);
}

QString safeId(const QString& name)
{
    return safeName(name);
}

QVector<Step> Story::problems() const
{
    QVector<Step> out;
    for (const Step& s : steps)
        if (s.kind == Step::Bad) out.push_back(s);
    return out;
}

// суефа's hands by who plays: "" - the game's own (Ромка's), the set's folder in data/assets/hands, "?" - none such
QString suefaHands(const QString& who)
{
    static const QPair<const char*, const char*> sets[] = {
        {"ромка", ""}, {"рома", ""}, {"катя", "katya"}, {"полина", "polina"}, {"оля", "olya"}, {"бяша", "byasha"},
        {"мама", "mama"}, {"папа", "papa"}, {"отец", "papa"}, {"девушка", "devushka"}, {"мужчина", "muzhchina"}, {"бабушка", "babushka"},
        {"алиса", "alisa"}, {"тихонов", "tikhonov"}, {"семён", "semyon"}, {"семен", "semyon"},
        {"учительница", "uchitelnica"}, {"харитон", "hariton"}, {"дед", "hariton"}, {"дед мороз", "dedmoroz"},
        {"врач", "vrach"}, {"киллер", "killer"}, {"лиса", "lisa"}, {"волчик", "volchik"}, {"медвежутка", "medvezhutka"},
        {"зайчик", "zaichik"}, {"скелет", "skelet"}};
    const QString w = who.trimmed().toLower();
    if (w.isEmpty()) return QString();
    for (const auto& x : sets)
        if (w == QString::fromUtf8(x.first) || (*x.second && w == QLatin1String(x.second))) return QString::fromLatin1(x.second);
    return QStringLiteral("?");
}

// суефа's drawn places (data/assets/suefa_bg): "дом ночь" -> its picture in the game; "" - not one of them
QString suefaPlace(const QString& where)
{
    static const QPair<const char*, const char*> places[] = {
        {"дом", "dom"}, {"школа", "shkola"}, {"двор", "dvor"}, {"гаражи", "garazhi"}, {"лес", "les"},
        {"деревня", "derevnya"}, {"поле", "pole"}};
    static const QPair<const char*, const char*> times[] = {{"день", "den"}, {"вечер", "vecher"}, {"ночь", "noch"}};
    const QStringList w = where.trimmed().toLower().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (w.isEmpty() || w.size() > 2) return QString();
    for (const auto& p : places) {
        if (w[0] != QString::fromUtf8(p.first)) continue;
        QString t = QStringLiteral("den");
        if (w.size() == 2) {
            t.clear();
            for (const auto& x : times)
                if (w[1] == QString::fromUtf8(x.first)) t = QString::fromLatin1(x.second);
            if (t.isEmpty()) return QString();
        }
        return QStringLiteral("mods/genry_assets/suefa_bg/%1_%2.jpg").arg(QString::fromLatin1(p.second), t);
    }
    return QString();
}

Story parseStory(const QString& text, const Catalog& cat)
{
    Story st;
    const QStringList lines = text.split(QLatin1Char('\n'));
    int menu = -1;                           // the выбор being read (an index: steps grows meanwhile)
    QHash<QString, QString> extra;           // unknown speaker name -> var
    QVector<int> insertLines, replaceLines;  // where each @вставка / @замена was written
    int explore = -1;                        // the осмотр being read (an index, as menu)
    int phone = -1;                          // the телефон being read
    for (int i = 0; i < lines.size(); ++i) {
        const int ln = i + 1;
        QString raw = lines[i];
        raw.remove(QLatin1Char('\r'));
        const QString t = raw.trimmed();
        if (t.isEmpty() || t.startsWith(QLatin1Char('#')) || t.startsWith(QLatin1String("//"))) continue;
        if (t.startsWith(QLatin1Char('@'))) {
            const int sp = t.indexOf(QLatin1Char(' '));
            const QString key = (sp < 0 ? t : t.left(sp)).mid(1).toLower(), val = sp < 0 ? QString() : t.mid(sp + 1).trimmed();
            if (key == QLatin1String("mod_id")) st.id = val;
            else if (key == QLatin1String("mod_name")) st.name = val;
            else if (key == QLatin1String("author")) st.author = val;
            else if (key == QLatin1String("18+") || key == QLatin1String("18") || key == QLatin1String("adult") || key == QStringLiteral("взрослый"))
                st.adult = true;
            else if (key == QLatin1String("description") || key == QStringLiteral("описание")) st.description = val;
            else if (key == QStringLiteral("вставка") || key == QLatin1String("insert") ||
                     key == QStringLiteral("замена") || key == QLatin1String("replace")) {
                static const QRegularExpression hook(QStringLiteral("^([A-Za-z_][A-Za-z0-9_.]*)\\s*->\\s*(\\S+)$"));
                const auto m = hook.match(val);
                if (!m.hasMatch()) {
                    st.steps.push_back(bad(ln, tbTr("пишется так: @%1 метка_игры -> моя_сцена").arg(key)));
                    continue;
                }
                const bool ins = key == QStringLiteral("вставка") || key == QLatin1String("insert");
                (ins ? st.inserts : st.replaces).push_back({m.captured(1), m.captured(2)});
                (ins ? insertLines : replaceLines) << ln;
            }
            continue;
        }
        if (phone >= 0) {                             // inside телефон: "- 31677 -> сцена" or "- неправильно -> сцена"
            if (command(t.section(QLatin1Char(' '), 0, 0)) == CPhoneEnd) {
                if (st.steps[phone].choices.isEmpty()) st.steps.push_back(bad(st.steps[phone].line, tbTr("в телефоне нет ни одного номера")));
                phone = -1;
                continue;
            }
            static const QRegularExpression num(QStringLiteral("^-\\s*(\\d{1,10}|неправильно|не\\s+тот)\\s*->\\s*(\\S+)\\s*$"));
            const auto m = num.match(t);
            if (!m.hasMatch()) {
                st.steps.push_back(bad(ln, tbTr("номер телефона: - 31677 -> сцена   или   - неправильно -> сцена")));
                continue;
            }
            Step& ph = st.steps[phone];
            if (m.captured(1).at(0).isDigit()) {
                if (!ph.choices.isEmpty() && ph.choices.first().size() != m.captured(1).size()) {
                    st.steps.push_back(bad(ln, tbTr("все номера одной длины: как %1").arg(ph.choices.first())));
                    continue;
                }
                ph.choices << m.captured(1);
                ph.targets << m.captured(2);
            } else {
                ph.c = m.captured(2);
            }
            continue;
        }
        if (explore >= 0) {                           // inside осмотр: "- Плита | x y | Описание" or "- Уйти | x y -> сцена"
            if (command(t.section(QLatin1Char(' '), 0, 0)) == CExploreEnd) {
                if (st.steps[explore].choices.isEmpty()) st.steps.push_back(bad(st.steps[explore].line, tbTr("в осмотре нет ни одной точки")));
                explore = -1;
                continue;
            }
            static const QRegularExpression look(QStringLiteral("^-\\s*([^|]+?)\\s*\\|\\s*(-?\\d+)\\s+(-?\\d+)\\s*(?:\\|\\s*(.+?)|->\\s*(\\S+))\\s*$"));
            const auto m = look.match(t);
            if (!m.hasMatch()) {
                st.steps.push_back(bad(ln, tbTr("точка осмотра: - Плита | 900 600 | Описание   или   - Уйти | 1700 900 -> сцена")));
                continue;
            }
            st.steps[explore].choices << m.captured(1);
            st.steps[explore].places << m.captured(2) + QLatin1Char(' ') + m.captured(3);
            st.steps[explore].notes << m.captured(4);
            st.steps[explore].targets << m.captured(5);
            continue;
        }
        if (menu >= 0) {                              // inside выбор: "- Текст -> сцена" until конецвыбора
            if (command(t.section(QLatin1Char(' '), 0, 0)) == CMenuEnd) {
                if (st.steps[menu].choices.isEmpty()) st.steps.push_back(bad(st.steps[menu].line, tbTr("в выборе нет вариантов")));
                menu = -1;
                continue;
            }
            static const QRegularExpression ch(QStringLiteral("^-\\s*(.+?)\\s*->\\s*([^|]+?)\\s*(?:\\|\\s*(-?\\d+)\\s+(-?\\d+)\\s*)?$"));
            const auto m = ch.match(t);
            if (!m.hasMatch()) {
                st.steps.push_back(bad(ln, tbTr("вариант выбора пишется так: - Текст -> сцена")));
                continue;
            }
            st.steps[menu].choices << m.captured(1);
            st.steps[menu].targets << m.captured(2).trimmed();
            st.steps[menu].places << (m.captured(3).isEmpty() ? QString() : m.captured(3) + QLatin1Char(' ') + m.captured(4));
            continue;
        }
        if (t.startsWith(QLatin1Char(':'))) {
            Step s;
            s.kind = Step::Label;
            s.line = ln;
            s.a = t.mid(1).trimmed();
            if (s.a.isEmpty()) {
                st.steps.push_back(bad(ln, tbTr("у сцены нет названия")));
                continue;
            }
            if (st.labels.contains(s.a)) {
                st.steps.push_back(bad(ln, tbTr("сцена «%1» уже есть").arg(s.a)));
                continue;
            }
            st.labels << s.a;
            st.steps.push_back(s);
            continue;
        }
        const QStringList words = t.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        const Cmd cmd = command(words[0]);
        const QString rest = t.mid(words[0].size()).trimmed();
        Step s;
        s.line = ln;
        switch (cmd) {
        case CBg: {
            if (words.size() < 2) { st.steps.push_back(bad(ln, tbTr("какой фон?"))); continue; }
            // the name may be two words (bg kitchen1_1); a last word that is a transition is the transition
            QStringList w = words.mid(1);
            if (w.size() > 1 && transitions().contains(lower(w.last()))) s.transition = transitions().value(lower(w.takeLast()));
            QString name = w.join(QLatin1Char(' '));
            if (lower(name) == QStringLiteral("черный") || lower(name) == QStringLiteral("чёрный")) name = QStringLiteral("bg_black");
            if (lower(name) == QStringLiteral("белый")) name = QStringLiteral("bg_white");
            // a picture file of the game (images/...png) is a background too; layers with + (as the endings are)
            auto isFile = [](const QString& n) {
                const QString l = n.trimmed().toLower();
                return l.startsWith(QLatin1String("images/")) &&
                       (l.endsWith(QLatin1String(".png")) || l.endsWith(QLatin1String(".jpg")) || l.endsWith(QLatin1String(".webp")));
            };
            bool files = name.contains(QLatin1Char('+')) || isFile(name);
            if (files) {
                QStringList layers;
                for (const QString& x : name.split(QLatin1Char('+'))) {
                    if (!isFile(x)) { files = false; break; }
                    layers << x.trimmed();
                }
                if (files) name = layers.join(QLatin1Char('+'));
            }
            if (!files && !cat.background(name)) { st.steps.push_back(bad(ln, tbTr("нет такого фона: %1").arg(name))); continue; }
            s.kind = Step::Background;
            s.a = name;
            break;
        }
        case CShow: {
            QString why;
            if (!resolveSprite(words.mid(1), cat, s, &why)) { st.steps.push_back(bad(ln, why)); continue; }
            s.kind = Step::Show;
            break;
        }
        case CHide: {
            if (words.size() < 2) { st.steps.push_back(bad(ln, tbTr("кого убрать?"))); continue; }
            QStringList w = words.mid(1);
            if (w.size() > 1 && transitions().contains(lower(w.last()))) s.transition = transitions().value(lower(w.takeLast()));
            const QString who = cat.spriteWhoFor(w.join(QLatin1Char(' ')));
            if (who.isEmpty()) { st.steps.push_back(bad(ln, tbTr("нет такого персонажа: %1").arg(w.join(QLatin1Char(' '))))); continue; }
            s.kind = Step::Hide;
            s.a = who;
            break;
        }
        case CVoice:
            if (rest.isEmpty()) { st.steps.push_back(bad(ln, tbTr("какой голос?"))); continue; }
            s.kind = Step::Voice;
            s.a = cleanFile(rest, QStringLiteral("voice"));
            break;
        case CMusic:
        case CAmb:
        case CSound: {
            if (rest.isEmpty()) { st.steps.push_back(bad(ln, tbTr("что играть?"))); continue; }
            s.kind = cmd == CMusic ? Step::Music : cmd == CAmb ? Step::Ambience : Step::Sound;
            const QString l = lower(rest);
            if (l == QStringLiteral("стоп") || l == QLatin1String("stop") || l == QStringLiteral("тишина")) {
                s.a.clear();
            } else if (cmd == CMusic) {
                s.a = cat.musicFile(rest);
                if (s.a.isEmpty()) s.a = cleanFile(rest, QStringLiteral("music"));
            } else {
                s.a = cleanFile(rest, QStringLiteral("sounds"));
            }
            break;
        }
        case CPause: {
            bool ok = false;
            const double sec = rest.isEmpty() ? 1.0 : QString(rest).replace(QLatin1Char(','), QLatin1Char('.')).toDouble(&ok);
            if (!rest.isEmpty() && !ok) { st.steps.push_back(bad(ln, tbTr("пауза в секундах: пауза 1.5"))); continue; }
            s.kind = Step::Pause;
            s.a = QString::number(sec);
            break;
        }
        case CWindow:
            s.kind = Step::Window;
            s.a = lower(rest).startsWith(QStringLiteral("скры")) || lower(rest) == QLatin1String("hide") ? QStringLiteral("hide")
                                                                                                        : QStringLiteral("show");
            break;
        case CMenu:
            s.kind = Step::Menu;
            if (!rest.isEmpty()) {
                const QString how = lower(rest);
                if (how == QStringLiteral("подсветка") || how == QStringLiteral("пятна") || how == QStringLiteral("на картинке")) {
                    s.a = QStringLiteral("spots");
                } else if (how.startsWith(QStringLiteral("время")) || how.startsWith(QStringLiteral("таймер"))) {
                    // выбор время 5: against the clock (time out - the last one)
                    bool ok = false;
                    const double sec = how.section(QLatin1Char(' '), 1).replace(QLatin1Char(','), QLatin1Char('.')).toDouble(&ok);
                    s.a = QStringLiteral("timed");
                    s.b = QString::number(ok ? qBound(1.0, sec, 60.0) : 5.0);
                } else {
                    st.steps.push_back(bad(ln, tbTr("выбор бывает обычный, «выбор подсветка» и «выбор время 5»")));
                    continue;
                }
            }
            st.steps.push_back(s);
            menu = int(st.steps.size()) - 1;
            continue;
        case CMenuEnd:
            st.steps.push_back(bad(ln, tbTr("конецвыбора без выбора")));
            continue;
        case CJump:
            if (rest.isEmpty()) { st.steps.push_back(bad(ln, tbTr("куда переход?"))); continue; }
            s.kind = Step::Jump;
            s.target = rest;
            break;
        case CSet:
        case CAdd:
        case CSub: {
            if (words.size() != 3) { st.steps.push_back(bad(ln, tbTr("так: %1 переменная 1").arg(words[0]))); continue; }
            bool ok = false;
            words[2].toDouble(&ok);
            if (!ok) { st.steps.push_back(bad(ln, tbTr("значение — число"))); continue; }
            s.kind = cmd == CSet ? Step::Set : Step::Add;
            s.a = words[1];
            s.b = cmd == CSub ? QStringLiteral("-") + words[2] : words[2];
            if (!st.vars.contains(s.a)) st.vars << s.a;
            break;
        }
        case CIf: {
            static const QRegularExpression re(QStringLiteral("^(\\S+)\\s*(>=|<=|==|!=|>|<|=)\\s*(-?[0-9.]+)\\s*->\\s*(\\S.*)$"));
            const auto m = re.match(rest);
            if (!m.hasMatch()) { st.steps.push_back(bad(ln, tbTr("так: если смелость >= 2 -> сцена"))); continue; }
            s.kind = Step::If;
            s.a = m.captured(1);
            s.b = m.captured(2) == QLatin1String("=") ? QStringLiteral("==") : m.captured(2);
            s.c = m.captured(3);
            s.target = m.captured(4).trimmed();
            if (!st.vars.contains(s.a)) st.vars << s.a;
            break;
        }
        case CEnd: s.kind = Step::End; break;
        case CGame: {
            static const QRegularExpression gl(QStringLiteral("^[A-Za-z_][A-Za-z0-9_.]*$"));
            if (!gl.match(rest).hasMatch()) { st.steps.push_back(bad(ln, tbTr("так: оригинал метка_игры (латиницей, как в скриптах игры)"))); continue; }
            s.kind = Step::GameJump;
            s.target = rest;
            break;
        }
        case CReturn: s.kind = Step::Return; break;
        case CSuefa: {
            // суефа [кто] [| где] -> выиграл | проиграл
            static const QRegularExpression re(QStringLiteral("^(.*?)\\s*->\\s*([^|\\s]+)\\s*\\|\\s*(\\S+)\\s*$"));
            const auto m = re.match(rest);
            if (!m.hasMatch()) { st.steps.push_back(bad(ln, tbTr("так: суефа Катя -> выиграл | проиграл"))); continue; }
            const QString who = m.captured(1).section(QLatin1Char('|'), 0, 0).trimmed();
            const QString where = m.captured(1).section(QLatin1Char('|'), 1).trimmed();
            const QString hands = suefaHands(who);
            if (hands == QLatin1String("?")) {
                st.steps.push_back(bad(ln, tbTr("нет таких рук: %1 (есть: Ромка, Катя, Полина, Оля, Бяша, Алиса, Семён, Мама, Папа, Учительница, Тихонов, Харитон, Дед Мороз, Врач, Киллер, Девушка, Мужчина, Бабушка, Лиса, Волчик, Медвежутка, Зайчик, Скелет)").arg(who)));
                continue;
            }
            QString place;                                  // "" here, "field", a drawn place, a background or a file
            const QString drawn = suefaPlace(where);
            if (where.compare(QStringLiteral("поле"), Qt::CaseInsensitive) == 0) {
                place = QStringLiteral("field");
            } else if (!drawn.isEmpty()) {
                place = drawn;
            } else if (!where.isEmpty() && where.compare(QStringLiteral("здесь"), Qt::CaseInsensitive) != 0) {
                const QString l = where.toLower();
                if (!cat.background(where) && !(l.startsWith(QLatin1String("images/")) && l.contains(QLatin1Char('.')))) {
                    st.steps.push_back(bad(ln, tbTr("нет такого места: %1 (есть: здесь, дом, школа, двор, гаражи, лес, деревня, поле + день/вечер/ночь, или фон игры)").arg(where)));
                    continue;
                }
                place = where;
            }
            s.kind = Step::Suefa;
            s.a = hands;
            s.b = place;
            s.target = m.captured(2);
            s.c = m.captured(3);
            break;
        }
        case CPuzzle: {
            // пазл картинка [| 3x3] -> сцена
            static const QRegularExpression re(QStringLiteral("^(.+?)\\s*(?:\\|\\s*([2-5])\\s*[xх×]\\s*([2-5])\\s*)?->\\s*(\\S+)\\s*$"));
            const auto m = re.match(rest);
            if (!m.hasMatch()) { st.steps.push_back(bad(ln, tbTr("так: пазл images/end/kaite/bg.png | 3x3 -> собрал"))); continue; }
            QStringList layers;
            bool okPic = true;
            for (const QString& x : m.captured(1).split(QLatin1Char('+'))) {
                const QString t = x.trimmed(), l = t.toLower();
                if (!cat.background(t) && !(l.startsWith(QLatin1String("images/")) && l.contains(QLatin1Char('.')))) okPic = false;
                layers << t;
            }
            if (!okPic) { st.steps.push_back(bad(ln, tbTr("так: пазл images/end/kaite/bg.png | 3x3 -> собрал"))); continue; }
            s.kind = Step::Puzzle;
            s.a = layers.join(QStringLiteral(" + "));
            s.b = m.captured(2).isEmpty() ? QStringLiteral("3x3") : m.captured(2) + QLatin1Char('x') + m.captured(3);
            s.target = m.captured(4);
            break;
        }
        case CInHand: {
            // предмет картинка [| подпись]: a picture file of the game or one of its backgrounds
            QString pic = rest.section(QLatin1Char('|'), 0, 0).trimmed();
            bool okPic = !pic.isEmpty();
            QStringList layers;
            for (const QString& x : pic.split(QLatin1Char('+'))) {
                const QString t = x.trimmed(), l = t.toLower();
                if (!cat.background(t) && !(l.startsWith(QLatin1String("images/")) && l.contains(QLatin1Char('.')))) okPic = false;
                layers << t;
            }
            pic = layers.join(QStringLiteral(" + "));
            if (!okPic) {
                st.steps.push_back(bad(ln, tbTr("так: предмет images/end/kaite/chara.png | Подпись (картинка игры или её фон)")));
                continue;
            }
            s.kind = Step::InHand;
            s.a = pic;
            s.b = rest.section(QLatin1Char('|'), 1).trimmed();
            break;
        }
        case CAch: {
            const QString title = rest.section(QLatin1Char('|'), 0, 0).trimmed();
            if (title.isEmpty()) { st.steps.push_back(bad(ln, tbTr("так: достижение Спаситель | Катя жива"))); continue; }
            s.kind = Step::Achievement;
            s.a = title;
            s.b = rest.section(QLatin1Char('|'), 1).trimmed();
            break;
        }
        case CSnap:
            s.kind = Step::Snapshot;
            s.a = rest;
            break;
        case CRiddle: {
            static const QRegularExpression re(QStringLiteral("^(.+?)\\|\\s*(.+?)\\s*->\\s*([^|\\s]+)\\s*\\|\\s*(\\S+)\\s*$"));
            const auto m = re.match(rest);
            if (!m.hasMatch()) { st.steps.push_back(bad(ln, tbTr("так: загадка Вопрос | ответ/другой ответ -> верно | неверно"))); continue; }
            s.kind = Step::Riddle;
            s.a = m.captured(1).trimmed();
            s.b = m.captured(2).trimmed();
            s.target = m.captured(3);
            s.c = m.captured(4);
            break;
        }
        case CExplore:
            s.kind = Step::Explore;
            s.line = ln;
            st.steps.push_back(s);
            explore = int(st.steps.size()) - 1;
            continue;
        case CExploreEnd:
            st.steps.push_back(bad(ln, tbTr("конецосмотра без осмотра")));
            continue;
        case CPhone:
            s.kind = Step::Phone;
            s.line = ln;
            st.steps.push_back(s);
            phone = int(st.steps.size()) - 1;
            continue;
        case CPhoneEnd:
            st.steps.push_back(bad(ln, tbTr("конецтелефона без телефона")));
            continue;
        case CTitle: {
            // надпись День пятый | 3
            const QString text = rest.section(QLatin1Char('|'), 0, 0).trimmed();
            if (text.isEmpty()) { st.steps.push_back(bad(ln, tbTr("так: надпись День пятый | 3"))); continue; }
            bool ok = true;
            const QString sec = rest.section(QLatin1Char('|'), 1).trimmed();
            const double v = sec.isEmpty() ? 3.0 : sec.toDouble(&ok);
            if (!ok || v <= 0) { st.steps.push_back(bad(ln, tbTr("секунды — число: надпись День пятый | 3"))); continue; }
            s.kind = Step::Title;
            s.a = text;
            s.b = QString::number(v);
            break;
        }
        case CEpilogue: {
            // эпилог Текст [| Кнопка]    or    эпилог картинка | ЗАГОЛОВОК | Текст [| Кнопка]
            if (rest.isEmpty()) { st.steps.push_back(bad(ln, tbTr("так: эпилог текст | кнопка"))); continue; }
            QStringList part;
            for (const QString& x : rest.split(QLatin1Char('|'))) part << x.trimmed();
            auto isArt = [&](const QString& a) {
                if (a.isEmpty()) return false;
                for (const QString& layer : a.split(QLatin1Char('+'))) {
                    const QString l = layer.trimmed();
                    if (!cat.background(l) && !(l.startsWith(QLatin1String("images/")) && l.contains(QLatin1Char('.')))) return false;
                }
                return true;
            };
            s.kind = Step::Epilogue;
            if (part.size() >= 3 && isArt(part[0])) {
                QStringList layers;
                for (const QString& layer : part[0].split(QLatin1Char('+'))) layers << layer.trimmed();
                s.transition = layers.join(QLatin1Char('+'));
                s.c = part[1];
                s.a = part[2];
                s.b = part.value(3);
            } else {
                s.a = part[0];
                s.b = part.value(1);
            }
            if (s.a.isEmpty()) { st.steps.push_back(bad(ln, tbTr("эпилог без текста"))); continue; }
            break;
        }
        case CButton:
            s.kind = Step::Button;
            s.a = rest.isEmpty() ? QStringLiteral("ДАЛЬШЕ") : rest;
            break;
        case CStruggle: {
            static const QRegularExpression re(QStringLiteral("^(.+?)\\|\\s*([0-9.]+)\\s*->\\s*([^|\\s]+)\\s*\\|\\s*(\\S+)\\s*$"));
            const auto m = re.match(rest);
            if (!m.hasMatch()) { st.steps.push_back(bad(ln, tbTr("так: борьба Тяни! | 6 -> вытащил | не_успел"))); continue; }
            s.kind = Step::Struggle;
            s.a = m.captured(1).trimmed();
            s.b = m.captured(2);
            s.target = m.captured(3);
            s.c = m.captured(4);
            break;
        }
        case CCredits: {
            QStringList lines;
            for (const QString& x : rest.split(QLatin1Char('|'))) lines << x.trimmed();
            if (rest.trimmed().isEmpty()) { st.steps.push_back(bad(ln, tbTr("так: титры Сценарий: Генри | Спасибо, что играли"))); continue; }
            s.kind = Step::Credits;
            s.a = lines.join(QLatin1Char('\n'));
            break;
        }
        case CCode: {
            QString code = rest;
            if (code.startsWith(QLatin1Char(':'))) code = code.mid(1).trimmed();
            if (code.isEmpty()) { st.steps.push_back(bad(ln, tbTr("код: строка на питоне"))); continue; }
            s.kind = Step::Code;
            s.a = code;
            break;
        }
        case CCamera: {
            // камера наезд | 960 400 - the move, and (for some) the point of the picture it looks at
            const QString how = rest.section(QLatin1Char('|'), 0, 0).trimmed();
            const QStringList at = rest.section(QLatin1Char('|'), 1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
            const QString l = lower(how);
            s.kind = Step::Camera;
            if (l == QStringLiteral("стоп") || l == QStringLiteral("нет") || l.isEmpty()) {
                s.a.clear();
            } else if (const MotionDef* m = motionByWord(how, true)) {
                s.a = m->id;
                if (m->point && at.size() == 2) s.b = QStringLiteral("%1 %2").arg(qBound(0, at[0].toInt(), 1920)).arg(qBound(0, at[1].toInt(), 1080));
            } else {
                st.steps.push_back(bad(ln, tbTr("камера так не умеет: «%1» (наезд, отъезд, крупно, рывок, шаги, бег, дыхание, качка, дрожь, сердце, влево, вправо, стоп)").arg(how)));
                continue;
            }
            break;
        }
        case CMove: {
            // движение Мама вздрог - the last word is the move, before it who
            if (words.size() < 3) { st.steps.push_back(bad(ln, tbTr("так: движение Мама вздрог"))); continue; }
            const QString who = cat.spriteWhoFor(words.mid(1, words.size() - 2).join(QLatin1Char(' ')));
            if (who.isEmpty()) { st.steps.push_back(bad(ln, tbTr("нет такого персонажа: %1").arg(words.mid(1, words.size() - 2).join(QLatin1Char(' '))))); continue; }
            const QString l = lower(words.last());
            s.kind = Step::Move;
            s.a = who;
            if (l == QStringLiteral("стоп") || l == QStringLiteral("нет")) {
                s.b.clear();
            } else if (const MotionDef* m = motionByWord(words.last(), false)) {
                s.b = m->id;
            } else {
                st.steps.push_back(bad(ln, tbTr("такого движения нет: «%1» (выбери в карточке)").arg(words.last())));
                continue;
            }
            break;
        }
        case CWeather: {
            const QString l = lower(rest);
            s.kind = Step::Weather;
            if (l == QStringLiteral("снег")) s.a = QStringLiteral("snow");
            else if (l == QStringLiteral("метель") || l == QStringLiteral("вьюга")) s.a = QStringLiteral("blizzard");
            else if (l == QStringLiteral("нет") || l == QStringLiteral("стоп") || l == QStringLiteral("ясно")) s.a.clear();
            else { st.steps.push_back(bad(ln, tbTr("погода бывает: снег, метель, нет"))); continue; }
            break;
        }
        case CFilm: {
            const QString l = lower(rest);
            s.kind = Step::Film;
            if (l == QLatin1String("vhs") || l == QStringLiteral("кассета") || l == QStringLiteral("вхс")) s.a = QStringLiteral("vhs");
            else if (l == QStringLiteral("помехи") || l == QStringLiteral("шум")) s.a = QStringLiteral("noise");
            else if (l == QStringLiteral("воспоминание") || l == QStringLiteral("память")) s.a = QStringLiteral("memory");
            else if (l == QStringLiteral("телевизор") || l == QStringLiteral("тв")) s.a = QStringLiteral("tv");
            else if (l == QStringLiteral("глазок")) s.a = QStringLiteral("peephole");
            else if (l == QStringLiteral("нет") || l == QStringLiteral("стоп")) s.a.clear();
            else { st.steps.push_back(bad(ln, tbTr("плёнка бывает: vhs, помехи, воспоминание, телевизор, глазок, нет"))); continue; }
            break;
        }
        case CPaper:
            if (rest.isEmpty()) { st.steps.push_back(bad(ln, tbTr("что написано в записке?"))); continue; }
            s.kind = Step::Paper;
            s.a = rest;
            break;
        case CPoster: {
            // объявление Алиса | ПРОПАЛА ДЕВОЧКА | Ушла из дома и не вернулась.
            // (the photo: the one named - объявление Катя Kawai m_day 00 good ahead 02 | ... - else as she stands)
            const QStringList part = rest.split(QLatin1Char('|'));
            const QStringList pw = part.value(0).split(QLatin1Char(' '), Qt::SkipEmptyParts);
            QString who = cat.spriteWhoFor(part.value(0).trimmed());
            QString photo;
            if (who.isEmpty() && pw.size() > 1) {
                Step probe;
                QString why;
                if (resolveSprite(pw, cat, probe, &why)) {
                    photo = probe.a;
                    who = photo.section(QLatin1Char(' '), 0, 0);
                }
            }
            if (who.isEmpty()) { st.steps.push_back(bad(ln, tbTr("так: объявление Алиса | ПРОПАЛА ДЕВОЧКА | текст"))); continue; }
            s.kind = Step::Poster;
            s.a = who;
            s.transition = photo;
            s.b = part.value(1).trimmed();
            s.c = QStringList(part.mid(2)).join(QLatin1Char('|')).trimmed();
            if (s.b.isEmpty()) s.b = QStringLiteral("ПРОПАЛ РЕБЁНОК");
            break;
        }
        case CShake: s.kind = Step::Shake; break;
        case CFlash: s.kind = Step::Flash; break;
        case None: {
            // a line: "Имя: текст" (the name has no spaces past two words), else the narrator
            static const QRegularExpression say(QStringLiteral("^([^:\"«]{1,40}?)\\s*:\\s*(.+)$"));
            const auto m = say.match(t);
            s.kind = Step::Say;
            if (m.hasMatch()) {
                const QString name = m.captured(1).trimmed();
                s.b = m.captured(2).trimmed();
                const QString l = lower(name);
                if (l == QStringLiteral("мысли") || l == QStringLiteral("антон (мысли)") || l == QStringLiteral("думаю")) {
                    s.a = QStringLiteral("Ant_m");
                } else if (l == QStringLiteral("автор") || l == QStringLiteral("рассказчик")) {
                    s.a.clear();
                } else if (const CharacterDef* c = cat.speaker(name)) {
                    s.a = c->var;
                } else {
                    if (!extra.contains(name)) {
                        const QString v = QStringLiteral("chr_%1").arg(extra.size() + 1);
                        extra.insert(name, v);
                        st.extraSpeakers.push_back({name, v});
                    }
                    s.a = QStringLiteral("@") + extra.value(name);      // the mod's own (prefixed when compiled)
                    s.c = name;
                }
            } else {
                s.b = t;
                if (s.b.size() > 1 && (s.b.startsWith(QLatin1Char('"')) && s.b.endsWith(QLatin1Char('"'))))
                    s.b = s.b.mid(1, s.b.size() - 2);
            }
            break;
        }
        }
        st.steps.push_back(s);
    }
    if (menu >= 0) st.steps.push_back(bad(st.steps[menu].line, tbTr("выбор без конецвыбора")));
    if (explore >= 0) st.steps.push_back(bad(st.steps[explore].line, tbTr("осмотр без конецосмотра")));
    if (phone >= 0) st.steps.push_back(bad(st.steps[phone].line, tbTr("телефон без конецтелефона")));
    // where the story jumps must exist (gathered first: steps is not grown while walked)
    QVector<Step> missing;
    for (const Step& s : st.steps) {
        QStringList to;
        if (s.kind == Step::Jump || s.kind == Step::If) to << s.target;
        if (s.kind == Step::Struggle || s.kind == Step::Suefa || s.kind == Step::Riddle) to << s.target << s.c;
        if (s.kind == Step::Puzzle) to << s.target;
        if (s.kind == Step::Explore)
            for (const QString& x : s.targets)
                if (!x.isEmpty()) to << x;
        if (s.kind == Step::Menu) to = s.targets;
        if (s.kind == Step::Phone) {
            to = s.targets;
            if (!s.c.isEmpty()) to << s.c;
        }
        for (const QString& x : to)
            if (!st.labels.contains(x)) missing.push_back(bad(s.line, tbTr("нет сцены «%1»").arg(x)));
    }
    // a hook's scene must be one of the mod's own
    for (int k = 0; k < st.inserts.size(); ++k)
        if (!st.labels.contains(st.inserts[k].second))
            missing.push_back(bad(insertLines[k], tbTr("нет сцены «%1»").arg(st.inserts[k].second)));
    for (int k = 0; k < st.replaces.size(); ++k)
        if (!st.labels.contains(st.replaces[k].second))
            missing.push_back(bad(replaceLines[k], tbTr("нет сцены «%1»").arg(st.replaces[k].second)));
    st.steps += missing;
    if (st.id.isEmpty()) st.steps.push_back(bad(1, tbTr("нет @mod_id (латиницей)")));
    if (st.labels.isEmpty()) st.steps.push_back(bad(1, tbTr("нет ни одной сцены (: start)")));
    return st;
}

SceneState stateAt(const Story& story, const Catalog& cat, int line)
{
    SceneState s;
    bool onceCamera = false;                         // (a move that plays once: seen only at its own card)
    for (int i = 0; i < story.steps.size(); ++i) {
        const Step& st = story.steps[i];
        if (st.line > line) break;
        if (st.kind == Step::Bad) continue;
        if (st.kind != Step::Menu && st.kind != Step::Explore) {   // (a choice is on the screen only at its own card)
            s.choices.clear();
            s.spots.clear();
        }
        s.once.clear();
        s.paper.clear();
        s.posterSprite.clear();
        s.posterTitle.clear();
        s.posterText.clear();
        s.overlay.clear();
        s.screen.clear();
        s.screenArgs.clear();
        s.timed = 0;
        if (onceCamera) {
            s.camera.clear();
            onceCamera = false;
        }
        switch (st.kind) {
        case Step::Label:
            break;
        case Step::Background:
            s.bg = st.a;
            s.sprites.clear();
            s.text.clear();
            s.who.clear();
            s.choices.clear();
            s.spots.clear();
            s.camera.clear();                        // a new place: a clean shot (the tape stays)
            s.sizes.clear();
            s.motions.clear();
            s.weather.clear();
            break;
        case Step::Show: {
            const SpriteDef* sp = cat.sprite(st.a);
            const QString who = sp ? sp->who : st.a.section(QLatin1Char(' '), 0, 0);
            for (int k = 0; k < s.sprites.size(); ++k)
                if (s.sprites[k].first.section(QLatin1Char(' '), 0, 0) == who) {
                    s.sprites.removeAt(k);
                    break;
                }
            s.sprites.push_back({st.a, st.b});
            if (st.c.isEmpty()) s.sizes.remove(who);
            else s.sizes.insert(who, st.c.toInt());
            break;
        }
        case Step::Hide:
            for (int k = 0; k < s.sprites.size(); ++k)
                if (s.sprites[k].first.section(QLatin1Char(' '), 0, 0) == st.a) {
                    s.sprites.removeAt(k);
                    break;
                }
            s.motions.remove(st.a);
            s.once.remove(st.a);
            s.sizes.remove(st.a);
            break;
        case Step::Paper:
            s.paper = st.a;
            s.text.clear();
            s.who.clear();
            break;
        // (the hub's own screens, as it draws them: the renderer's drawUi)
        case Step::Epilogue:
            s.screen = QStringLiteral("epilogue");
            s.screenArgs = QStringList{st.transition, st.c, st.a, st.b.isEmpty() ? QStringLiteral("Конец") : st.b};
            s.text.clear();
            s.who.clear();
            break;
        case Step::Title:
            s.screen = QStringLiteral("title");
            s.screenArgs = QStringList{st.a};
            s.text.clear();
            s.who.clear();
            break;
        case Step::Credits:
            s.screen = QStringLiteral("credits");
            s.screenArgs = st.a.split(QLatin1Char('\n'));
            s.text.clear();
            s.who.clear();
            break;
        case Step::Button:
            s.choices = QStringList{st.a};
            break;
        case Step::Suefa:
            // the hub's genry_tb_suefa: the place (the field, or the scene in a haze), the two fists, the three signs
            if (st.b == QLatin1String("field")) s.overlay << QStringLiteral("interface/suefa/fon.png");
            else if (st.b.startsWith(QLatin1String("mods/"))) s.overlay << st.b;
            else {
                if (!st.b.isEmpty()) s.overlay << QStringLiteral("#bg:") + st.b;
                s.overlay << QStringLiteral("#fog");
            }
            s.overlay << (st.a.isEmpty() ? QStringLiteral("interface/suefa/R_03.png")
                                         : QStringLiteral("mods/genry_assets/hands/%1/R_03.png").arg(st.a))
                      << QStringLiteral("interface/suefa/A_03.png") << QStringLiteral("interface/suefa/01.png")
                      << QStringLiteral("interface/suefa/1.png") << QStringLiteral("interface/suefa/2.png")
                      << QStringLiteral("interface/suefa/3.png");
            s.text.clear();
            s.who.clear();
            break;
        case Step::Puzzle:
            s.screen = QStringLiteral("puzzle");
            s.screenArgs = QStringList{st.a, st.b};
            s.text.clear();
            s.who.clear();
            break;
        case Step::InHand:
            s.screen = QStringLiteral("inhand");
            s.screenArgs = QStringList{st.a, st.b};
            s.text.clear();
            s.who.clear();
            break;
        case Step::Achievement:
            s.screen = QStringLiteral("ach");
            s.screenArgs = QStringList{st.a, st.b};
            break;
        case Step::Phone:
            // the game's phone with the receiver up, its keys
            s.overlay << QStringLiteral("locate/Polina/phone_bg.png") << QStringLiteral("locate/Polina/phone_2.png");
            for (int k = 0; k <= 10; ++k) s.overlay << QStringLiteral("locate/Polina/phone/%1.png").arg(k, 3, 10, QLatin1Char('0'));
            s.text.clear();
            s.who.clear();
            break;
        case Step::Snapshot:
            s.screen = QStringLiteral("snap");
            s.screenArgs = QStringList{st.a};
            s.text.clear();
            s.who.clear();
            break;
        case Step::Riddle:
            s.screen = QStringLiteral("riddle");
            s.screenArgs = QStringList{st.a, st.b.section(QLatin1Char('/'), 0, 0)};
            s.text.clear();
            s.who.clear();
            break;
        case Step::Explore:
            s.choices = st.choices;
            for (int k = 0; k < st.choices.size(); ++k) s.spots.push_back(spotPlace(st.places.value(k), k, int(st.choices.size())));
            break;
        case Step::Struggle:
            s.screen = QStringLiteral("struggle");
            s.screenArgs = QStringList{st.a};
            s.text.clear();
            s.who.clear();
            break;
        case Step::Poster: {
            // her photo: as she is on the screen now, else as the game shows her most
            for (const auto& sp : s.sprites)
                if (sp.first.section(QLatin1Char(' '), 0, 0) == st.a) s.posterSprite = sp.first;
            if (s.posterSprite.isEmpty()) {
                const auto all = cat.spritesOf(st.a);
                if (!all.isEmpty()) s.posterSprite = all.first()->name;
            }
            s.posterTitle = st.b;
            s.posterText = st.c;
            s.text.clear();
            s.who.clear();
            break;
        }
        case Step::Camera:
            s.camera = st.a;
            s.cameraAt = st.b.isEmpty() ? QPoint(960, 540) : spotPlace(st.b, 0, 1);
            if (const MotionDef* m = motionById(st.a, true)) onceCamera = m->once;
            break;
        case Step::Move:
            if (const MotionDef* m = motionById(st.b, false)) {
                if (m->once) s.once.insert(st.a, st.b);
                else s.motions.insert(st.a, st.b);
            } else {
                s.motions.remove(st.a);
            }
            break;
        case Step::Weather:
            s.weather = st.a;
            break;
        case Step::Film:
            s.film = st.a;
            break;
        case Step::Say: {
            s.choices.clear();
            s.spots.clear();
            s.text = st.b;
            s.thoughts = st.a == QLatin1String("Ant_m");
            s.quoted = false;
            s.whoColor.clear();
            if (st.a.isEmpty() || st.a == QLatin1String("Ant_m")) {
                s.who.clear();
            } else if (st.a.startsWith(QLatin1Char('@'))) {
                s.who = st.c;
                s.quoted = true;
            } else if (const CharacterDef* c = cat.speaker(st.a)) {
                s.who = c->name;
                s.whoColor = c->color;
                s.thoughts = c->quote.startsWith(QLatin1String("bunny"));
                s.quoted = c->quote == QLatin1String("quote") || c->quote == QLatin1String("bunny_quote");
            }
            break;
        }
        case Step::Window:
            s.window = st.a == QLatin1String("show");
            break;
        case Step::Menu:
            s.choices = st.choices;
            s.spots.clear();
            if (st.a == QLatin1String("spots"))
                for (int k = 0; k < st.choices.size(); ++k) s.spots.push_back(spotPlace(st.places.value(k), k, int(st.choices.size())));
            if (st.a == QLatin1String("timed")) s.timed = qMax(1, qRound(st.b.toDouble()));
            s.text.clear();
            s.who.clear();
            break;
        case Step::Music:
            s.music = st.a;
            break;
        default:
            break;
        }
        s.step = i;
    }
    return s;
}

SceneState coverState(const Story& story, const Catalog& cat)
{
    int withSprite = 0, withBg = 0;
    bool shown = false;
    for (const Step& s : story.steps) {
        if (s.kind == Step::Background) {
            if (!withBg) withBg = s.line;
            shown = false;
        } else if (s.kind == Step::Show) {
            shown = true;
        } else if (s.kind == Step::Say && shown) {
            withSprite = s.line;
            break;
        }
    }
    SceneState st = stateAt(story, cat, withSprite ? withSprite : withBg);
    st.text.clear();
    st.who.clear();
    return st;
}

} // namespace tb
