#include "Rpa.h"
#include "Pickle.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QtEndian>

namespace tb {
namespace {

// zlib stream -> bytes. qUncompress wants a 4-byte big-endian size hint in front; the real size is
// unknown, so start generous and grow if Qt refuses.
QByteArray inflate(const QByteArray& z)
{
    qint64 hint = qMax<qint64>(z.size() * 32, 64 * 1024);
    while (hint <= 512ll * 1024 * 1024) {
        QByteArray buf(4, Qt::Uninitialized);
        qToBigEndian<quint32>(quint32(hint), reinterpret_cast<uchar*>(buf.data()));
        buf += z;
        QByteArray out = qUncompress(buf);
        if (!out.isEmpty())
            return out;
        hint *= 8;
    }
    return {};
}

} // namespace

bool RpaArchive::open(const QString& path, QString* error)
{
    m_path = path;
    m_index.clear();
    if (m_file.isOpen()) m_file.close();
    m_file.setFileName(path);
    if (!m_file.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("не открыть %1").arg(path);
        return false;
    }
    const QString head = QString::fromLatin1(m_file.readLine(256)).trimmed();
    const QStringList parts = head.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    qint64 offset = 0, key = 0;
    bool ok = false;
    if ((head.startsWith(QLatin1String("RPA-3.0")) || head.startsWith(QLatin1String("RPA-3.2"))) && parts.size() >= 3) {
        bool ok2 = false;
        offset = parts[1].toLongLong(&ok, 16);
        key = parts[2].toLongLong(&ok2, 16);
        ok = ok && ok2;
    } else if (head.startsWith(QLatin1String("RPA-2.0")) && parts.size() >= 2) {
        offset = parts[1].toLongLong(&ok, 16);
    }
    if (!ok) {
        if (error) *error = QStringLiteral("%1: неизвестный заголовок '%2'").arg(path, head.left(40));
        return false;
    }
    m_file.seek(offset);
    const QByteArray raw = inflate(m_file.readAll());
    if (raw.isEmpty()) {
        if (error) *error = QStringLiteral("%1: индекс не распаковался (zlib)").arg(path);
        return false;
    }
    QString perr;
    PyRef root = unpickle(raw, &perr);
    if (!root || root->kind != PyObj::Dict) {
        if (error) *error = QStringLiteral("%1: индекс не pickle-dict: %2").arg(path, perr);
        return false;
    }
    for (const auto& kv : root->dict) {
        const PyRef& entries = kv.second;
        if (!kv.first || !entries || entries->items.isEmpty()) continue;
        const PyRef& t = entries->items.first();
        if (!t || t->items.size() < 2) continue;
        RpaEntry e;
        e.offset = t->items[0]->i ^ key;
        e.length = t->items[1]->i ^ key;
        if (t->items.size() > 2 && t->items[2]) e.prefix = t->items[2]->bytes;
        QString name = kv.first->text();
        name.replace(QLatin1Char('\\'), QLatin1Char('/'));
        m_index.insert(name, e);
    }
    return true;
}

QByteArray RpaArchive::read(const QString& p)
{
    auto it = m_index.constFind(p);
    if (it == m_index.constEnd()) return {};
    const RpaEntry& e = *it;
    m_file.seek(e.offset);
    return e.prefix + m_file.read(e.length - e.prefix.size());
}

QString Vfs::findGame(const QString& hint)
{
    auto ok = [](const QString& d) { return QFileInfo(QDir(d).filePath(QStringLiteral("TinyBunny.exe"))).exists(); };
    if (!hint.isEmpty() && ok(hint)) return QDir::cleanPath(hint);
    QStringList libs;
    for (const QString& steam : {QStringLiteral("C:/Program Files (x86)/Steam"), QStringLiteral("C:/Program Files/Steam")}) {
        libs << steam;
        QFile vdf(QDir(steam).filePath(QStringLiteral("steamapps/libraryfolders.vdf")));
        if (vdf.open(QIODevice::ReadOnly)) {
            static const QRegularExpression re(QStringLiteral("\"path\"\\s+\"([^\"]+)\""));
            auto it = re.globalMatch(QString::fromUtf8(vdf.readAll()));
            while (it.hasNext()) libs << it.next().captured(1).replace(QStringLiteral("\\\\"), QStringLiteral("/"));
        }
    }
    for (const QChar d : QStringLiteral("CDEFGH")) libs << QStringLiteral("%1:/SteamLibrary").arg(d);
    for (const QString& lib : libs) {
        const QString d = QDir(lib).filePath(QStringLiteral("steamapps/common/TinyBunny"));
        if (ok(d)) return QDir::cleanPath(d);
    }
    return {};
}

bool Vfs::mount(const QString& root, QString* error)
{
    QMutexLocker lock(&m_lock);
    m_root.clear();
    m_game.clear();
    m_arcs.clear();
    m_owner.clear();
    m_lower.clear();
    const QString game = QDir(root).filePath(QStringLiteral("game"));
    if (!QFileInfo(game).isDir()) {
        if (error) *error = QStringLiteral("нет папки game в %1").arg(root);
        return false;
    }
    QStringList rpas = QDir(game).entryList({QStringLiteral("*.rpa")}, QDir::Files, QDir::Name);
    for (const QString& f : rpas) {
        auto a = std::make_unique<RpaArchive>();
        QString err;
        if (!a->open(QDir(game).filePath(f), &err)) {
            if (error) *error = err;
            continue;
        }
        const int idx = int(m_arcs.size());
        for (const QString& p : a->files())
            if (!m_owner.contains(p)) m_owner.insert(p, idx);
        m_arcs.push_back(std::move(a));
    }
    if (m_arcs.empty()) {
        if (error && error->isEmpty()) *error = QStringLiteral("в %1 нет архивов игры (.rpa)").arg(game);
        return false;
    }
    m_root = QDir::cleanPath(root);
    m_game = QDir::cleanPath(game);
    for (auto it = m_owner.constBegin(); it != m_owner.constEnd(); ++it) m_lower.insert(it.key().toLower(), it.key());
    QDirIterator walk(m_game, QDir::Files, QDirIterator::Subdirectories);   // the loose ones win (Ren'Py's order)
    while (walk.hasNext()) {
        const QString rel = QDir(m_game).relativeFilePath(walk.next());
        m_lower.insert(rel.toLower(), rel);
    }
    return true;
}

QString Vfs::resolve(const QString& path) const
{
    if (path.isEmpty() || m_game.isEmpty()) return {};
    QString p = path;
    p.replace(QLatin1Char('\\'), QLatin1Char('/'));
    for (const QString& c : {p, QStringLiteral("images/") + p}) {
        if (QFileInfo(QDir(m_game).filePath(c)).isFile()) return c;
        if (m_owner.contains(c)) return c;
    }
    for (const QString& c : {p, QStringLiteral("images/") + p}) {      // any letters' case ("h1" for "H1")
        const QString real = m_lower.value(c.toLower());
        if (!real.isEmpty()) return real;
    }
    return {};
}

QByteArray Vfs::read(const QString& path) const
{
    const QString p = resolve(path);
    if (p.isEmpty()) return {};
    const QString loose = QDir(m_game).filePath(p);
    if (QFileInfo(loose).isFile()) {
        QFile f(loose);
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    }
    QMutexLocker lock(&m_lock);
    const int i = m_owner.value(p, -1);
    return i >= 0 ? m_arcs[size_t(i)]->read(p) : QByteArray();
}

QStringList Vfs::files() const
{
    return m_owner.keys();
}

} // namespace tb
