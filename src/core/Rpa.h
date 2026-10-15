// GenryTB V1 - Ren'Py archive reader + the files Tiny Bunny can load (its game/ folder and archives).
#pragma once
#include <QFile>
#include <QHash>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>

namespace tb {

struct RpaEntry {
    qint64 offset = 0;
    qint64 length = 0;
    QByteArray prefix;
};

class RpaArchive {
public:
    bool open(const QString& path, QString* error);
    QString path() const { return m_path; }
    QStringList files() const { return m_index.keys(); }
    bool contains(const QString& p) const { return m_index.contains(p); }
    QByteArray read(const QString& p);

private:
    QString m_path;
    QHash<QString, RpaEntry> m_index;
    QFile m_file;
};

// What Tiny Bunny can load, in Ren'Py 8's own order: loose files under game/ first, then its archives
// (game/*.rpa by name). A path is as the game names it ("images/sprite/Mom/..."); a name without the
// images/ prefix is found under it too, as Ren'Py does for images.
class Vfs {
public:
    // the Tiny Bunny folder: given, else the Steam one (E:/SteamLibrary/... or any library in libraryfolders.vdf)
    static QString findGame(const QString& hint = {});
    bool mount(const QString& root, QString* error);
    bool isMounted() const { return !m_game.isEmpty(); }
    QString root() const { return m_root; }
    QString gameDir() const { return m_game; }
    QString resolve(const QString& path) const;     // the path that exists ("" none)
    bool has(const QString& path) const { return !resolve(path).isEmpty(); }
    QByteArray read(const QString& path) const;
    QStringList files() const;                      // every archive path

private:
    QString m_root, m_game;
    std::vector<std::unique_ptr<RpaArchive>> m_arcs;
    QHash<QString, int> m_owner;                    // path -> archive
    QHash<QString, QString> m_lower;                // lower-case path -> the real one (archives and loose files)
    mutable QMutex m_lock;
};

} // namespace tb
