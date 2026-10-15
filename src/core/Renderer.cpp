#include "Renderer.h"

#include <QTextOption>

#include <QFontDatabase>
#include <QPainter>
#include <QPainterPath>
#include <QTextLayout>

namespace tb {
namespace {

constexpr int W = 1920, H = 1080;

int kib(const QImage& i) { return qMax(1, int(i.sizeInBytes() / 1024)); }

// where a sprite of the mod stands (Story's positions; the game's own transforms are drawn in place)
QPointF placeFor(const QString& pos, QSize size, qreal* zoom, int percent = 100)
{
    *zoom = percent / 100.0;                               // (its size: показать ... рост 90)
    qreal dx = 0.0;
    if (pos == QLatin1String("genry_left")) dx = -480;
    else if (pos == QLatin1String("genry_right")) dx = 480;
    else if (pos == QLatin1String("genry_far")) *zoom *= 0.8;
    else if (pos == QLatin1String("genry_close")) *zoom *= 1.25;
    const qreal w = size.width() * *zoom, h = size.height() * *zoom;
    return QPointF((W - w) / 2.0 + dx, H - h);            // Ren'Py's default: bottom centre
}

// text with the game's 1px black outline
void outlined(QPainter& p, const QPointF& at, const QString& text, const QFont& f, const QColor& c)
{
    QPainterPath path;
    path.addText(at, f, text);
    p.setPen(QPen(QColor(0, 0, 0), 2.0));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    p.fillPath(path, c);
}

} // namespace

QImage Renderer::file(const QString& path)
{
    {
        QMutexLocker lock(&m_lock);
        if (QImage* c = m_files.object(path)) return *c;
    }
    // read and decoded outside the lock: the image threads work side by side
    QImage img;
    img.loadFromData(m_vfs.read(path));
    if (img.isNull()) return {};
    img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QMutexLocker lock(&m_lock);
    m_files.insert(path, new QImage(img), kib(img));
    return img;
}

QFont Renderer::font(const QString& path, int px, bool italic)
{
    QString family;
    {
        QMutexLocker lock(&m_lock);
        family = m_families.value(path);
        if (family.isEmpty()) {
            const int id = QFontDatabase::addApplicationFontFromData(m_vfs.read(path));
            const QStringList fams = id >= 0 ? QFontDatabase::applicationFontFamilies(id) : QStringList();
            family = fams.isEmpty() ? QStringLiteral("Arial") : fams.first();
            m_families.insert(path, family);
        }
    }
    QFont f(family);
    f.setPixelSize(px);
    f.setItalic(italic);
    f.setStyleStrategy(QFont::PreferAntialias);
    return f;
}

QImage Renderer::sprite(const QString& name)
{
    {
        QMutexLocker lock(&m_lock);
        if (QImage* c = m_sprites.object(name)) return *c;
    }
    const SpriteDef* s = m_cat.sprite(name);
    if (!s) return {};
    QImage canvas(s->size, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);
    QPainter p(&canvas);
    for (int i = 0; i < s->layers.size(); ++i) {
        const QImage part = file(s->layers[i]);
        const int dy = (i == 1 && !s->eyes.isEmpty()) ? s->eyesDy : 0;
        p.drawImage(QPoint(0, dy), part);
    }
    p.end();
    QMutexLocker lock(&m_lock);
    m_sprites.insert(name, new QImage(canvas), kib(canvas));
    return canvas;
}

// the character's own box: the canvas is the whole screen, the character stands somewhere on it
static QRect characterBox(const QImage& full)
{
    int top = full.height(), bottom = -1, left = full.width(), right = -1;
    for (int y = 0; y < full.height(); y += 4) {
        const QRgb* row = reinterpret_cast<const QRgb*>(full.constScanLine(y));
        for (int x = 0; x < full.width(); x += 4)
            if (qAlpha(row[x]) > 16) {
                top = qMin(top, y);
                bottom = qMax(bottom, y);
                left = qMin(left, x);
                right = qMax(right, x);
            }
    }
    if (bottom < 0) return {};
    return QRect(QPoint(left, top), QPoint(right, bottom)).adjusted(-4, -4, 4, 4).intersected(full.rect());
}

QImage Renderer::spriteThumb(const QString& name, int h)
{
    const QImage full = sprite(name);
    if (full.isNull()) return {};
    const QRect box = characterBox(full);
    if (box.isEmpty()) return {};
    return full.copy(box).scaledToHeight(h, Qt::SmoothTransformation);
}

QImage Renderer::spriteFace(const QString& name, int size)
{
    const QImage full = sprite(name);
    const QRect head = spriteHead(name);
    if (full.isNull() || head.isEmpty()) return {};
    return full.copy(head).scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QRect Renderer::spriteHead(const QString& name)
{
    const QImage full = sprite(name);
    if (full.isNull()) return {};
    const QRect box = characterBox(full);
    if (box.isEmpty()) return {};
    // the head: the top of the figure, a square about the middle of what is drawn up there
    const int side = qMax(40, qMin(int(box.width() * 0.62), int(box.height() * 0.34)));
    qint64 sum = 0, n = 0;
    for (int y = box.top(); y < box.top() + side / 2 && y < full.height(); y += 4) {
        const QRgb* row = reinterpret_cast<const QRgb*>(full.constScanLine(y));
        for (int x = box.left(); x <= box.right(); x += 4)
            if (qAlpha(row[x]) > 16) {
                sum += x;
                ++n;
            }
    }
    const int cx = n ? int(sum / n) : box.center().x();
    QRect head(cx - side / 2, box.top(), side, side);
    if (head.left() < 0) head.moveLeft(0);
    if (head.right() >= full.width()) head.moveRight(full.width() - 1);
    return head.intersected(full.rect());
}

QImage Renderer::background(const QString& name)
{
    QImage out(W, H, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::black);
    if (name.startsWith(QLatin1String("images/"))) {       // a picture file of the game (or its layers: a + b)
        QPainter p(&out);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        for (const QString& layer : name.split(QLatin1Char('+'))) {
            const QImage img = file(layer.trimmed());
            if (!img.isNull()) p.drawImage(QRect(0, 0, W, H), img);
        }
        return out;
    }
    const BackgroundDef* b = m_cat.background(name);
    if (!b) return out;
    if (!b->color.isEmpty()) {
        out.fill(QColor(b->color));
        return out;
    }
    QPainter p(&out);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    if (!b->layers.isEmpty()) {
        // the game's LiveComposite: its canvas, centred like any image shown at the default place (a wide
        // panorama shows its middle)
        const QPoint at((W - b->size.width()) / 2, H - b->size.height());
        for (const BgLayer& l : b->layers) {
            const QImage img = file(l.file);
            if (!img.isNull()) p.drawImage(at + QPoint(l.x, l.y), img);
        }
        return out;
    }
    const QImage img = file(b->file);
    if (img.isNull()) return out;
    if (img.size() == QSize(W, H)) p.drawImage(0, 0, img);
    else p.drawImage(QPoint((W - img.width()) / 2, H - img.height()), img);   // as the game places it
    return out;
}

QImage Renderer::frame(const SceneState& s, const QSize& size)
{
    QImage out = background(s.bg);
    QPainter p(&out);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setRenderHint(QPainter::Antialiasing);
    for (const auto& sp : s.sprites) {
        const QImage img = sprite(sp.first);
        if (img.isNull()) continue;
        qreal zoom = 1.0;
        const QPointF at = placeFor(sp.second, img.size(), &zoom, s.sizes.value(sp.first.section(QLatin1Char(' '), 0, 0), 100));
        p.drawImage(QRectF(at, QSizeF(img.width() * zoom, img.height() * zoom)), img);
    }
    drawUi(p, s);
    p.end();
    return size == out.size() ? out : out.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage Renderer::spriteLayer(const QString& name, const QString& pos, int percent, const QSize& size)
{
    QImage out(W, H, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    const QImage img = sprite(name);
    if (!img.isNull()) {
        QPainter p(&out);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        qreal zoom = 1.0;
        const QPointF at = placeFor(pos, img.size(), &zoom, percent);
        p.drawImage(QRectF(at, QSizeF(img.width() * zoom, img.height() * zoom)), img);
    }
    return size == out.size() ? out : out.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage Renderer::ui(const SceneState& s, const QSize& size)
{
    QImage out(W, H, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setRenderHint(QPainter::Antialiasing);
    drawUi(p, s);
    p.end();
    return size == out.size() ? out : out.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

// plain words on paper: wrapped to the width, from the left or centred
static void written(QPainter& p, const QRectF& box, const QString& text, const QFont& f, const QColor& c, bool centred, qreal gap)
{
    QTextLayout layout(text, f);
    QTextOption opt;
    opt.setWrapMode(QTextOption::WordWrap);
    layout.setTextOption(opt);
    layout.beginLayout();
    qreal y = 0;
    const QFontMetricsF fm(f);
    while (true) {
        QTextLine l = layout.createLine();
        if (!l.isValid()) break;
        l.setLineWidth(box.width());
        l.setPosition(QPointF(0, y));
        y += fm.lineSpacing() + gap;
    }
    layout.endLayout();
    p.setPen(c);
    p.setFont(f);
    for (int i = 0; i < layout.lineCount(); ++i) {
        const QTextLine l = layout.lineAt(i);
        if (box.y() + l.position().y() + fm.height() > box.bottom()) break;
        const qreal x = centred ? box.x() + (box.width() - l.naturalTextWidth()) / 2.0 : box.x();
        p.drawText(QPointF(x, box.y() + l.position().y() + fm.ascent()), text.mid(l.textStart(), l.textLength()).trimmed());
    }
}

// the hub's own screens (HubMotion.inc), the same numbers: an ending page, a heading, the titles, the polaroid,
// the riddle's paper, the struggle's bar
void Renderer::drawScreen(QPainter& p, const SceneState& s)
{
    const QStringList& a = s.screenArgs;
    const QColor ink(0x1b, 0x1b, 0x1b);
    if (s.screen == QLatin1String("title")) {
        p.fillRect(QRect(0, 0, W, H), QColor(0, 0, 0, 0x66));
        const QFont f = font(QStringLiteral("font/razor_k.ttf"), 150);
        const QFontMetricsF fm(f);
        const QString t = a.value(0);
        outlined(p, QPointF((W - fm.horizontalAdvance(t)) / 2.0, H * 0.45 + fm.ascent() / 2.0), t, f, QColor(0xf2, 0xf2, 0xf2));
    } else if (s.screen == QLatin1String("epilogue")) {
        // genry_tb_epilogue: the art's layers (or the scene gone dark), the column from (100, 100) 960 wide, the brush
        const QStringList layers = a.value(0).split(QLatin1Char('+'), Qt::SkipEmptyParts);
        if (layers.isEmpty()) {
            p.fillRect(QRect(0, 0, W, H), QColor(0, 0, 0, 0xcc));
        } else {
            for (const QString& l : layers) {
                const QString x = l.trimmed();
                const QImage img = x.startsWith(QLatin1String("images/")) ? file(x) : background(x);
                if (!img.isNull()) p.drawImage(QRect(0, 0, W, H), img);
            }
            p.fillRect(QRect(0, 0, W, H), QColor(0, 0, 0, 0x33));
        }
        qreal y = 100;
        if (!a.value(1).isEmpty()) {
            const QFont hf = font(QStringLiteral("font/russia.ttf"), 62);
            outlined(p, QPointF(100, y + QFontMetricsF(hf).ascent()), a.value(1), hf, Qt::white);
            y += QFontMetricsF(hf).height() + 14;
        }
        const QFont bf = font(QStringLiteral("font/russia.ttf"), 52);
        QTextLayout layout(a.value(2), bf);
        layout.beginLayout();
        qreal ly = 0;
        while (true) {
            QTextLine l = layout.createLine();
            if (!l.isValid()) break;
            l.setLineWidth(960);
            l.setPosition(QPointF(0, ly));
            ly += QFontMetricsF(bf).lineSpacing() + 6;
        }
        layout.endLayout();
        for (int i = 0; i < layout.lineCount(); ++i) {
            const QTextLine l = layout.lineAt(i);
            outlined(p, QPointF(100, y + l.position().y() + QFontMetricsF(bf).ascent()), a.value(2).mid(l.textStart(), l.textLength()).trimmed(), bf, Qt::white);
        }
        const QImage btn = file(QStringLiteral("images/interface/button.png"));
        const QRectF br(1673 - 438, 821, 438, 86);
        if (!btn.isNull()) p.drawImage(br, btn);
        const QFont cf = font(QStringLiteral("font/russia.ttf"), 50);
        const QFontMetricsF cm(cf);
        p.setFont(cf);
        p.setPen(Qt::black);
        p.drawText(QPointF(br.center().x() - cm.horizontalAdvance(a.value(3)) / 2.0, br.center().y() + cm.ascent() / 2.0 - 6), a.value(3));
    } else if (s.screen == QLatin1String("credits")) {
        p.fillRect(QRect(0, 0, W, H), Qt::black);
        const QFont f = font(QStringLiteral("font/russia.ttf"), 54);
        const QFontMetricsF fm(f);
        qreal y = (H - a.size() * (fm.height() + 34)) / 2.0;
        for (const QString& l : a) {
            outlined(p, QPointF((W - fm.horizontalAdvance(l)) / 2.0, y + fm.ascent()), l, f, QColor(0xe6, 0xe6, 0xe6));
            y += fm.height() + 34;
        }
    } else if (s.screen == QLatin1String("snap")) {
        // genry_tb_polaroid's card: the shot (here: the place), its caption scrawled under it
        p.fillRect(QRect(0, 0, W, H), QColor(0, 0, 0, 0xaa));
        const QRectF card(520, 100, 880, 880);
        p.save();
        p.translate(card.center());
        p.rotate(-3);
        p.translate(-card.center());
        p.fillRect(card, QColor(0xf1, 0xed, 0xe4));
        const QRectF shot(card.x() + 40, card.y() + 40, 800, 450);
        const QImage place = background(s.bg);
        if (!place.isNull()) p.drawImage(shot, place);
        else p.fillRect(shot, QColor(0x6e, 0x6e, 0x6e));
        for (const auto& sp : s.sprites) {
            const QImage img = sprite(sp.first);
            if (img.isNull()) continue;
            qreal zoom = 1.0;
            const QPointF at = placeFor(sp.second, img.size(), &zoom, s.sizes.value(sp.first.section(QLatin1Char(' '), 0, 0), 100));
            const qreal k = shot.width() / W;
            p.drawImage(QRectF(shot.x() + at.x() * k, shot.y() + at.y() * k, img.width() * zoom * k, img.height() * zoom * k), img);
        }
        written(p, QRectF(card.x() + 60, card.y() + 560, 760, 280), a.value(0), font(QStringLiteral("font/razor_k.ttf"), 66), QColor(0x22, 0x22, 0x22), false, 0);
        p.restore();
    } else if (s.screen == QLatin1String("riddle")) {
        p.fillRect(QRect(0, 0, W, H), QColor(0, 0, 0, 0x99));
        const QRectF sheet(460, 230, 1000, 560);
        p.save();
        p.translate(sheet.center());
        p.rotate(-1.5);
        p.translate(-sheet.center());
        p.fillRect(sheet, QColor(0xe8, 0xe2, 0xd2));
        written(p, QRectF(sheet.x() + 70, sheet.y() + 60, 860, 240), a.value(0), font(QStringLiteral("font/razor_k.ttf"), 60), ink, false, 4);
        p.setFont(font(QStringLiteral("font/russia.ttf"), 64));
        p.setPen(QColor(0x7a, 0x10, 0x10));
        p.drawText(QPointF(sheet.x() + 80, sheet.y() + 384), a.value(1));
        p.fillRect(QRectF(sheet.x() + 70, sheet.y() + 400, 860, 3), QColor(0x1b, 0x1b, 0x1b, 0x55));
        p.setFont(font(QStringLiteral("font/russia.ttf"), 34));
        p.setPen(QColor(0x1b, 0x1b, 0x1b, 0x99));
        p.drawText(QPointF(sheet.x() + 70, sheet.y() + 470), QStringLiteral("впиши ответ и нажми Enter"));
        p.restore();
    } else if (s.screen == QLatin1String("puzzle")) {
        // genry_tb_puzzle: the poster game's table, the picture's ghost on its place, its pieces scattered around
        const QImage table = file(QStringLiteral("images/minigame/ss/fon.png"));
        if (!table.isNull()) p.drawImage(QRect(0, 0, W, H), table);
        else p.fillRect(QRect(0, 0, W, H), QColor(0x2a, 0x2a, 0x2a));
        QImage pic(1200, 675, QImage::Format_ARGB32_Premultiplied);
        pic.fill(Qt::transparent);
        {
            QPainter pp(&pic);
            pp.setRenderHint(QPainter::SmoothPixmapTransform);
            for (const QString& layer : a.value(0).split(QLatin1Char('+'), Qt::SkipEmptyParts)) {
                const QString x = layer.trimmed();
                const QImage one = x.startsWith(QLatin1String("images/")) ? file(x) : background(x);
                if (!one.isNull()) pp.drawImage(QRect(0, 0, 1200, 675), one);
            }
        }
        p.fillRect(QRect(350, 160, 1220, 695), QColor(0, 0, 0, 0x40));
        p.setOpacity(0.14);
        p.drawImage(QPoint(360, 170), pic);
        p.setOpacity(1.0);
        const int cols = qBound(2, a.value(1).section(QLatin1Char('x'), 0, 0).toInt(), 5), rows = qBound(2, a.value(1).section(QLatin1Char('x'), 1, 1).toInt(), 5);
        const int pw = 1200 / cols, ph = 675 / rows;
        quint32 seed = 7;
        auto rnd = [&seed](int lo, int hi) { seed = seed * 1103515245u + 12345u; return lo + int((seed >> 8) % quint32(qMax(1, hi - lo))); };
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c) {
                const int x = rnd(20, 1900 - pw), y = rnd(20, 1060 - ph);
                p.fillRect(QRect(x + 6, y + 6, pw, ph), QColor(0, 0, 0, 0x55));
                p.drawImage(QRect(x, y, pw, ph), pic, QRect(c * pw, r * ph, pw, ph));
            }
    } else if (s.screen == QLatin1String("inhand")) {
        // genry_tb_inhand: the dark, the picture in the pinch (tilted), the game's hand over it, the caption
        p.fillRect(QRect(0, 0, W, H), QColor(0, 0, 0, 0xb0));
        QImage img;
        for (const QString& layer : a.value(0).split(QLatin1Char('+'), Qt::SkipEmptyParts)) {
            const QString x = layer.trimmed();
            const QImage one = x.startsWith(QLatin1String("images/")) ? file(x) : background(x);
            if (one.isNull()) continue;
            if (img.isNull()) {
                img = one.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            } else {
                QPainter lp(&img);
                lp.drawImage(QRect(QPoint(0, 0), img.size()), one);
            }
        }
        // a photo card: cream margins, the picture on dark inside them
        p.save();
        p.translate(800, 360);
        p.rotate(-4);
        p.fillRect(QRectF(-240, -180, 480, 360), QColor(0xec, 0xe8, 0xdf));
        p.fillRect(QRectF(-226, -166, 452, 332), QColor(0x2a, 0x2a, 0x2a));
        if (!img.isNull()) {
            const QSize fit = img.size().scaled(452, 332, Qt::KeepAspectRatio);
            p.drawImage(QRectF(-fit.width() / 2.0, -fit.height() / 2.0, fit.width(), fit.height()), img);
        }
        p.restore();
        const QImage hand = file(QStringLiteral("images/interface/suefa/vkladush_0.png"));
        if (!hand.isNull()) p.drawImage(QRect(0, 0, W, H), hand);
        if (!a.value(1).isEmpty()) {
            const QFont f = font(QStringLiteral("font/razor_k.ttf"), 64);
            outlined(p, QPointF((W - QFontMetricsF(f).horizontalAdvance(a.value(1))) / 2.0, 930 + QFontMetricsF(f).ascent()), a.value(1), f, Qt::white);
        }
    } else if (s.screen == QLatin1String("ach")) {
        // genry_tb_ach: the dark plate at the top right, the cup, the title, how it was got
        const QRectF plate(W - 40 - 620, 40, 620, 150);
        p.fillRect(plate, QColor(0x0e, 0x0e, 0x10, 0xe6));
        p.fillRect(QRectF(plate.x(), plate.y(), 6, plate.height()), QColor(0xd6, 0x3a, 0x3a));
        const QFont small = font(QStringLiteral("font/russia.ttf"), 30);
        p.setFont(small);
        p.setPen(QColor(0x96, 0x96, 0x9c));
        p.drawText(QPointF(plate.x() + 140, plate.y() + 44), QStringLiteral("ДОСТИЖЕНИЕ"));
        const QFont tf = font(QStringLiteral("font/razor_k.ttf"), 56);
        outlined(p, QPointF(plate.x() + 140, plate.y() + 100), a.value(0), tf, Qt::white);
        p.setFont(font(QStringLiteral("font/russia.ttf"), 30));
        p.setPen(QColor(0xcf, 0xcf, 0xcf));
        p.drawText(QPointF(plate.x() + 140, plate.y() + 136), a.value(1));
        p.setPen(QPen(QColor(0xe8, 0xe2, 0xd2), 5));
        p.setBrush(Qt::NoBrush);
        const QPointF c(plate.x() + 70, plate.y() + 70);
        p.drawEllipse(c, 40, 40);
        QFont star(QStringLiteral("Segoe UI Symbol"));
        star.setPixelSize(50);
        p.setFont(star);
        p.drawText(QRectF(c.x() - 40, c.y() - 40, 80, 80), Qt::AlignCenter, QStringLiteral("★"));
    } else if (s.screen == QLatin1String("struggle")) {
        p.fillRect(QRect(0, 0, W, H), QColor(0, 0, 0, 0x77));
        const QFont f = font(QStringLiteral("font/razor_k.ttf"), 120);
        const QFontMetricsF fm(f);
        outlined(p, QPointF((W - fm.horizontalAdvance(a.value(0))) / 2.0, 230 + fm.ascent()), a.value(0), f, Qt::white);
        p.fillRect(QRectF(460, 520, 1000, 60), QColor(0, 0, 0, 0xcc));
        p.fillRect(QRectF(460, 520, 300, 60), QColor(0xe8, 0xe2, 0xd2));
        const QFont hf = font(QStringLiteral("font/russia.ttf"), 44);
        const QString hint = QStringLiteral("жми пробел или кликай!");
        outlined(p, QPointF((W - QFontMetricsF(hf).horizontalAdvance(hint)) / 2.0, 610 + QFontMetricsF(hf).ascent()), hint, hf, QColor(0xd8, 0xd8, 0xd8));
        p.fillRect(QRectF(660, 700, 600, 6), Qt::white);
    }
}

// what lies over the scene and does not move with it: a choice, the text box with the line
void Renderer::drawUi(QPainter& p, const SceneState& s)
{
    for (const QString& layer : s.overlay) {          // суефа's field and hands (the game's pictures, the hands' sets)
        if (layer == QLatin1String("#fog")) {
            p.fillRect(QRect(0, 0, W, H), QColor(0x30, 0x30, 0x30, 0x99));      // the place in a haze (preview: no blur)
            continue;
        }
        if (layer.startsWith(QLatin1String("#bg:"))) {
            p.drawImage(QRect(0, 0, W, H), background(layer.mid(4)));
            continue;
        }
        const QImage img = file(layer);
        if (!img.isNull()) p.drawImage(QRect(0, 0, W, H), img);
    }
    if (s.screen == QLatin1String("ach")) {
        drawScreen(p, s);                             // (a trophy at the corner: the scene and its line stay)
    } else if (!s.screen.isEmpty()) {
        drawScreen(p, s);
        return;                                       // (the text box is away while it is on)
    }
    if (!s.paper.isEmpty() || !s.posterTitle.isEmpty()) {
        // a note / a "missing" notice: the hub's genry_tb_note and genry_tb_poster screens, the same numbers
        const bool poster = s.paper.isEmpty();
        const QRectF sheet = poster ? QRectF(580, 60, 760, 960) : QRectF(510, 198, 900, 640);
        p.fillRect(QRect(0, 0, W, H), QColor(0, 0, 0, 0x99));
        p.save();
        p.translate(sheet.center());
        p.rotate(-1.5);
        p.translate(-sheet.center());
        p.fillRect(sheet, QColor(0xe8, 0xe2, 0xd2));
        const QFont scrawl = font(QStringLiteral("font/razor_k.ttf"), poster ? 84 : 58);
        if (!poster) {
            written(p, QRectF(sheet.x() + 70, sheet.y() + 56, 760, 560), s.paper, scrawl, QColor(0x1b, 0x1b, 0x1b), false, 4);
        } else {
            written(p, QRectF(sheet.x() + 30, sheet.y() + 34, 700, 130), s.posterTitle, scrawl, QColor(0x14, 0x14, 0x14), true, 0);
            p.fillRect(QRectF(sheet.x() + 196, sheet.y() + 176, 368, 368), QColor(0x14, 0x14, 0x14));
            const QRectF photo(sheet.x() + 204, sheet.y() + 184, 352, 352);
            p.fillRect(photo, QColor(0xcf, 0xc9, 0xb8));
            const QImage full = sprite(s.posterSprite);
            const QRect head = spriteHead(s.posterSprite);
            if (!full.isNull() && !head.isEmpty()) p.drawImage(photo, full, head);
            written(p, QRectF(sheet.x() + 60, sheet.y() + 576, 640, 360), s.posterText, font(QStringLiteral("font/russia.ttf"), 40),
                    QColor(0x1b, 0x1b, 0x1b), true, 0);
        }
        p.restore();
        return;                                       // (the text box is away while it is read)
    }
    if (!s.choices.isEmpty()) {
        const QFont cf = font(QStringLiteral("font/russia.ttf"), 60);
        const QFontMetricsF fm(cf);
        if (!s.spots.isEmpty()) {
            // the game's picture choice: a soft glow at each spot, its word over it
            const QImage glow = file(QStringLiteral("images/interface/intercative_button.png"));
            for (int k = 0; k < s.choices.size() && k < s.spots.size(); ++k) {
                const QPointF c = s.spots[k];
                if (!glow.isNull()) {
                    p.setOpacity(0.45);
                    p.drawImage(c - QPointF(glow.width() / 2.0, glow.height() / 2.0), glow);
                    p.setOpacity(1.0);
                }
                const qreal w = fm.horizontalAdvance(s.choices[k]);
                outlined(p, QPointF(c.x() - w / 2.0, c.y() + fm.ascent() / 2.0 - 6), s.choices[k], cf, Qt::white);
            }
        } else {
            // the hub's genry_tb_choice: dark strips 84 tall, 18 apart, the column low on the screen
            const int n = int(s.choices.size());
            const qreal total = n * 84.0 + (n - 1) * 18.0 + (s.timed > 0 ? 18 + 6 + 18 + 6 : 0);
            qreal top = (H - total) * 0.72;
            for (const QString& c : s.choices) {
                const qreal w = fm.horizontalAdvance(c);
                const qreal strip = qMax(760.0, w + 120.0);
                p.fillRect(QRectF((W - strip) / 2.0, top, strip, 84), QColor(0, 0, 0, 0x88));
                outlined(p, QPointF((W - w) / 2.0, top + (84 - fm.height()) / 2.0 + fm.ascent()), c, cf, Qt::white);
                top += 84 + 18;
            }
            // against the clock: the strip under the list, part of it burnt already
            if (s.timed > 0) p.fillRect(QRectF((W - 600 * 0.7) / 2.0, top + 6, 600 * 0.7, 6), Qt::white);
        }
    }
    if (s.window && !s.text.isEmpty()) {
        // screens.rpy say: the window 1820x250 at the bottom, its panel.png 1600 wide in the middle
        const QImage panel = file(QStringLiteral("images/interface/panel.png"));
        const int wx = (W - 1820) / 2, wy = H - 250;
        if (!panel.isNull()) p.drawImage(QRect((W - 1600) / 2, wy, 1600, 250), panel);
        const QFont text = font(QStringLiteral("font/russia.ttf"), 60, s.thoughts);
        if (!s.who.isEmpty()) {
            // the name strip: polosa.png 500x40 at (150, -30) of the window, the name over it
            const QImage strip = file(QStringLiteral("images/interface/polosa.png"));
            if (!strip.isNull()) p.drawImage(QRect(wx + 150, wy - 30, 500, 40), strip);
            const QFont nf = font(QStringLiteral("font/russia.ttf"), 80);
            const QFontMetrics fm(nf);
            outlined(p, QPointF(wx + 150 + 42 + 30, wy - 30 + 20 - 45 + fm.ascent() / 2.0), s.who, nf,
                     s.whoColor.isEmpty() ? QColor(Qt::white) : QColor(s.whoColor));
        }
        // the line: quotes round a character's (the game's what_prefix), the narrator's bare
        const QString line = s.quoted ? QStringLiteral("\"%1\"").arg(s.text) : s.text;
        QTextLayout layout(line, text);
        layout.beginLayout();
        qreal y = 0;
        const QFontMetricsF fm(text);
        while (true) {
            QTextLine l = layout.createLine();
            if (!l.isValid()) break;
            l.setLineWidth(1400);
            l.setPosition(QPointF(0, y));
            y += fm.lineSpacing() + 1;
        }
        layout.endLayout();
        for (int i = 0; i < layout.lineCount(); ++i) {
            const QTextLine l = layout.lineAt(i);
            const QString part = line.mid(l.textStart(), l.textLength()).trimmed();
            outlined(p, QPointF(wx + 200, wy + 45 + l.position().y() + fm.ascent()), part, text, Qt::white);
        }
    }
}

} // namespace tb
