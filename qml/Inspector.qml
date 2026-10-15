import QtQuick
import QtQuick.Controls
import GenryTB

// Under the preview: what the selected card is made of, as pictures and lists to pick from - a line's
// speaker and voice, a background, a character, a sound. Whatever is picked goes straight into the card.
Rectangle {
    id: root
    property var ed
    readonly property int row: ed.row
    readonly property var blk: ed.blk
    readonly property string kind: ed.kind
    color: Theme.panel

    function set(fields) { Engine.editBlock(root.row, fields) }

    Rectangle { width: parent.width; height: 1; color: Theme.line }
    Text {
        id: title
        x: 12; y: 9
        text: root.kind === "" ? qsTr("ЧТО ЗДЕСЬ") : Theme.kindName(root.kind).toUpperCase() + qsTr(" — ВЫБЕРИ")
        color: root.kind === "" ? Theme.faint : Theme.kindColor(root.kind)
        font.family: Theme.ui; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.8
    }

    Loader {
        anchors.fill: parent
        anchors.topMargin: 26
        sourceComponent: {
            switch (root.kind) {
            case "say": return sayC
            case "bg": return bgC
            case "show": return showC
            case "hide": case "move": case "poster": return hideC
            case "music": case "amb": case "sound": return audioC
            }
            return hintC
        }
    }

    Component {
        id: sayC
        SpeakerPicker {
            value: root.blk.f.key
            who: root.blk.f.who
            voice: root.blk.f.voice || ""
            onPicked: (key) => { root.set({ key: key }); root.ed.focusRow(root.row) }
            onVoicePicked: (file, text) => {
                // a recorded line brings its words too - unless the card already has its own
                var t = root.blk.f.text
                if (file !== "" && (t === "…" || t === "")) root.set({ voice: file, text: text })
                else root.set({ voice: file })
                if (file !== "") Engine.play(file)
            }
        }
    }
    Component {
        id: bgC
        BackgroundPicker { value: root.blk.f.name; onPicked: (name) => root.set({ name: name }) }
    }
    Component {
        id: showC
        CharacterPicker { value: root.blk.f.sprite; onPicked: (sprite) => root.set({ sprite: sprite }) }
    }
    Component {
        id: hideC
        Flow {
            x: 10; y: 6
            width: parent.width - 20
            spacing: 6
            Repeater {
                model: Engine.spriteOwners()
                Rectangle {
                    id: o
                    required property var modelData
                    readonly property bool on: modelData.who === root.blk.f.who
                    width: l.implicitWidth + 46; height: 36; radius: 18
                    color: on ? Theme.white : a.containsMouse ? Theme.panel3 : Theme.panel2
                    border.color: on ? Theme.white : Theme.line
                    Image { x: 3; y: 3; width: 30; height: 30; source: "image://tb/face/72/" + encodeURIComponent(o.modelData.thumb); asynchronous: true; fillMode: Image.PreserveAspectCrop }
                    Text { id: l; x: 38; anchors.verticalCenter: parent.verticalCenter; text: o.modelData.name; color: o.on ? "#101010" : Theme.text; font.family: Theme.ui; font.pixelSize: 13 }
                    MouseArea { id: a; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.set({ who: o.modelData.who }) }
                }
            }
        }
    }
    Component {
        id: audioC
        AudioPicker { mode: root.kind; value: root.blk.f.file; onPicked: (file) => { root.set({ file: file }); Engine.play(file) } }
    }
    Component {
        id: hintC
        Flickable {
            contentHeight: hint.implicitHeight + 20
            clip: true
            Text {
                id: hint
                x: 14; y: 4
                width: parent.width - 28
                wrapMode: Text.WordWrap
                color: Theme.dim
                font.family: Theme.ui
                font.pixelSize: 14
                lineHeight: 1.25
                textFormat: Text.StyledText
                text: {
                    switch (root.kind) {
                    case "menu": return qsTr("<b>Выбор.</b> Игрок видит варианты и выбирает один — история уходит в сцену, на которую показывает стрелка.<br><br>Текст варианта правится прямо в карточке. Порядок — перетащи вариант за точки слева. Сцену «куда» выбери по стрелке: там же можно сразу создать новую.<br><br><b>Подсветка на картинке</b> — выбор как в самой игре («ВЗЯТЬ / ОТКАЗАТЬСЯ»): на сцене светятся пятна со словами, под мышью пятно разгорается. Включи её в карточке и расставь пятна на превью: на конфету в руке, на дверь, на лицо.")
                    case "jump": return qsTr("<b>Переход.</b> История продолжается в другой сцене. Без перехода сцена, дойдя до конца, заканчивает мод.")
                    case "add": case "set": return qsTr("<b>Счётчик.</b> Мод помнит числа: смелость, дружбу с Полиной, сколько конфет взял. «Изменить» прибавляет или убавляет, «Условие» потом смотрит, сколько набралось, и ведёт в нужную сцену.")
                    case "if": return qsTr("<b>Условие.</b> Если счётчик набрал нужное — история уходит в сцену по стрелке. Если нет — идёт дальше по карточкам.")
                    case "end": return qsTr("<b>Конец.</b> Музыка стихает, на чёрном появляется «Конец», и игра возвращается в главное меню.")
                    case "pause": return qsTr("<b>Пауза.</b> Ничего не происходит — секунда тишины перед важной репликой.")
                    case "note": return qsTr("<b>Заметка.</b> Видна только тебе, в игру не попадает.")
                    case "camera": return qsTr("<b>Камера.</b> Так «Зайчик» оживляет неподвижные картинки: двигается весь кадр — фон и все, кто на нём.<br><br><b>Наезд</b> и <b>крупный план</b> — камера идёт к точке: поставь её на превью (кружок с прицелом), на лицо или на дверь.<br><b>Шаги, бег</b> — герой идёт или бежит. <b>Дыхание, сердце, дрожь, качка</b> — настроение кадра.<br><br>Движение держится до карточки «Камера: стоп» или до нового фона. Щёлкни по превью — проиграет заново.")
                    case "paper": return qsTr("<b>Записка.</b> Лист бумаги с нацарапанными словами — висит на экране, пока игрок не щёлкнет. Угроза в почтовом ящике, шпаргалка, письмо.")
                    case "weather": return qsTr("<b>Погода.</b> Снег из самой игры поверх сцены. Идёт до нового фона: новый фон — чистый кадр.")
                    case "film": return qsTr("<b>Плёнка.</b> Девяностые: картинка как с затёртой VHS-кассеты — полосы, цветные края, ползущая помеха. Или телевизионный «снег». Держится, пока не поставишь «обычная»: хорошо для воспоминаний, снов, записи с камеры.<br><br><b>Телевизор</b> — сцена идёт на экране старого телевизора из самой игры. <b>Воспоминание</b> — рамка и царапины старой плёнки: так игра показывает прошлое.")
                    case "bad": return qsTr("<b>Эту строку мод не понимает.</b> Поправь её текст в карточке или убери карточку крестиком.")
                    case "suefa": return qsTr("<b>Камень-ножницы-бумага.</b> Игрок выбирает знак, руки трясутся три раза — и кто победил. Ничья — играют снова. Выбери, чьими руками играет соперник, и где: «здесь» — сама сцена в дымке, или одно из мест днём, вечером, ночью. Выиграл и проиграл — две разные сцены.")
                    case "struggle": return qsTr("<b>Борьба.</b> Полоска выскальзывает — игрок жмёт пробел или кликает, чтобы её удержать. Заполнил до конца — одна сцена, не успел — другая. Так в игре спасают из мясорубки.")
                    case "riddle": return qsTr("<b>Загадка.</b> Игрок вписывает ответ на листке. Верных ответов может быть несколько — через «/»; регистр и «ё» не важны. Угадал — одна сцена, нет — другая.")
                    case "explore": return qsTr("<b>Осмотр.</b> На картинке светятся точки: игрок наводит и щёлкает. «Смотреть» — показать строчку описания и вернуться к осмотру. «Выход» — уйти в другую сцену. Хотя бы один выход нужен, иначе из осмотра не выбраться.")
                    case "snap": return qsTr("<b>Снимок.</b> Антон поднимает полароид, вспышка — и на экране карточка с этим самым кадром и твоей подписью. Как в третьем эпизоде.")
                    case "phone": return qsTr("<b>Телефон.</b> Тот самый кнопочный телефон из пятого эпизода. Игрок сам набирает номер: каждый номер ведёт в свою сцену. Не тот номер — оператор скажет «Неправильно набран номер», или веди в свою сцену. Номера подсказывай заранее: в записке, на объявлении, в реплике.")
                    case "ach": return qsTr("<b>Достижение.</b> В углу выезжает кубок с названием. Каждое — один раз и навсегда: в меню «Моды» видно, сколько собрано.")
                    case "inhand": return qsTr("<b>Предмет в руке.</b> Снизу поднимается рука Антона и держит фотокарточку с любой картинкой игры — как вкладыш от жвачки. Висит, пока игрок не щёлкнет.")
                    case "title": return qsTr("<b>Надпись.</b> Крупные буквы посреди экрана на несколько секунд: «День пятый», «Через год», «Никому не говори».")
                    case "epilogue": return qsTr("<b>Эпилог.</b> Страница концовки, как в самой игре: арт одной из концовок (или затемнённая сцена), заголовок, текст и кнопка. Ставь перед «Концом».")
                    case "credits": return qsTr("<b>Титры.</b> Строчки плывут снизу вверх по чёрному экрану. Каждая строка — отдельная строчка титров. Щелчок — пропустить.")
                    case "button": return qsTr("<b>Кнопка.</b> История останавливается и ждёт, пока игрок нажмёт кнопку с твоей надписью.")
                    case "game": return qsTr("<b>В игру.</b> Дальше идёт оригинальная история «Зайчика» с указанной метки. Метки — в скриптах игры (например d4_begin).")
                    case "return": return qsTr("<b>Назад в игру.</b> Конец вставки: игра продолжается с того места, где мод её прервал. Нужна в конце каждой сцены, вставленной через @вставка.")
                    case "code": return qsTr("<b>Код.</b> Одна строка на Python — для тех, кто знает код игры. Например, изменить переменную игры. Ошибка здесь может уронить игру.")
                    case "puzzle": return qsTr("<b>Пазл.</b> Картинка рвётся на куски и рассыпается по столу из мини-игры с плакатом Полины. Игрок перетаскивает куски — у своего места кусок защёлкивается. Собрал всё — история идёт в сцену по стрелке. Можно собрать сразу кнопкой в углу.")
                    case "shake": case "flash": return qsTr("<b>Эффект.</b> Срабатывает в этот момент истории.")
                    }
                    return qsTr("Выбери карточку слева — здесь появится всё, из чего она собирается: лица, фоны, музыка.<br><br><b>Как писать мод</b><br>• Внизу слева кнопки: «Реплика», «Фон», «Персонаж», «Выбор»… — каждая добавляет карточку после выбранной.<br>• В реплике жми <b>Enter</b> — появится следующая, тем же голосом.<br>• Карточки двигаются: тащи за точки слева.<br>• Сцены — в колонке слева; выборы ведут из сцены в сцену.<br>• <b>F5</b> — запустить мод в игре с начала, <b>F6</b> — с этой сцены.<br>• <b>Ctrl+Z</b> отменяет что угодно.")
                }
            }
        }
    }
}
