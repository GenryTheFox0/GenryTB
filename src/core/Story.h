// GenryTB V1 - a mod's story (story.txt, the constructor's Russian command language) read into steps,
// and the state of the scene at any line of it (what the preview draws).
//
//   @mod_id first_mod          the header: id (latin), name, author
//   @mod_name Мой мод
//   @author Генри
//   @18+                       for grown-ups only: «18+» in the game's «Моды», asked before it starts
//   : start                    a scene (the mod starts at its first one)
//   фон bg kitchen1_1 dissolve the background (any of the game's: bg kitchen1_1, bg_win_f_0...)
//   показать Мама Normal m_day 01 norm aside   a sprite: who, then the game's own words for it
//   показать Оля Serious m_day 00 good ahead 01 справа быстро   (+ where, + how)
//   убрать Мама                 off the screen
//   Мама: Не подначивай ребёнка.     a line (the game's Character by name)
//   мысли: Мне ведь уже двенадцать.   Anton's thoughts (italic)
//   Всю ночь ветер скрёбся в окно.   no name: the narrator
//   голос voice/karina/21 K.ogg      the voice of the next line
//   музыка 1_Melancholy(Tiny_Bunny) | музыка стоп     фонзвук sounds/fon/001wind.ogg | стоп     звук ...
//   пауза 1.5   окно скрыть|показать   тряска   вспышка
//   выбор / - Взять -> take / - Убежать -> run / конецвыбора
//   выбор подсветка / - ВЗЯТЬ -> take | 1250 700 / ...   the game's own kind of choice: glowing spots on the
//                                                         picture, each with its word (x y of 1920x1080)
//   переход take   установить smelo 1   прибавить smelo 1   если smelo >= 1 -> brave   конецигры
//   камера наезд | 960 400   камера шаги   камера стоп      the game's pseudo-motion: the whole picture moves
//   движение Мама вздрог     движение Оля смеётся   движение Оля стоп      a sprite moves (Motion.h)
//   погода снег | метель | нет        плёнка vhs | помехи | воспоминание | телевизор | нет
//   показать Мама ... слева рост 90   a sprite taller or smaller (percent)
//   записка Я вижу тебя.              a note on paper, until clicked
//   объявление Алиса | ПРОПАЛА ДЕВОЧКА | Ушла из дома и не вернулась.     a "missing" notice with her photo
//   выбор время 5 / - ... / конецвыбора        a choice against the clock: time out - the last one
//   @вставка bunny_day2_semen -> my_scene   a story mod: my_scene plays right before the game's own label
//                                           (it ends with вернуться - the game goes on as it was)
//   @замена sK2_good -> saved               the game's label is played by my scene instead (the story turns)
//   оригинал d4_begin                       jump into the game's own story at its label
//   вернуться                               an inserted scene is over: back to the game
//   код: SK_hard_mode = SK_easy_mode        one line of Python (for those who know the game's code)
//   надпись День пятый | 3                  big words in the middle (the game's own), for 3 s
//   эпилог Прошло десять лет... | Дальше    the scene goes dark: the words, the game's brush button
//   эпилог images/end/kaite/bg.png + images/end/kaite/chara.png | ЖИВА | Текст | Конец
//                                           the game's ending page: its art (layers with +), a heading, words
//   кнопка ОТКРЫТЬ                          one button: the story waits for it
//   борьба Тяни! | 6 -> saved | lost        press faster than it slips: bar full in 6 s - saved, else lost
//   титры Сценарий: Генри | Спасибо, что играли     the end titles rolling over black
//   суефа -> won | lost                     rock-paper-scissors, the game's own hands (a draw - again)
//   суефа Катя | дом ночь -> won | lost     with her hands (Ромка Катя Полина Оля Бяша Алиса Семён Мама Папа
//                                           Учительница Тихонов Харитон «Дед Мороз» Врач Киллер Девушка Мужчина Бабушка
//                                           Лиса Волчик Медвежутка Зайчик Скелет); where: here - the scene blurred (none given), поле,
//                                           дом/школа/двор/гаражи/лес/деревня/поле + день/вечер/ночь, a background
//   снимок Подпись                          the polaroid: raised, a flash, the card of this very shot
//   загадка Вопрос | ответ/другой -> ok | no   the player types the answer
//   осмотр / - Плита | 900 600 | Описание / - Уйти | 1700 900 -> сцена / конецосмотра    spots to look at
//   {словарь=пояснение}слово{/словарь}       a word with a note, shown on paper when clicked
//   плёнка глазок                           the picture through a door peephole
//   телефон / - 31677 -> katya / - неправильно -> nobody / конецтелефона    the player dials (the game's phone)
//   достижение Спаситель | Катя жива        a trophy slides in at the corner (once; remembered for good)
//   камера мышь                             the picture drifts after the mouse
//   предмет images/end/kaite/chara.png | Подпись    the game's hand rises holding a picture, until clicked
//   пазл images/end/kaite/bg.png | 3x3 -> собрал     the picture in pieces on the table (episode 4's poster game)
//   in any line: {дрожь}...{/дрожь} {прыг} {появление} {хаос} {кругом} {курсив} - the game's living text
//   (a story mod's variables live in persistent: points won in episode 2 are still there in episode 4)
#pragma once
#include <QHash>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QVector>

