// GenryTB V1 - what Tiny Bunny has to put on the screen and play (data/tb_catalog.json, made from the
// game's own scripts by tools/make_tb_catalog.py) and the names a mod calls it by.
#pragma once
#include <QHash>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

namespace tb {

struct CharacterDef {
    QString var;          // the game's Character: Ant, Oly, Pol...
    QString name;         // as it shows: «Антон»
    QString color;        // who_color, "" = the game's default
    QString quote;        // quote | bunny (italic thoughts in quotes) | none
    int lines = 0;        // how often the game gives it a line
};

struct SpriteDef {
    QString name;         // as `show` takes it: "Mom Normal m_day 01 norm aside"
    QString who, emotion, clothes, pose, look, eyes, mouth;
    bool blur = false;
    QSize size{1920, 1080};
    int eyesDy = 0;       // the eyes layer is drawn this far down
    QStringList layers;   // body, eyes (open), mouth, tears - archive paths, in drawing order
    int shown = 0;        // how often the game shows it
};

struct BgLayer {
    int x = 0, y = 0;
    QString file;
};

struct BackgroundDef {
    QString name;         // as `scene` takes it
    QString file;         // one picture, or
    QVector<BgLayer> layers;   // the game's LiveComposite: pictures, each where it goes
    QSize size{1920, 1080};
    QString color;        // "#rrggbb"
    int shown = 0;
};

class Catalog {
public:
    static QString findData(const QString& name);    // data/<name>, walking up from the program
    bool load(const QString& path, QString* error);
    bool isLoaded() const { return !m_sprites.isEmpty(); }

    const QVector<CharacterDef>& characters() const { return m_chars; }
    const QVector<SpriteDef>& sprites() const { return m_sprites; }
    const QVector<BackgroundDef>& backgrounds() const { return m_bgs; }
    const QStringList& music() const { return m_music; }
    const QHash<QString, QStringList>& sounds() const { return m_sounds; }
    const QHash<QString, int>& voice() const { return m_voice; }

    const SpriteDef* sprite(const QString& name) const;
    const BackgroundDef* background(const QString& name) const;
    QVector<const SpriteDef*> spritesOf(const QString& who) const;  // most shown first
    QStringList spriteWho() const;                                  // sprite owners, most sprites first

    // who a line is for: the game's Character by its shown name (Антон, Мама, Полина...) or variable
    const CharacterDef* speaker(const QString& nameOrVar) const;
    // a sprite owner (Mom, Fox...) by any name a mod calls it: Мама, мама, Алиса, Лиса, Mom...
    QString spriteWhoFor(const QString& name) const;
    // the Russian name of a sprite owner (Mom -> Мама)
    QString ruName(const QString& who) const;
    // a music track by its short name (Menu_Tiny_Bunny) or path
    QString musicFile(const QString& name) const;

private:
    QVector<CharacterDef> m_chars;
    QVector<SpriteDef> m_sprites;
    QVector<BackgroundDef> m_bgs;
    QStringList m_music;
    QHash<QString, QStringList> m_sounds;
    QHash<QString, int> m_voice;
    QHash<QString, int> m_spriteIndex, m_bgIndex;
    QHash<QString, QVector<int>> m_byWho;
};

} // namespace tb
