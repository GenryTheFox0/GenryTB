#include "Catalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

namespace tb {
namespace {

// sprite owners (the game's sprite folders) and the names a mod calls them by
struct Alias {
    const char* who;
    const char* ru;          // the name shown in the constructor
    const char* also;        // more names, '|' between
};
const Alias kAliases[] = {
    {"Ant", "Антон", "антон|тоша|антоша"},
    {"Olya", "Оля", "оля|олька"},
    {"Mom", "Мама", "мама|карина|мать"},
    {"Dad", "Отец", "отец|папа|батя"},
    {"Polina", "Полина", "полина|поля"},
    {"Fox", "Алиса", "алиса|лиса|лисичка"},
    {"Romka", "Рома", "рома|ромка"},
    {"Byasha", "Бяша", "бяша"},
    {"ByashaOld", "Бяша (взрослый)", "бяша взрослый"},
    {"Semen", "Семён", "семён|семен|сёма|сема"},
    {"Kate", "Катя", "катя"},
    {"Kate_k", "Катя (костыли)", "катя костыли|катя кукла"},
    {"Tihonov", "Тихонов", "тихонов|лейтенант"},
    {"Olga", "Ольга", "ольга"},
    {"Med", "Врач", "врач|медсестра"},
    {"Ded", "Харитон", "харитон|дед"},
    {"Bear", "Медвежутка", "медвежутка|медведь"},
    {"Owl", "Совушка", "совушка|сова"},
    {"Wolf", "Волчик", "волчик|волк"},
    {"Anya", "Анька", "анька|аня"},
    {"Liliya", "Лилия Павловна", "лилия павловна|лилия|учительница"},
};

QString norm(const QString& s)
{
    QString t = s.trimmed().toLower();
    t.replace(QChar(0x0451), QChar(0x0435));       // ё -> е
    return t;
}

} // namespace

QString Catalog::findData(const QString& name)
{
    QDir d(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        const QString p = d.filePath(QStringLiteral("data/") + name);
        if (QFileInfo::exists(p)) return p;
        if (!d.cdUp()) break;
    }
#ifdef TB_SOURCE_DIR
    const QString src = QDir(QStringLiteral(TB_SOURCE_DIR)).filePath(QStringLiteral("data/") + name);
    if (QFileInfo::exists(src)) return src;
#endif
    return {};
}

bool Catalog::load(const QString& path, QString* error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("нет каталога %1").arg(path);
        return false;
    }
    QJsonParseError pe;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll(), &pe).object();
    if (pe.error != QJsonParseError::NoError) {
        if (error) *error = QStringLiteral("каталог битый: %1").arg(pe.errorString());
        return false;
    }
    m_chars.clear();
    m_sprites.clear();
    m_bgs.clear();
    for (const QJsonValue& v : root.value(QStringLiteral("characters")).toArray()) {
        const QJsonObject o = v.toObject();
        CharacterDef c;
        c.var = o.value(QStringLiteral("var")).toString();
        c.name = o.value(QStringLiteral("name")).toString();
        c.color = o.value(QStringLiteral("color")).toString();
        c.quote = o.value(QStringLiteral("quote")).toString();
        c.lines = o.value(QStringLiteral("lines")).toInt();
        m_chars.push_back(c);
    }
    for (const QJsonValue& v : root.value(QStringLiteral("sprites")).toArray()) {
        const QJsonObject o = v.toObject();
        if (!o.value(QStringLiteral("ok")).toBool()) continue;          // the game's own broken ones: never offered
        SpriteDef s;
        s.name = o.value(QStringLiteral("name")).toString();
        s.who = o.value(QStringLiteral("who")).toString();
        s.emotion = o.value(QStringLiteral("emotion")).toString();
        s.clothes = o.value(QStringLiteral("clothes")).toString();
        s.pose = o.value(QStringLiteral("pose")).toString();
        s.look = o.value(QStringLiteral("look")).toString();
        s.eyes = o.value(QStringLiteral("eyes")).toString();
        s.mouth = o.value(QStringLiteral("mouth")).toString();
        s.blur = o.value(QStringLiteral("blur")).toBool();
        const QJsonArray sz = o.value(QStringLiteral("size")).toArray();
        if (sz.size() == 2) s.size = QSize(sz[0].toInt(), sz[1].toInt());
        s.eyesDy = o.value(QStringLiteral("eyes_dy")).toInt();
        for (const QJsonValue& l : o.value(QStringLiteral("layers")).toArray()) s.layers << l.toString();
        s.shown = o.value(QStringLiteral("shown")).toInt();
        m_sprites.push_back(s);
    }
    const QJsonObject bgs = root.value(QStringLiteral("backgrounds")).toObject();
    for (auto it = bgs.begin(); it != bgs.end(); ++it) {
        const QJsonObject o = it.value().toObject();
        BackgroundDef b;
        b.name = it.key();
        b.file = o.value(QStringLiteral("file")).toString();
        b.color = o.value(QStringLiteral("color")).toString();
        b.shown = o.value(QStringLiteral("shown")).toInt();
        for (const QJsonValue& l : o.value(QStringLiteral("layers")).toArray()) {
            const QJsonArray la = l.toArray();
            if (la.size() == 3) b.layers.push_back({la[0].toInt(), la[1].toInt(), la[2].toString()});
        }
        const QJsonArray bs = o.value(QStringLiteral("size")).toArray();
        if (bs.size() == 2) b.size = QSize(bs[0].toInt(), bs[1].toInt());
        m_bgs.push_back(b);
    }
    std::stable_sort(m_bgs.begin(), m_bgs.end(), [](const BackgroundDef& a, const BackgroundDef& b) {
        return a.shown != b.shown ? a.shown > b.shown : a.name < b.name;
    });
    m_music.clear();
    for (const QJsonValue& v : root.value(QStringLiteral("music")).toArray()) m_music << v.toString();
    m_sounds.clear();
    const QJsonObject snd = root.value(QStringLiteral("sounds")).toObject();
    for (auto it = snd.begin(); it != snd.end(); ++it) {
        QStringList l;
        for (const QJsonValue& v : it.value().toArray()) l << v.toString();
        m_sounds.insert(it.key(), l);
    }
    m_voice.clear();
    const QJsonObject vo = root.value(QStringLiteral("voice")).toObject();
    for (auto it = vo.begin(); it != vo.end(); ++it) m_voice.insert(it.key(), it.value().toInt());

    m_spriteIndex.clear();
    m_bgIndex.clear();
    m_byWho.clear();
    for (int i = 0; i < m_sprites.size(); ++i) {
        m_spriteIndex.insert(m_sprites[i].name, i);
        m_byWho[m_sprites[i].who].push_back(i);
    }
    for (auto it = m_byWho.begin(); it != m_byWho.end(); ++it)
        std::stable_sort(it->begin(), it->end(), [this](int a, int b) { return m_sprites[a].shown > m_sprites[b].shown; });
    for (int i = 0; i < m_bgs.size(); ++i) m_bgIndex.insert(m_bgs[i].name, i);
    if (m_sprites.isEmpty()) {
        if (error) *error = QStringLiteral("в каталоге нет спрайтов");
        return false;
    }
    return true;
}

