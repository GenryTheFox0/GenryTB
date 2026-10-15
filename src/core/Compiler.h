// GenryTB V1 - a story into Tiny Bunny's own Ren'Py 8 script (game/mods/<id>/<id>.rpy), and the hub
// every mod hangs on (game/mods/genry_tb_hub.rpy: the mods' list, «Моды» in the game's main menu, the
// positions a mod can stand a sprite at).
#pragma once
#include "Story.h"

#include <QRect>
#include <QString>

#include <functional>

namespace tb {

class Catalog;

struct Compiled {
    QString rpy;
    QString startLabel;          // <mod>__<first scene>
    QStringList labels;
    bool ok = false;
    QString error;
};

Compiled compileStory(const Story& story, const Catalog& cat);
// where a sprite's head is on its canvas (a notice's photo is cut there): given by whoever has the game's
// pictures (the renderer); without it - a guess
void setHeadBox(std::function<QRect(const QString& sprite)> fn);
QString hubScript();             // genry_tb_hub.rpy
int hubVersion();

} // namespace tb