namespace tb {

class Catalog;

struct Step {
    enum Kind {
        Label, Background, Show, Hide, Say, Voice, Music, Ambience, Sound, Pause, Window, Menu, Jump, Set, Add,
        If, End, Shake, Flash, Camera, Move, Weather, Film, Paper, Poster, GameJump, Return, Code,
        Title, Epilogue, Button, Struggle, Credits, Suefa, Snapshot, Riddle, Explore, Phone, Achievement, InHand, Puzzle, Bad
    };
    Kind kind = Bad;
    int line = 0;                // 1-based line in story.txt
    QString a, b, c;             // per kind: Label name; Background name; Show sprite/position;
    QString transition;          //   Say who(var)/text; file; var/value; If var/op/value;
                                 //   Camera move id ("" stop)/"x y"; Move who/move id; Weather, Film: id ("" off);
                                 //   Show c: size percent ("" = 100); Menu a: spots | timed, b: seconds;
                                 //   Paper a: text; Poster a: who, b: title, c: text, transition: the photo's sprite ("" = as shown)
                                 //   Epilogue a: text, b: button, c: heading, transition: art layers joined by "+" 
    QStringList choices, targets;   // Menu
    QStringList places;             // Menu "подсветка": where each choice glows on the picture, "x y" ("" = spread)
    QStringList notes;              // Explore: what each spot says ("" - a way out, its target in targets)
                                    // Phone: choices - the numbers, targets - their scenes; c - the wrong number's ("" the operator)
    QString target;              // Jump / If / Menu
    QString error;               // Bad: what is wrong
};

struct Speaker {                 // a name the game has no Character for: the mod makes one
    QString name, var;
};

struct Story {
    QString id, name, author, description;
    bool adult = false;              // @18+ - for grown-ups only (the game asks «тебе есть 18?» before it starts)
    QVector<Step> steps;
    QStringList labels;          // the scenes, as written
    QVector<Speaker> extraSpeakers;
    QStringList vars;            // the mod's own variables
    // a story mod: hooks into the game's own labels - (the game's label, my scene)
    QVector<QPair<QString, QString>> inserts, replaces;
    bool hooked() const { return !inserts.isEmpty() || !replaces.isEmpty(); }
    QVector<Step> problems() const;   // the Bad ones
};

// what is on the screen after a line
struct SceneState {
    QString bg;                      // background name
    QVector<QPair<QString, QString>> sprites;   // sprite name, position (in show order)
    QString who, whoColor, text;     // the line on the screen ("" none)
    bool thoughts = false;           // italic (Anton's thoughts, the Bunny)
    bool quoted = false;             // in quotes (a character's line; not the narrator's, not Anton's thoughts)
    bool window = true;
    // a choice on the screen: its words; with places - the glowing spots (the game's picture choice)
    QStringList choices;
    QVector<QPoint> spots;
    QString music;
    // the pseudo-motion: the camera's move and the point it looks at; who moves how; the weather; the tape
    QString camera;
    QPoint cameraAt{960, 540};
    QHash<QString, QString> motions;     // sprite owner -> the move that lasts
    QHash<QString, QString> once;        // sprite owner -> the move played once (at its own card only)
    QHash<QString, int> sizes;           // sprite owner -> size percent (none: 100)
    // over the scene at their own card only: a note; a notice with a photo; a choice's seconds
    QString paper;
    QString posterSprite, posterTitle, posterText;
    int timed = 0;
    QString weather, film;
    // full-screen pictures over the scene at their own card only (суефа: the field, the hands); "#fog" - a haze,
    // "#bg:<name>" - a background of the game
    QStringList overlay;
    // a screen of the hub at its own card: title | epilogue | credits | snap | riddle | struggle, and its words
    QString screen;
    QStringList screenArgs;
    int step = -1;
};

Story parseStory(const QString& text, const Catalog& cat);
QString suefaHands(const QString& who);          // суефа: who plays -> the hands' set ("" the game's own, "?" none)
QString suefaPlace(const QString& where);        // суефа: "дом ночь" -> its drawn picture ("" not one of them)
SceneState stateAt(const Story& story, const Catalog& cat, int line);   // after the step at or before line
// the scene that stands for the mod (its cover): the first with someone in it - else its first background;
// no line on it
SceneState coverState(const Story& story, const Catalog& cat);

// helpers shared with the compiler
QString renpyTransition(const QString& t);       // dissolve | Dissolve(0.3) | fade | "" ...
QString labelName(const QString& modId, const QString& scene);   // <mod>__<scene> (latin, safe)
QString varName(const QString& modId, const QString& var);       // <mod>_<var>
// where a choice's spot is: its "x y", or spread across the screen (the i-th of n)
QPoint spotPlace(const QString& place, int i, int n);
QString safeId(const QString& name);                             // a name spelled in latin (a mod's id)
// the story's own words back from what a step holds (the block editor writes lines with them)
QString transitionWord(const QString& renpy);    // dissolve -> плавно, "" -> "" (at once)
QString positionWord(const QString& transform);  // genry_left -> слева; the game's own transform as it is
bool isCommandWord(const QString& word);         // a line starting with it is a command, not a narrator's line

} // namespace tb
