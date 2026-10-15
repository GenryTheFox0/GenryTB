import QtQuick
import QtQuick.Controls
import GenryTB

// The scene as a column of cards, top to bottom as it plays; under it what can be added. A card is moved
// by dragging its grip: a white line shows where it would land.
Item {
    id: root
    property var ed
    property alias list: list

    function reveal(row) { if (row >= 0) list.positionViewAtIndex(row, ListView.Contain) }
    function focusText(row) {
        Qt.callLater(function () {
            list.positionViewAtIndex(row, ListView.Contain)
            var item = list.itemAtIndex(row)
            if (item) item.focusText()
        })
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.bottomMargin: addBar.height + 10
        anchors.leftMargin: 10
        model: Engine.blocks
        spacing: 6
        clip: true
        topMargin: 10
        bottomMargin: 30
        boundsBehavior: Flickable.StopAtBounds
        cacheBuffer: 1200
        ScrollBar.vertical: ScrollBar { }

        property int dragFrom: -1
        property int dragTo: -1            // the row it would become
        property real dragY: 0
        function dragBegin(row) { dragFrom = row; dragTo = row }
        function dragMove(y) {
            dragY = y
            var to = count - 1
            for (var i = 0; i < count; i++) {
                var it = itemAtIndex(i)
                if (!it) continue
                if (y < it.y + it.height / 2) { to = i > dragFrom ? i - 1 : i; break }
            }
            dragTo = Math.max(0, Math.min(count - 1, to))
            // near an edge: the list rolls on
            var vy = y - contentY
            roll.dir = vy < 40 ? -1 : vy > height - 40 ? 1 : 0
        }
        function dragEnd() {
            roll.dir = 0
            var from = dragFrom, to = dragTo
            dragFrom = -1; dragTo = -1
            if (from >= 0 && to >= 0 && from !== to) root.ed.select(Engine.moveBlock(from, to))
        }
        function dragCancel() { roll.dir = 0; dragFrom = -1; dragTo = -1 }
        Timer {
            id: roll
            property int dir: 0
            interval: 16
            repeat: true
            running: dir !== 0
            onTriggered: list.contentY = Math.max(-list.topMargin, Math.min(list.contentHeight - list.height + list.bottomMargin, list.contentY + dir * 9))
        }

        delegate: BlockCard { ed: root.ed; view: list }

        // where the dragged card would land
        Rectangle {
            parent: list.contentItem
            visible: list.dragFrom >= 0 && list.dragTo !== list.dragFrom
            z: 20
            x: 0
            width: list.width - 14
            height: 4
            radius: 2
            color: Theme.white
            y: {
                if (list.dragTo < 0) return 0
                var it = list.itemAtIndex(list.dragTo)
                if (!it) return 0
                return list.dragTo > list.dragFrom ? it.y + it.height + 1 : it.y - 5
            }
        }

        // an empty scene
        Text {
            anchors.centerIn: parent
            visible: list.count === 0
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("В этой сцене пока пусто.\nДобавь первое, что случится: кнопки внизу.")
            color: Theme.faint
            font.family: Theme.ui
            font.pixelSize: 16
        }
    }

    // ---- what can be added (after the selected card): a tab of each kind of thing, its tiles ----
    readonly property var groups: [
        { g: qsTr("Текст"), icon: "💬", color: "#ffd28a", items: [
            { k: "say", t: qsTr("Реплика"), h: qsTr("кто-то говорит (Enter в реплике — следующая)") },
            { k: "thought", t: qsTr("Мысли"), h: qsTr("мысли Антона, курсивом") },
            { k: "narrator", t: qsTr("Рассказчик"), h: qsTr("текст без имени") },
            { k: "note", t: qsTr("Заметка"), h: qsTr("для себя, в игре не видна") } ] },
        { g: qsTr("Картинка"), icon: "🏞️", color: "#8fb8ff", items: [
            { k: "bg", t: qsTr("Фон"), h: qsTr("место действия") },
            { k: "show", t: qsTr("Персонаж"), h: qsTr("кто-то появляется или меняет лицо") },
            { k: "hide", t: qsTr("Уходит"), h: qsTr("персонаж уходит со сцены") },
            { k: "weather", t: qsTr("Погода"), h: qsTr("снег, метель") } ] },
        { g: qsTr("90-е"), icon: "📼", color: "#e9dcae", items: [
            { k: "film", t: qsTr("Плёнка"), h: qsTr("VHS-кассета, старый телевизор, воспоминание, помехи, глазок") },
            { k: "paper", t: qsTr("Записка"), h: qsTr("лист бумаги со словами, пока игрок не щёлкнет") },
            { k: "poster", t: qsTr("Объявление"), h: qsTr("«Пропал ребёнок»: фото персонажа и текст") },
            { k: "inhand", t: qsTr("Предмет в руке"), h: qsTr("рука поднимается снизу и держит картинку: фото, вкладыш, записку") } ] },
        { g: qsTr("Движение"), icon: "🎥", color: "#ff9fd6", items: [
            { k: "camera", t: qsTr("Камера"), h: qsTr("весь кадр движется: наезд, шаги, бег, дыхание…") },
            { k: "move", t: qsTr("Движение"), h: qsTr("персонаж вздрагивает, смеётся, подходит…") },
            { k: "shake", t: qsTr("Тряска"), h: qsTr("экран вздрагивает") },
            { k: "flash", t: qsTr("Вспышка"), h: qsTr("белая вспышка") },
            { k: "pause", t: qsTr("Пауза"), h: qsTr("тишина на секунду") } ] },
        { g: qsTr("Звук"), icon: "🎵", color: "#d7a6ff", items: [
            { k: "music", t: qsTr("Музыка"), h: qsTr("музыка игры") },
            { k: "amb", t: qsTr("Фоновый звук"), h: qsTr("ветер, кухня, улица — по кругу") },
            { k: "sound", t: qsTr("Звук"), h: qsTr("шаги, дверь, стук — один раз") } ] },
        { g: qsTr("История"), icon: "🔀", color: "#ff9a8a", items: [
            { k: "menu", t: qsTr("Выбор"), h: qsTr("игрок выбирает — история расходится") },
            { k: "jump", t: qsTr("Переход"), h: qsTr("дальше — в другую сцену") },
            { k: "add", t: qsTr("Очки"), h: qsTr("прибавить к счётчику (смелость, дружба…)") },
            { k: "if", t: qsTr("Условие"), h: qsTr("если счётчик набрался — в другую сцену") },
            { k: "end", t: qsTr("Конец"), h: qsTr("конец истории") } ] },
        { g: qsTr("Игры"), icon: "✊", color: "#7fe0ff", items: [
            { k: "suefa", t: qsTr("Суефа"), h: qsTr("камень-ножницы-бумага: руки любого персонажа, любое место") },
            { k: "struggle", t: qsTr("Борьба"), h: qsTr("жми быстрее, чем выскальзывает: успел — одна сцена, нет — другая") },
            { k: "riddle", t: qsTr("Загадка"), h: qsTr("игрок вписывает ответ") },
            { k: "explore", t: qsTr("Осмотр"), h: qsTr("точки на картинке: что там, или выход в другую сцену") },
            { k: "snap", t: qsTr("Снимок"), h: qsTr("полароид: вспышка и карточка этого самого кадра") },
            { k: "phone", t: qsTr("Телефон"), h: qsTr("игрок сам набирает номер: каждый номер — своя сцена") },
            { k: "ach", t: qsTr("Достижение"), h: qsTr("кубок всплывает в углу — и запоминается навсегда") },
            { k: "puzzle", t: qsTr("Пазл"), h: qsTr("картинка рвётся на куски — игрок собирает её на столе") } ] },
        { g: qsTr("Кино"), icon: "🎬", color: "#f2c46b", items: [
            { k: "title", t: qsTr("Надпись"), h: qsTr("крупные буквы посреди экрана: «День пятый»") },
            { k: "epilogue", t: qsTr("Эпилог"), h: qsTr("страница концовки: арт, заголовок, текст, кнопка") },
            { k: "credits", t: qsTr("Титры"), h: qsTr("титры плывут по чёрному") },
            { k: "button", t: qsTr("Кнопка"), h: qsTr("история ждёт, пока нажмут") } ] },
        { g: qsTr("В игру"), icon: "🎮", color: "#b0a4ff", items: [
            { k: "game", t: qsTr("В игру"), h: qsTr("дальше — оригинальная история игры с её метки") },
            { k: "return", t: qsTr("Назад в игру"), h: qsTr("вставка кончилась — игра идёт как шла") },
            { k: "code", t: qsTr("Код"), h: qsTr("строка на питоне — для тех, кто знает код игры") } ] }
    ]
    property int tab: 0

    Rectangle {
        id: addBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: tabs.height + tiles.height + 46
        color: Theme.panel
        Rectangle { width: parent.width; height: 1; color: Theme.line }
        Text {
            x: 14; y: 8
            text: qsTr("ДОБАВИТЬ ПОСЛЕ ВЫБРАННОЙ КАРТОЧКИ")
            color: Theme.faint
            font.family: Theme.ui; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.8
        }
        // the tabs
        Flow {
            id: tabs
            x: 12; y: 26
            width: parent.width - 24
            spacing: 5
            Repeater {
                model: root.groups
                Rectangle {
                    id: tabItem
                    required property var modelData
                    required property int index
                    readonly property bool on: root.tab === index
                    width: tabRow.implicitWidth + 20
                    height: 30
                    radius: 15
                    color: on ? modelData.color : ta.containsMouse ? Theme.panel3 : Theme.panel2
                    border.color: on ? modelData.color : Theme.line
                    scale: ta.pressed ? 0.93 : 1
                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on scale { NumberAnimation { duration: 90 } }
                    Row {
                        id: tabRow
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: tabItem.modelData.icon; font.family: Theme.emoji; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                        Text {
                            text: tabItem.modelData.g
                            color: tabItem.on ? "#101010" : Theme.text
                            font.family: Theme.ui; font.pixelSize: 13; font.weight: tabItem.on ? Font.DemiBold : Font.Normal
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    MouseArea { id: ta; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { root.tab = tabItem.index; Engine.click("hover") } }
                }
            }
        }
        // the tiles of the open tab
        Flow {
            id: tiles
            x: 12
            anchors.top: tabs.bottom
            anchors.topMargin: 10
            width: parent.width - 24
            spacing: 8
            Repeater {
                id: tileRep
                model: root.groups[root.tab].items
                Rectangle {
                    id: tile
                    required property var modelData
                    required property int index
                    readonly property color tint: root.groups[root.tab].color
                    width: 128
                    height: 62
                    radius: 10
                    color: tl.containsMouse ? Theme.panel3 : Theme.panel2
                    border.width: tl.containsMouse ? 2 : 1
                    border.color: tl.containsMouse ? tint : Theme.line
                    scale: tl.pressed ? 0.92 : 1
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }
                    transform: Translate { y: tl.containsMouse ? -2 : 0; Behavior on y { NumberAnimation { duration: 120 } } }
                    // tiles of a new tab pop up one after another
                    opacity: 0
                    Component.onCompleted: pop.start()
                    SequentialAnimation {
                        id: pop
                        PauseAnimation { duration: tile.index * 35 }
                        NumberAnimation { target: tile; property: "opacity"; from: 0; to: 1; duration: 140 }
                    }
                    Rectangle { x: 10; y: 0; width: parent.width - 20; height: 3; radius: 1.5; color: tile.tint; opacity: 0.85 }
                    Text { x: 10; y: 10; text: Theme.kindIcon(tile.modelData.k); font.family: Theme.emoji; font.pixelSize: 22 }
                    Text {
                        x: 44; y: 10; width: parent.width - 50
                        text: tile.modelData.t
                        color: Theme.text
                        font.family: Theme.ui; font.pixelSize: 14; font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Text {
                        x: 44; y: 31; width: parent.width - 50
                        text: tile.modelData.h
                        color: Theme.faint
                        font.family: Theme.ui; font.pixelSize: 10
                        wrapMode: Text.Wrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        lineHeight: 0.9
                    }
                    MouseArea {
                        id: tl
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { Engine.click("click"); root.ed.add(tile.modelData.k) }
                    }
                    ToolTip.visible: tl.containsMouse
                    ToolTip.text: tile.modelData.h
                    ToolTip.delay: 600
                }
            }
        }
    }
}
