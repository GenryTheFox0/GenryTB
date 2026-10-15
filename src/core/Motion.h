// GenryTB V1 - the game's "pseudo-motion": how Tiny Bunny makes still pictures move (the camera pushing in,
// the frame bobbing with steps, a sprite flinching or laughing). One table of moves: each is a piece of the
// game's own ATL. The same text goes into the hub as a Ren'Py transform and is played by the constructor's
// preview (motionAt) - what is seen in the editor is what the game does.
#pragma once
#include <QHash>
#include <QString>
#include <QVector>

namespace tb {

struct MotionDef {
    QString id;          // zoomin (latin: the transform's name is made of it)
    QString word;        // наезд (the story's word)
    QString title;       // on the card's chip
    QString hint;        // what it looks like
    QString group;       // the chips' row on the card
    bool camera = false; // the whole picture (the master layer); else one sprite
    bool once = false;   // plays once at its card; else lasts until stopped
    bool point = false;  // the camera looks at a point of the picture (ax, ay)
    QString atl;         // the ATL body
};

const QVector<MotionDef>& motions();
const MotionDef* motionByWord(const QString& word, bool camera);
const MotionDef* motionById(const QString& id, bool camera);
QString motionTransform(const MotionDef& m);            // genry_cam_zoomin / genry_mv_happy
QString motionsHub();                                   // all of them as Ren'Py transforms

// the transform's properties t seconds after it starts (zoom, xzoom, yzoom, xoffset, yoffset, rotate, alpha,
// xalign, yalign); ax, ay - the camera's point, 0..1
QHash<QString, double> motionAt(const MotionDef& m, double t, double ax = 0.5, double ay = 0.5);

} // namespace tb
