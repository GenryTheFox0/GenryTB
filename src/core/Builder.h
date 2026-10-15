// GenryTB V1 - mods into Tiny Bunny: installed under game/mods/<id>/ next to the hub (genry_tb_hub.rpy:
// «Моды» in the game's main menu), and the game started straight in a scene of one, with its own saves.
#pragma once
#include "Compiler.h"
#include "Story.h"

#include <QImage>
#include <QString>
#include <QStringList>

namespace tb {

class Catalog;

struct InstallReport {
    bool ok = false;
    QString error, modDir, startLabel;
    QStringList labels;
};

// cover: the mod's picture in the game's «Моды» (Renderer::frame(coverState(...))); null = none
InstallReport install(const QString& gameRoot, const Story& story, const Catalog& cat, const QImage& cover = QImage());
bool uninstall(const QString& gameRoot, const QString& modId, QString* err);
// the fonts (data/fonts): the ones the game's own {chaos} asks for - into game/ under their own names - and the
// constructor's for every script (Cyrillic, Japanese, Chinese, Korean, Arabic) - into game/mods/genry_fonts/;
// only what is missing or different. Returns how many were copied (-1: none could be).
int installFonts(const QString& gameRoot);
QStringList installedMods(const QString& gameRoot);

// the game straight into a scene (label = <mod>__<scene>; "" = the mod's start); saves in saveDir.
// Returns the game's pid (0: did not start). removeHook() after it closes (the next run does it too).
qint64 runAt(const QString& gameRoot, const QString& label, const QString& saveDir, QString* err);
void removeHook(const QString& gameRoot);
bool isRunning(qint64 pid);

} // namespace tb