const SpriteDef* Catalog::sprite(const QString& name) const
{
    auto it = m_spriteIndex.constFind(name.simplified());
    return it == m_spriteIndex.constEnd() ? nullptr : &m_sprites[*it];
}

const BackgroundDef* Catalog::background(const QString& name) const
{
    auto it = m_bgIndex.constFind(name.simplified());
    return it == m_bgIndex.constEnd() ? nullptr : &m_bgs[*it];
}

QVector<const SpriteDef*> Catalog::spritesOf(const QString& who) const
{
    QVector<const SpriteDef*> out;
    for (int i : m_byWho.value(who)) out.push_back(&m_sprites[i]);
    return out;
}

QStringList Catalog::spriteWho() const
{
    QStringList w = m_byWho.keys();
    std::sort(w.begin(), w.end(), [this](const QString& a, const QString& b) {
        return m_byWho.value(a).size() > m_byWho.value(b).size();
    });
    return w;
}

const CharacterDef* Catalog::speaker(const QString& nameOrVar) const
{
    const QString n = norm(nameOrVar);
    // the variable exactly, then the shown name - the one the game uses most when names repeat (Алиса)
    for (const CharacterDef& c : m_chars)
        if (c.var == nameOrVar.trimmed()) return &c;
    const CharacterDef* best = nullptr;
    for (const CharacterDef& c : m_chars)
        if (!c.name.isEmpty() && norm(c.name) == n && (!best || c.lines > best->lines)) best = &c;
    return best;
}

QString Catalog::spriteWhoFor(const QString& name) const
{
    const QString n = norm(name);
    for (const Alias& a : kAliases) {
        if (norm(QString::fromLatin1(a.who)) == n || norm(QString::fromUtf8(a.ru)) == n) return QString::fromLatin1(a.who);
        for (const QString& x : QString::fromUtf8(a.also).split(QLatin1Char('|')))
            if (norm(x) == n) return QString::fromLatin1(a.who);
    }
    return m_byWho.contains(name.trimmed()) ? name.trimmed() : QString();
}

QString Catalog::ruName(const QString& who) const
{
    for (const Alias& a : kAliases)
        if (who == QLatin1String(a.who)) return QString::fromUtf8(a.ru);
    return who;
}

QString Catalog::musicFile(const QString& name) const
{
    const QString n = name.trimmed();
    if (m_music.contains(n)) return n;
    for (const QString& m : m_music) {
        const QString base = QFileInfo(m).completeBaseName();
        if (base.compare(n, Qt::CaseInsensitive) == 0 || m.compare(QStringLiteral("music/") + n, Qt::CaseInsensitive) == 0)
            return m;
    }
    return {};
}

} // namespace tb
