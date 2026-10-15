// GenryTB V1 - one renderer for everything the constructor shows of the game: a scene as Tiny Bunny draws
// it (its background, its sprites layer by layer, its textbox and name strip, its fonts), a sprite alone
// (character cards), a background (the browser), any image of the archive.
#pragma once
#include "Catalog.h"
#include "Rpa.h"
#include "Story.h"

#include <QCache>
#include <QFont>
#include <QImage>
#include <QMutex>

namespace tb {

class Renderer {
public:
    Renderer(const Vfs& vfs, const Catalog& cat) : m_vfs(vfs), m_cat(cat) {}
    QImage frame(const SceneState& s, const QSize& size = QSize(1920, 1080));
    // the same frame in layers (the preview moves them as the game does): a sprite where it stands on the
    // empty screen; the choice and the text box alone
    QImage spriteLayer(const QString& name, const QString& pos, int percent = 100, const QSize& size = QSize(1920, 1080));
    QRect spriteHead(const QString& name);              // where its head is on its canvas (a square)
    QImage ui(const SceneState& s, const QSize& size = QSize(1920, 1080));
    QImage sprite(const QString& name);                 // the whole sprite canvas (as the game draws it)
    QImage spriteThumb(const QString& name, int h);     // cut to the character, h pixels tall
    QImage spriteFace(const QString& name, int size);   // its head, a square (who speaks, on the cards)
    QImage background(const QString& name);             // 1920x1080
    QImage file(const QString& path);                   // any picture of the game (decoded, cached)
    QFont font(const QString& path, int px, bool italic = false);   // one of the game's fonts

private:
    void drawUi(class QPainter& p, const SceneState& s);
    void drawScreen(class QPainter& p, const SceneState& s);   // the hub's own screens (epilogue, title, polaroid...)
    const Vfs& m_vfs;
    const Catalog& m_cat;
    QCache<QString, QImage> m_files{512 * 1024};        // in KiB
    QCache<QString, QImage> m_sprites{512 * 1024};
    QHash<QString, QString> m_families;                 // font file -> family
    QMutex m_lock;
};

} // namespace tb
