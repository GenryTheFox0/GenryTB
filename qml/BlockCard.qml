import QtQuick
import QtQuick.Controls
import GenryTB

// One card of the scene: a line of the story as a thing to look at and change - who says what, a
// background, a character coming in, a choice with its ways out. Drag it by its grip to move it.
Rectangle {
    id: card
    required property int index
    required property string kind
    required property var f
    required property string error
    property var ed                       // the editor (its selected row)
    property var view                     // the list (drag: where it would land)
    readonly property bool selected: ed && ed.row === index
    readonly property color accent: Theme.kindColor(kind)

    width: ListView.view ? ListView.view.width - 14 : 600
    height: body.implicitHeight + 20
    radius: 9
    color: selected ? "#222226" : hover.hovered ? "#19191c" : Theme.panel
    border.color: error !== "" ? Theme.danger : selected ? accent : Theme.line
    border.width: selected ? 2 : 1
    opacity: view && view.dragFrom === index ? 0.35 : 1
    Behavior on color { ColorAnimation { duration: 120 } }
    Behavior on border.color { ColorAnimation { duration: 160 } }

    // the selected one glows in its kind's colour
    Rectangle {
        z: -1
        anchors.fill: parent
        anchors.margins: -5
        radius: parent.radius + 5
        color: "transparent"
        border.width: 5
        border.color: Qt.rgba(card.accent.r, card.accent.g, card.accent.b, 0.22)
        opacity: card.selected ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 180 } }
    }

    // a card just added slides in from the left and fades up
    transform: Translate { id: slide; x: 0 }
    Component.onCompleted: if (card.ed && card.ed.animateRow === card.index) { card.ed.animateRow = -1; appear.start() }
    ParallelAnimation {
        id: appear
        NumberAnimation { target: slide; property: "x"; from: -26; to: 0; duration: 260; easing.type: Easing.OutBack }
        NumberAnimation { target: card; property: "opacity"; from: 0; to: 1; duration: 200 }
    }

    function edit(fields) { Engine.editBlock(index, fields) }
    function focusText() { if (content.item && content.item.focusText) content.item.focusText() }

    HoverHandler { id: hover }
    TapHandler { onTapped: card.ed.select(card.index) }

    // the kind's colour down its left edge
    Rectangle { x: 1; y: 8; width: 4; height: parent.height - 16; radius: 2; color: card.accent }

    // the grip: drag to move the card
    Item {
        id: grip
        x: 8; width: 22; height: parent.height
        Column {
            anchors.centerIn: parent
            spacing: 3
            Repeater {
                model: 3
                Row {
                    spacing: 3
                    Rectangle { width: 3; height: 3; radius: 1.5; color: gripArea.containsMouse || gripArea.pressed ? Theme.white : Theme.faint }
                    Rectangle { width: 3; height: 3; radius: 1.5; color: gripArea.containsMouse || gripArea.pressed ? Theme.white : Theme.faint }
                }
            }
        }
        MouseArea {
            id: gripArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
            preventStealing: true
            onPressed: { card.ed.select(card.index); card.view.dragBegin(card.index) }
            onPositionChanged: (mouse) => { if (pressed) card.view.dragMove(mapToItem(card.view.contentItem, mouse.x, mouse.y).y) }
            onReleased: card.view.dragEnd()
            onCanceled: card.view.dragCancel()
        }
    }

    Column {
        id: body
        x: 36
        y: 10
        width: parent.width - 36 - 62
        spacing: 6
        Row {
            spacing: 8
            Text {
                text: Theme.kindIcon(card.kind)
                font.family: Theme.emoji
                font.pixelSize: 13
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: Theme.kindName(card.kind)
                color: card.accent
                font.family: Theme.ui
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.6
            }
            Text {
                visible: card.error !== ""
                text: card.error
                color: Theme.danger
                font.family: Theme.ui
                font.pixelSize: 12
            }
        }
        Loader {
            id: content
            width: parent.width
            sourceComponent: {
                switch (card.kind) {
                case "say": return sayC
                case "bg": return bgC
                case "show": return showC
                case "hide": return hideC
                case "music": case "amb": case "sound": case "voice": return audioC
                case "pause": return pauseC
                case "menu": return menuC
                case "jump": return jumpC
                case "set": case "add": return varC
                case "if": return ifC
                case "note": return noteC
                case "bad": return badC
                case "window": return windowC
                case "camera": return cameraC
                case "move": return moveC
                case "weather": return weatherC
                case "film": return filmC
                case "paper": return paperC
                case "poster": return posterC
                case "title": return titleC
                case "epilogue": return epilogueC
                case "button": return buttonC
                case "struggle": return struggleC
                case "credits": return creditsC
                case "snap": return snapC
                case "suefa": return suefaC
                case "riddle": return riddleC
                case "explore": return exploreC
                case "code": return codeC
                case "game": return gameC
                case "phone": return phoneC
                case "ach": return achC
                case "inhand": return inhandC
                case "puzzle": return puzzleC
                }
                return plainC
            }
        }
    }

    // on the right: a copy, away
    Row {
        anchors.right: parent.right
        anchors.rightMargin: 8
        y: 6
        spacing: 2
        opacity: hover.hovered || card.selected ? 1 : 0
        Repeater {
            model: [{ t: "⧉", tip: qsTr("копия"), fn: 0 }, { t: "✕", tip: qsTr("убрать"), fn: 1 }]
            Rectangle {
                required property var modelData
                width: 24; height: 24; radius: 6
                color: a.containsMouse ? (modelData.fn === 1 ? "#5a2626" : Theme.panel3) : "transparent"
                Text { anchors.centerIn: parent; text: parent.modelData.t; color: Theme.dim; font.pixelSize: 13 }
                MouseArea {
                    id: a
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (parent.modelData.fn === 0) card.ed.select(Engine.duplicateBlock(card.index))
                        else card.ed.removeRow(card.index)
                    }
                }
            }
        }
    }

    // ---- what each kind of card shows ----
    Component {
        id: sayC
        Item {
            implicitHeight: Math.max(who.height, ta.implicitHeight)
            function focusText() { ta.forceActiveFocus(); if (ta.text === "…") ta.selectAll(); else ta.cursorPosition = ta.length }
            Rectangle {
                id: who
                width: Math.min(170, whoText.implicitWidth + (face.visible ? 44 : 22))
                height: 30
                radius: 15
                color: card.f.key === "" ? Theme.panel3 : card.f.key === "мысли" ? "#3a3350" : "#4a3f28"
                Image {
                    id: face
                    x: 3; y: 3; width: 24; height: 24
                    visible: source != ""
                    source: card.ed.faceOf(card.f.key)
                    asynchronous: true
                    fillMode: Image.PreserveAspectCrop
                }
                Text {
                    id: whoText
                    x: face.visible ? 32 : 11
                    width: parent.width - x - 8
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    text: qsTr(card.f.who)
                    color: card.f.key === "" ? Theme.dim : card.f.key === "мысли" ? "#cdbcff" : "#ffd28a"
                    font.family: Theme.ui
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: card.ed.select(card.index) }
            }
            TextArea {
                id: ta
                x: who.width + 10
                width: parent.width - x - (voice.visible ? voice.width + 8 : 0)
                wrapMode: TextEdit.Wrap
                text: card.f.text
                font.family: Theme.ui
                font.pixelSize: 15
                font.italic: card.f.key === "мысли"
                color: Theme.text
                selectionColor: "#ffffff"
                selectedTextColor: "#101010"
                selectByMouse: true
                padding: 4
                topPadding: 5
                background: Rectangle { radius: 6; color: ta.activeFocus ? Theme.bg : "transparent"; border.color: ta.activeFocus ? "#55555c" : "transparent" }
                onTextChanged: if (activeFocus && ta.text !== card.f.text) card.edit({ text: ta.text })
                onActiveFocusChanged: if (activeFocus) card.ed.select(card.index)
                // Enter: the next line of the talk (Shift+Enter is not a new line: a line of the story is one line)
                Keys.onReturnPressed: card.ed.add("say")
                Keys.onEnterPressed: card.ed.add("say")
            }
            Rectangle {
                id: voice
                visible: card.f.voice !== undefined && card.f.voice !== ""
                anchors.right: parent.right
                width: 58; height: 26; radius: 13
                y: 2
                color: Engine.playing === card.f.voice ? "#d7a6ff" : "#3b2f4a"
                Text {
                    anchors.centerIn: parent
                    text: Engine.playing === card.f.voice ? qsTr("■ голос") : qsTr("▶ голос")
                    color: Engine.playing === card.f.voice ? "#101010" : "#e2c8ff"
                    font.family: Theme.ui
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Engine.play(card.f.voice) }
            }
        }
    }

    Component {
        id: bgC
        Row {
            spacing: 12
            Rectangle {
                width: 128; height: 72; radius: 5; color: "#000"; clip: true
                Image {
                    anchors.fill: parent
                    source: "image://tb/bg/256/" + encodeURIComponent(card.f.name)
                    asynchronous: true
                    fillMode: Image.PreserveAspectCrop
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: card.ed.select(card.index) }
            }
            Column {
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter
                Text { text: card.f.name; color: Theme.text; font.family: Theme.ui; font.pixelSize: 15 }
                Row {
                    spacing: 6
                    Repeater {
                        model: [["", qsTr("сразу")], ["плавно", qsTr("плавно")], ["затемнение", qsTr("через темноту")], ["медленно", qsTr("медленно")]]
                        Chip { required property var modelData; text: modelData[1]; on: card.f.tr === modelData[0]; onClicked: card.edit({ tr: modelData[0] }) }
                    }
                }
            }
        }
    }

    Component {
        id: showC
        Row {
            spacing: 12
            Rectangle {
                width: 70; height: 92; radius: 5; color: "#0e0e10"; clip: true
                Image {
                    anchors.fill: parent
                    anchors.margins: 2
                    source: "image://tb/sprite/180/" + encodeURIComponent(card.f.sprite)
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    verticalAlignment: Image.AlignBottom
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: card.ed.select(card.index) }
            }
            Column {
                spacing: 7
                anchors.verticalCenter: parent.verticalCenter
                Row {
                    spacing: 8
                    Text { text: qsTr(card.f.whoName); color: Theme.text; font.family: Theme.ui; font.pixelSize: 15; font.weight: Font.DemiBold }
                    Text { text: card.f.sprite.substring(card.f.who.length + 1); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 13; height: 20; verticalAlignment: Text.AlignBottom }
                }
                Row {
                    spacing: 6
                    Text { text: qsTr("где"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                    Repeater {
                        model: [["слева", qsTr("слева")], ["", qsTr("по центру")], ["справа", qsTr("справа")], ["далеко", qsTr("дальше")], ["близко", qsTr("ближе")]]
                        Chip { required property var modelData; text: modelData[1]; on: card.f.pos === modelData[0]; onClicked: card.edit({ pos: modelData[0] }) }
                    }
                }
                Row {
                    spacing: 6
                    Text { text: qsTr("как"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                    Repeater {
                        model: [["", qsTr("сразу")], ["быстро", qsTr("быстро")], ["плавно", qsTr("плавно")], ["медленно", qsTr("медленно")]]
                        Chip { required property var modelData; text: modelData[1]; on: card.f.tr === modelData[0]; onClicked: card.edit({ tr: modelData[0] }) }
                    }
                }
                // taller or smaller than the game draws it (percent of its own size)
                Row {
                    spacing: 6
                    readonly property int size: card.f.size !== undefined ? card.f.size : 100
                    Text { text: qsTr("рост"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                    Chip { text: "−"; hint: qsTr("ниже"); onClicked: { card.ed.select(card.index); card.edit({ size: Math.max(50, parent.size - 5) }) } }
                    Text { width: 46; height: 28; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; text: parent.size + "%"; color: parent.size === 100 ? Theme.dim : Theme.white; font.family: Theme.ui; font.pixelSize: 14; font.weight: Font.DemiBold }
                    Chip { text: "+"; hint: qsTr("выше"); onClicked: { card.ed.select(card.index); card.edit({ size: Math.min(160, parent.size + 5) }) } }
                    Chip { visible: parent.size !== 100; text: qsTr("как в игре"); onClicked: card.edit({ size: 100 }) }
                }
            }
        }
    }

    Component {
        id: hideC
        Row {
            spacing: 10
            Text { text: qsTr(card.f.whoName); color: Theme.text; font.family: Theme.ui; font.pixelSize: 15; font.weight: Font.DemiBold; height: 28; verticalAlignment: Text.AlignVCenter }
            Text { text: qsTr("уходит"); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 14; height: 28; verticalAlignment: Text.AlignVCenter }
            Repeater {
                model: [["", qsTr("сразу")], ["быстро", qsTr("быстро")], ["плавно", qsTr("плавно")]]
                Chip { required property var modelData; text: modelData[1]; on: card.f.tr === modelData[0]; onClicked: card.edit({ tr: modelData[0] }) }
            }
        }
    }

    Component {
        id: audioC
        Row {
            spacing: 10
            Rectangle {
                width: 30; height: 30; radius: 15
                visible: card.f.file !== ""
                color: Engine.playing === card.f.file ? "#d7a6ff" : Theme.panel3
                Text { anchors.centerIn: parent; text: Engine.playing === card.f.file ? "■" : "▶"; color: Engine.playing === card.f.file ? "#101010" : Theme.text; font.pixelSize: 12 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Engine.play(card.f.file) }
            }
            Text {
                height: 30; verticalAlignment: Text.AlignVCenter
                text: card.f.file === "" ? qsTr("тишина — ") + (card.kind === "music" ? qsTr("музыка стихает") : qsTr("звук стихает")) : (card.f.name !== undefined ? card.f.name : card.f.file)
                color: card.f.file === "" ? Theme.dim : Theme.text
                font.family: Theme.ui; font.pixelSize: 15
            }
            Chip {
                visible: card.kind === "music" || card.kind === "amb"
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("стоп")
                on: card.f.file === ""
                onClicked: card.edit({ file: card.f.file === "" ? card.ed.firstAudio(card.kind) : "" })
            }
        }
    }

    Component {
        id: pauseC
        Row {
            spacing: 8
            Repeater {
                model: [0.5, 1, 2, 3, 5]
                Chip { required property var modelData; text: modelData + qsTr(" с"); on: card.f.sec === modelData; onClicked: card.edit({ sec: modelData }) }
            }
        }
    }

    Component {
        id: windowC
        Row {
            spacing: 8
            Chip { text: qsTr("показать"); on: card.f.on === true; onClicked: card.edit({ on: true }) }
            Chip { text: qsTr("спрятать"); on: card.f.on === false; onClicked: card.edit({ on: false }) }
        }
    }

    Component {
        id: menuC
        Column {
            id: menuCol
            spacing: 6
            property int dragFrom: -1
            property int dragTo: -1
            readonly property bool spots: card.f.style === "spots"
            readonly property bool timed: card.f.style === "timed"
            function options() { var o = []; for (var i = 0; i < card.f.options.length; i++) o.push({ text: card.f.options[i].text, target: card.f.options[i].target, x: card.f.options[i].x, y: card.f.options[i].y }); return o }
            function setOpt(i, key, value) { var o = options(); o[i][key] = value; card.edit({ options: o }) }
            Row {
                spacing: 6
                Chip { text: qsTr("списком"); on: !menuCol.spots && !menuCol.timed; onClicked: card.edit({ style: "" }) }
                Chip { text: qsTr("подсветка на картинке"); hint: qsTr("как в игре: светящиеся пятна на сцене"); on: menuCol.spots; onClicked: { card.ed.select(card.index); card.edit({ style: "spots" }) } }
                Chip { text: qsTr("на время"); hint: qsTr("полоска сгорает; не успел — срабатывает последний вариант"); on: menuCol.timed; onClicked: { card.ed.select(card.index); card.edit({ style: "timed" }) } }
                Text {
                    visible: menuCol.spots
                    height: 28; verticalAlignment: Text.AlignVCenter
                    text: qsTr("— пятна двигай мышью прямо на превью")
                    color: Theme.dim; font.family: Theme.ui; font.pixelSize: 12
                }
            }
            Row {
                visible: menuCol.timed
                spacing: 6
                Text { text: qsTr("секунд"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                Repeater {
                    model: [3, 5, 8, 10, 15]
                    Chip { required property var modelData; text: "" + modelData; on: card.f.seconds === modelData; onClicked: card.edit({ seconds: modelData }) }
                }
                Text { text: qsTr("— не успел: последний вариант"); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
            }
            Repeater {
                model: card.f.options
                Item {
                    id: opt
                    required property var modelData
                    required property int index
                    width: menuCol.width
                    height: 34
                    opacity: menuCol.dragFrom === index ? 0.35 : 1
                    // where the dragged one would land
                    Rectangle { visible: menuCol.dragTo === opt.index && menuCol.dragFrom >= 0 && menuCol.dragFrom !== opt.index; y: menuCol.dragTo > menuCol.dragFrom ? parent.height + 1 : -4; width: parent.width; height: 3; radius: 1.5; color: Theme.white; z: 5 }
                    Rectangle {
                        width: 20; height: 34; color: "transparent"
                        Text { anchors.centerIn: parent; text: "⋮⋮"; color: og.containsMouse || og.pressed ? Theme.white : Theme.faint; font.pixelSize: 13 }
                        MouseArea {
                            id: og
                            anchors.fill: parent
                            hoverEnabled: true
                            preventStealing: true
                            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                            onPressed: { card.ed.select(card.index); menuCol.dragFrom = opt.index; menuCol.dragTo = opt.index }
                            onPositionChanged: (mouse) => {
                                if (!pressed) return
                                var y = mapToItem(menuCol, mouse.x, mouse.y).y
                                menuCol.dragTo = Math.max(0, Math.min(card.f.options.length - 1, Math.floor(y / 40)))
                            }
                            onReleased: {
                                var from = menuCol.dragFrom, to = menuCol.dragTo
                                menuCol.dragFrom = -1; menuCol.dragTo = -1
                                if (from >= 0 && to >= 0 && from !== to) { var o = menuCol.options(); var m = o.splice(from, 1)[0]; o.splice(to, 0, m); card.edit({ options: o }) }
                            }
                            onCanceled: { menuCol.dragFrom = -1; menuCol.dragTo = -1 }
                        }
                    }
                    TField {
                        id: optText
                        x: 24
                        width: parent.width - 24 - picker.width - 40
                        text: opt.modelData.text
                        font.pixelSize: 15
                        onTextEdited: menuCol.setOpt(opt.index, "text", text)
                        onActiveFocusChanged: if (activeFocus) card.ed.select(card.index)
                    }
                    ScenePicker {
                        id: picker
                        anchors.right: del.left
                        anchors.rightMargin: 6
                        y: 2
                        value: opt.modelData.target
                        onPicked: (name) => menuCol.setOpt(opt.index, "target", name)
                    }
                    Rectangle {
                        id: del
                        anchors.right: parent.right
                        y: 5; width: 24; height: 24; radius: 6
                        visible: card.f.options.length > 1
                        color: da.containsMouse ? "#5a2626" : "transparent"
                        Text { anchors.centerIn: parent; text: "✕"; color: Theme.dim; font.pixelSize: 12 }
                        MouseArea { id: da; anchors.fill: parent; hoverEnabled: true; onClicked: { var o = menuCol.options(); o.splice(opt.index, 1); card.edit({ options: o }) } }
                    }
                }
            }
            TButton {
                text: qsTr("＋ вариант")
                onClicked: { var o = menuCol.options(); o.push({ text: qsTr("Ещё вариант"), target: o.length ? o[o.length - 1].target : Engine.sceneNames()[0], x: 960, y: 300 + 120 * (o.length % 5) }); card.edit({ options: o }) }
            }
        }
    }

    Component {
        id: jumpC
        Row {
            spacing: 8
            Text { text: qsTr("дальше —"); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 14; height: 30; verticalAlignment: Text.AlignVCenter }
            ScenePicker { value: card.f.target; onPicked: (name) => card.edit({ target: name }) }
        }
    }

    Component {
        id: varC
        Row {
            spacing: 8
            Text { text: card.kind === "set" ? qsTr("запомнить") : qsTr("изменить"); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 14; height: 32; verticalAlignment: Text.AlignVCenter }
            TField { width: 180; text: card.f.var; onTextEdited: if (text.trim() !== "") card.edit({ "var": text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
            Text { text: card.kind === "set" ? "=" : qsTr("на"); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 14; height: 32; verticalAlignment: Text.AlignVCenter }
            TField {
                width: 70
                text: (card.kind === "add" && card.f.value > 0 ? "+" : "") + card.f.value
                validator: RegularExpressionValidator { regularExpression: /[+-]?\d{0,6}/ }
                onTextEdited: if (text !== "" && text !== "-" && text !== "+") card.edit({ value: Number(text) })
            }
        }
    }

    Component {
        id: ifC
        Row {
            spacing: 8
            Text { text: qsTr("если"); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 14; height: 32; verticalAlignment: Text.AlignVCenter }
            TField { width: 160; text: card.f.var; onTextEdited: if (text.trim() !== "") card.edit({ "var": text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
            Repeater {
                model: [[">=", qsTr("не меньше")], [">", qsTr("больше")], ["==", qsTr("ровно")], ["<", qsTr("меньше")]]
                Chip { required property var modelData; anchors.verticalCenter: parent.verticalCenter; text: modelData[1]; on: card.f.op === modelData[0]; onClicked: card.edit({ op: modelData[0] }) }
            }
            TField {
                width: 60
                text: "" + card.f.value
                validator: RegularExpressionValidator { regularExpression: /-?\d{0,6}/ }
                onTextEdited: if (text !== "" && text !== "-") card.edit({ value: Number(text) })
            }
            ScenePicker { y: 1; value: card.f.target; onPicked: (name) => card.edit({ target: name }) }
        }
    }

    Component {
        id: noteC
        TField {
            text: card.f.text
            color: Theme.dim
            font.italic: true
            onTextEdited: card.edit({ text: text })
            onActiveFocusChanged: if (activeFocus) card.ed.select(card.index)
        }
    }

    Component {
        id: badC
        TField {
            text: card.f.raw
            onTextEdited: card.edit({ raw: text })
            onActiveFocusChanged: if (activeFocus) card.ed.select(card.index)
        }
    }

    // the camera: the whole picture moves, as the game does it (show layer master: ...)
    Component {
        id: cameraC
        Column {
            id: camCol
            spacing: 7
            readonly property var all: Engine.motionList(true)
            function group(g) { return all.filter(function (m) { return m.group === g }) }
            function def(word) { for (var i = 0; i < all.length; i++) if (all[i].word === word) return all[i]; return null }
            // not selected: what it does, in a line; selected: everything it can do
            Row {
                visible: !card.selected
                spacing: 8
                readonly property var d: camCol.def(card.f.how)
                Text { text: parent.d ? qsTr(parent.d.title) : qsTr("стоп"); color: Theme.text; font.family: Theme.ui; font.pixelSize: 15; font.weight: Font.DemiBold }
                Text { text: "— " + (parent.d ? qsTr(parent.d.hint) : qsTr("кадр снова стоит ровно")); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 14 }
            }
            Repeater {
                model: card.selected ? ["объектив", "живой кадр"] : []
                Row {
                    id: camRow
                    required property string modelData
                    spacing: 6
                    Text { width: 76; height: 28; verticalAlignment: Text.AlignVCenter; text: qsTr(camRow.modelData); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12 }
                    Flow {
                        width: camCol.width - 82
                        spacing: 6
                        Repeater {
                            model: camCol.group(camRow.modelData)
                            Chip {
                                required property var modelData
                                text: qsTr(modelData.title)
                                hint: qsTr(modelData.hint)
                                on: card.f.how === modelData.word
                                onClicked: { card.ed.select(card.index); card.edit({ how: modelData.word }) }
                            }
                        }
                    }
                }
            }
            Row {
                visible: card.selected
                spacing: 6
                Item { width: 76; height: 28 }
                Chip { text: qsTr("стоп"); hint: qsTr("кадр снова стоит ровно"); on: card.f.how === ""; onClicked: { card.ed.select(card.index); card.edit({ how: "" }) } }
                Text {
                    visible: card.f.point === true
                    height: 28; verticalAlignment: Text.AlignVCenter
                    text: qsTr("— куда смотрит камера, двигай мышью на превью")
                    color: Theme.dim; font.family: Theme.ui; font.pixelSize: 12
                }
            }
        }
    }

    // a sprite moves: flinches, laughs, steps closer
    Component {
        id: moveC
        Column {
            id: mvCol
            spacing: 7
            readonly property var all: Engine.motionList(false)
            function group(g) { return all.filter(function (m) { return m.group === g }) }
            function def(word) { for (var i = 0; i < all.length; i++) if (all[i].word === word) return all[i]; return null }
            Row {
                spacing: 8
                readonly property var d: mvCol.def(card.f.how)
                Text { text: qsTr(card.f.whoName); color: Theme.text; font.family: Theme.ui; font.pixelSize: 15; font.weight: Font.DemiBold; height: 24; verticalAlignment: Text.AlignVCenter }
                Text {
                    text: card.selected ? qsTr("— кто двигается, выбирается справа") : (parent.d ? qsTr(parent.d.title) + " — " + qsTr(parent.d.hint) : qsTr("перестаёт двигаться"))
                    color: card.selected ? Theme.faint : Theme.dim
                    font.family: Theme.ui; font.pixelSize: card.selected ? 12 : 14
                    height: 24; verticalAlignment: Text.AlignVCenter
                }
            }
            Repeater {
                model: card.selected ? ["один раз", "всё время"] : []
                Row {
                    id: mvRow
                    required property string modelData
                    spacing: 6
                    Text { width: 76; height: 28; verticalAlignment: Text.AlignVCenter; text: qsTr(mvRow.modelData); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12 }
                    Flow {
                        width: mvCol.width - 82
                        spacing: 6
                        Repeater {
                            model: mvCol.group(mvRow.modelData)
                            Chip {
                                required property var modelData
                                text: qsTr(modelData.title)
                                hint: qsTr(modelData.hint)
                                on: card.f.how === modelData.word
                                onClicked: { card.ed.select(card.index); card.edit({ how: modelData.word }); card.ed.replay() }
                            }
                        }
                    }
                }
            }
            Row {
                visible: card.selected
                spacing: 6
                Item { width: 76; height: 28 }
                Chip { text: qsTr("стоп"); hint: qsTr("перестаёт двигаться"); on: card.f.how === ""; onClicked: { card.ed.select(card.index); card.edit({ how: "" }) } }
            }
        }
    }

    Component {
        id: weatherC
        Row {
            spacing: 6
            Repeater {
                model: [["снег", qsTr("снег"), qsTr("тихий снегопад")], ["метель", qsTr("метель"), qsTr("снег несёт ветром")], ["", qsTr("ясно"), qsTr("снег перестаёт")]]
                Chip { required property var modelData; text: modelData[1]; hint: modelData[2]; on: card.f.how === modelData[0]; onClicked: { card.ed.select(card.index); card.edit({ how: modelData[0] }) } }
            }
            Text { height: 28; verticalAlignment: Text.AlignVCenter; text: qsTr("— идёт до нового фона"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12 }
        }
    }

    Component {
        id: filmC
        Row {
            spacing: 6
            Repeater {
                model: [["vhs", qsTr("VHS-кассета"), qsTr("картинка как с затёртой видеокассеты")], ["телевизор", qsTr("телевизор"), qsTr("сцена на экране старого телевизора")],
                        ["воспоминание", qsTr("воспоминание"), qsTr("рамка и царапины старой плёнки — как воспоминания в игре")],
                        ["помехи", qsTr("помехи"), qsTr("телевизионный «снег»")], ["", qsTr("обычная"), qsTr("плёнка выключена")]]
                Chip { required property var modelData; text: modelData[1]; hint: modelData[2]; on: card.f.how === modelData[0]; onClicked: { card.ed.select(card.index); card.edit({ how: modelData[0] }) } }
            }
        }
    }

    // a note on paper
    Component {
        id: paperC
        TField {
            function focusText() { forceActiveFocus() }
            text: card.f.text
            font.pixelSize: 15
            placeholderText: qsTr("что написано в записке")
            onTextEdited: card.edit({ text: text })
            onActiveFocusChanged: if (activeFocus) card.ed.select(card.index)
        }
    }

    // a "missing" notice: whose photo (picked on the right), the headline, the words
    Component {
        id: posterC
        Column {
            spacing: 6
            Row {
                spacing: 8
                Text { text: qsTr("фото:"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12; height: 30; verticalAlignment: Text.AlignVCenter }
                Text { text: qsTr(card.f.whoName); color: Theme.text; font.family: Theme.ui; font.pixelSize: 15; font.weight: Font.DemiBold; height: 30; verticalAlignment: Text.AlignVCenter }
                TField {
                    width: 300
                    text: card.f.title
                    font.pixelSize: 15
                    placeholderText: qsTr("заголовок")
                    onTextEdited: card.edit({ title: text })
                    onActiveFocusChanged: if (activeFocus) card.ed.select(card.index)
                }
            }
            TField {
                width: parent.width
                text: card.f.text
                placeholderText: qsTr("приметы, когда пропал, куда сообщить")
                onTextEdited: card.edit({ text: text })
                onActiveFocusChanged: if (activeFocus) card.ed.select(card.index)
            }
        }
    }


    // ---- the mechanics ----
    component Lbl: Text { color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12; height: 30; verticalAlignment: Text.AlignVCenter }

    // big words in the middle of the screen
    Component {
        id: titleC
        Row {
            spacing: 8
            function focusText() { tt.forceActiveFocus() }
            TField { id: tt; width: 320; text: card.f.text; font.pixelSize: 16; placeholderText: qsTr("День пятый"); onTextEdited: card.edit({ text: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
            Lbl { text: qsTr("держать") }
            Repeater {
                model: [2, 3, 4, 6]
                Chip { required property var modelData; anchors.verticalCenter: parent.verticalCenter; text: modelData + qsTr(" с"); on: card.f.sec === modelData; onClicked: card.edit({ sec: modelData }) }
            }
        }
    }

    // an ending page: the art (the game's endings, as pictures), a heading, the words, the button
    Component {
        id: epilogueC
        Column {
            id: epCol
            spacing: 7
            readonly property var arts: [
                ["", qsTr("без арта"), ""],
                ["images/end/kaite/bg.png + images/end/kaite/chara.png", qsTr("Катя"), "images/end/kaite/chara.png"],
                ["images/end/runaway.png", qsTr("Побег"), "images/end/end_mini/runaway.png"],
                ["images/end/rage.png", qsTr("Ярость"), "images/end/end_mini/rage.png"],
                ["images/end/past.png", qsTr("Прошлое"), "images/end/end_mini/past.png"],
                ["images/end/still_water.png", qsTr("Тихий омут"), "images/end/end_mini/still_water.png"],
                ["images/end/was_there.png", qsTr("Он был там"), "images/end/end_mini/was_there.png"],
                ["images/end/traces.png", qsTr("Следы"), "images/end/end_mini/traces.png"],
                ["images/end/hunt.png", qsTr("Охота"), "images/end/end_mini/hunt.png"],
                ["images/end/in_dark.png", qsTr("Во тьме"), "images/end/end_mini/in_dark.png"],
                ["images/end/nightmare.png", qsTr("Кошмар"), "images/end/end_mini/nightmare.png"],
                ["images/end/mad_world.png", qsTr("Безумный мир"), "images/end/end_mini/mad_world.png"],
                ["images/end/genius.png", qsTr("Гений"), "images/end/end_mini/genius.png"],
                ["images/end/end.png", qsTr("Конец"), "images/end/end_mini/end.png"]]
            function focusText() { et.forceActiveFocus() }
            Flow {
                visible: card.selected
                width: parent.width
                spacing: 6
                Repeater {
                    model: epCol.arts
                    Rectangle {
                        required property var modelData
                        width: 96; height: 64; radius: 6; clip: true
                        color: "#0e0e10"
                        border.width: 2
                        border.color: card.f.art === modelData[0] ? Theme.white : ea.containsMouse ? Theme.dim : "transparent"
                        Image { anchors.fill: parent; anchors.margins: 2; visible: parent.modelData[2] !== ""; source: parent.modelData[2] !== "" ? "image://tb/file/" + parent.modelData[2] : ""; sourceSize.width: 180; asynchronous: true; fillMode: Image.PreserveAspectCrop }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 18; color: "#000000aa" }
                        Text { anchors.bottom: parent.bottom; anchors.bottomMargin: 2; anchors.horizontalCenter: parent.horizontalCenter; text: parent.modelData[1]; color: Theme.text; font.family: Theme.ui; font.pixelSize: 11 }
                        MouseArea { id: ea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { card.ed.select(card.index); card.edit({ art: parent.modelData[0] }) } }
                    }
                }
            }
            Row {
                spacing: 8
                TField { width: 240; text: card.f.heading; font.pixelSize: 15; placeholderText: qsTr("ЗАГОЛОВОК"); onTextEdited: card.edit({ heading: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
                Lbl { text: qsTr("кнопка") }
                TField { width: 140; text: card.f.button; placeholderText: qsTr("Конец"); onTextEdited: card.edit({ button: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
            }
            TField { id: et; width: parent.width; text: card.f.text; placeholderText: qsTr("Прошло десять лет…"); onTextEdited: card.edit({ text: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
        }
    }

    Component {
        id: buttonC
        Row {
            spacing: 8
            function focusText() { bt.forceActiveFocus() }
            Lbl { text: qsTr("на кнопке") }
            TField { id: bt; width: 220; text: card.f.text; font.pixelSize: 15; onTextEdited: card.edit({ text: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
        }
    }

    // press faster than it slips away
    Component {
        id: struggleC
        Column {
            spacing: 6
            function focusText() { st.forceActiveFocus() }
            Row {
                spacing: 8
                TField { id: st; width: 220; text: card.f.text; font.pixelSize: 16; placeholderText: qsTr("Тяни!"); onTextEdited: card.edit({ text: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
                Lbl { text: qsTr("секунд") }
                Repeater {
                    model: [4, 6, 8, 10]
                    Chip { required property var modelData; anchors.verticalCenter: parent.verticalCenter; text: "" + modelData; on: card.f.sec === modelData; onClicked: card.edit({ sec: modelData }) }
                }
            }
            Row {
                spacing: 8
                Lbl { text: qsTr("успел —") }
                ScenePicker { value: card.f.win; onPicked: (name) => card.edit({ win: name }) }
                Lbl { text: qsTr("не успел —") }
                ScenePicker { value: card.f.lose; onPicked: (name) => card.edit({ lose: name }) }
            }
        }
    }

    // the end titles: a line each
    Component {
        id: creditsC
        TextArea {
            function focusText() { forceActiveFocus() }
            wrapMode: TextEdit.Wrap
            text: card.f.text
            font.family: Theme.ui
            font.pixelSize: 15
            color: Theme.text
            placeholderText: qsTr("по строчке: кто что делал")
            selectByMouse: true
            background: Rectangle { radius: 6; color: parent.activeFocus ? Theme.bg : Theme.panel2; border.color: parent.activeFocus ? "#55555c" : Theme.line }
            onTextChanged: if (activeFocus && text !== card.f.text) card.edit({ text: text })
            onActiveFocusChanged: if (activeFocus) card.ed.select(card.index)
        }
    }

    Component {
        id: snapC
        Row {
            spacing: 8
            function focusText() { sn.forceActiveFocus() }
            Lbl { text: qsTr("подпись на карточке") }
            TField { id: sn; width: 300; text: card.f.text; font.pixelSize: 15; placeholderText: qsTr("Мы с Катей, зима"); onTextEdited: card.edit({ text: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
        }
    }

    // rock-paper-scissors: whose hands (pictures), where (pictures), when; won / lost
    Component {
        id: suefaC
        Column {
            id: sfCol
            spacing: 7
            readonly property var hands: [
                ["Ромка", "romka"], ["Катя", "katya"], ["Полина", "polina"], ["Оля", "olya"], ["Бяша", "byasha"], ["Алиса", "alisa"],
                ["Семён", "semyon"], ["Мама", "mama"], ["Папа", "papa"], ["Учительница", "uchitelnica"], ["Тихонов", "tikhonov"],
                ["Харитон", "hariton"], ["Дед Мороз", "dedmoroz"], ["Врач", "vrach"], ["Киллер", "killer"], ["Девушка", "devushka"],
                ["Мужчина", "muzhchina"], ["Бабушка", "babushka"], ["Лиса", "lisa"], ["Волчик", "volchik"], ["Медвежутка", "medvezhutka"],
                ["Зайчик", "zaichik"], ["Скелет", "skelet"]]
            readonly property var places: [["здесь", ""], ["поле", "pole"], ["дом", "dom"], ["школа", "shkola"], ["двор", "dvor"],
                                           ["гаражи", "garazhi"], ["лес", "les"], ["деревня", "derevnya"]]
            readonly property var times: [["день", "den"], ["вечер", "vecher"], ["ночь", "noch"]]
            function timeKey() { for (var i = 0; i < times.length; i++) if (times[i][0] === card.f.time) return times[i][1]; return "den" }
            function placeThumb(key) {
                if (key === "") return ""
                if (key === "pole" && timeKey() === "den") return "image://tb/file/interface/suefa/fon.png"
                return "image://tb/file/mods/genry_assets/suefa_bg/" + key + "_" + timeKey() + ".jpg"
            }
            // not selected: who, where, when in a line
            Row {
                visible: !card.selected
                spacing: 10
                Image { width: 72; height: 40; source: "image://tb/file/mods/genry_assets/hands/" + card.f.hands + "/thumb.png"; sourceSize.width: 144; fillMode: Image.PreserveAspectFit; asynchronous: true }
                Text { height: 40; verticalAlignment: Text.AlignVCenter; text: qsTr("с: ") + qsTr(card.f.who) + qsTr("   где: ") + qsTr(card.f.place) + (card.f.place === "здесь" ? "" : ", " + qsTr(card.f.time)); color: Theme.text; font.family: Theme.ui; font.pixelSize: 15 }
            }
            Lbl { visible: card.selected; text: qsTr("чьи руки") }
            Flow {
                visible: card.selected
                width: parent.width
                spacing: 5
                Repeater {
                    model: sfCol.hands
                    Rectangle {
                        required property var modelData
                        width: 84; height: 62; radius: 6
                        color: card.f.who === modelData[0] ? "#2c3a40" : ha.containsMouse ? Theme.panel3 : Theme.panel2
                        border.width: card.f.who === modelData[0] ? 2 : 0
                        border.color: Theme.white
                        Image { x: 4; y: 3; width: parent.width - 8; height: 38; source: "image://tb/file/mods/genry_assets/hands/" + parent.modelData[1] + "/thumb.png"; sourceSize.width: 160; fillMode: Image.PreserveAspectFit; asynchronous: true }
                        Text { anchors.bottom: parent.bottom; anchors.bottomMargin: 3; anchors.horizontalCenter: parent.horizontalCenter; text: qsTr(parent.modelData[0]); color: Theme.text; font.family: Theme.ui; font.pixelSize: 11 }
                        MouseArea { id: ha; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { card.ed.select(card.index); card.edit({ who: parent.modelData[0] }) } }
                    }
                }
            }
            Lbl { visible: card.selected; text: qsTr("где играют (здесь — сама сцена в дымке)") }
            Flow {
                visible: card.selected
                width: parent.width
                spacing: 5
                Repeater {
                    model: sfCol.places
                    Rectangle {
                        required property var modelData
                        width: 112; height: 70; radius: 6; clip: true
                        color: "#0e0e10"
                        border.width: 2
                        border.color: card.f.place === modelData[0] ? Theme.white : pa.containsMouse ? Theme.dim : "transparent"
                        Image { anchors.fill: parent; anchors.margins: 2; visible: parent.modelData[1] !== ""; source: sfCol.placeThumb(parent.modelData[1]); sourceSize.width: 220; fillMode: Image.PreserveAspectCrop; asynchronous: true }
                        Text { visible: parent.modelData[1] === ""; anchors.centerIn: parent; text: "✦"; color: Theme.dim; font.pixelSize: 22 }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 18; color: "#000000aa" }
                        Text { anchors.bottom: parent.bottom; anchors.bottomMargin: 2; anchors.horizontalCenter: parent.horizontalCenter; text: qsTr(parent.modelData[0]); color: Theme.text; font.family: Theme.ui; font.pixelSize: 11 }
                        MouseArea { id: pa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { card.ed.select(card.index); card.edit({ place: parent.modelData[0] }) } }
                    }
                }
            }
            Row {
                visible: card.selected && card.f.place !== "здесь"
                spacing: 6
                Lbl { text: qsTr("когда") }
                Repeater {
                    model: sfCol.times
                    Chip { required property var modelData; anchors.verticalCenter: parent.verticalCenter; text: qsTr(modelData[0]); on: card.f.time === modelData[0]; onClicked: card.edit({ time: modelData[0] }) }
                }
            }
            Row {
                spacing: 8
                Lbl { text: qsTr("выиграл —") }
                ScenePicker { value: card.f.win; onPicked: (name) => card.edit({ win: name }) }
                Lbl { text: qsTr("проиграл —") }
                ScenePicker { value: card.f.lose; onPicked: (name) => card.edit({ lose: name }) }
            }
            Text { visible: card.selected; text: qsTr("ничья — играют ещё раз"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12 }
        }
    }

    Component {
        id: riddleC
        Column {
            spacing: 6
            function focusText() { rq.forceActiveFocus() }
            TField { id: rq; width: parent.width; text: card.f.question; font.pixelSize: 16; placeholderText: qsTr("вопрос"); onTextEdited: card.edit({ question: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
            Row {
                spacing: 8
                Lbl { text: qsTr("верные ответы") }
                TField { width: 260; text: card.f.answers; placeholderText: qsTr("ёлка/ель"); onTextEdited: card.edit({ answers: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
                Lbl { text: qsTr("— через /, регистр и ё не важны") }
            }
            Row {
                spacing: 8
                Lbl { text: qsTr("угадал —") }
                ScenePicker { value: card.f.ok; onPicked: (name) => card.edit({ ok: name }) }
                Lbl { text: qsTr("нет —") }
                ScenePicker { value: card.f.no; onPicked: (name) => card.edit({ no: name }) }
            }
        }
    }

    // spots on the picture: what is there (looked at - dimmed), or a way out
    Component {
        id: exploreC
        Column {
            id: exCol
            spacing: 6
            function items() { var o = []; for (var i = 0; i < card.f.items.length; i++) { var x = card.f.items[i]; o.push({ name: x.name, x: x.x, y: x.y, note: x.note, target: x.target }) } return o }
            function setItem(i, key, value) { var o = items(); o[i][key] = value; card.edit({ items: o }) }
            Repeater {
                model: card.f.items
                Row {
                    id: it
                    required property var modelData
                    required property int index
                    spacing: 6
                    TField { width: 130; text: it.modelData.name; font.pixelSize: 15; placeholderText: qsTr("что"); onTextEdited: exCol.setItem(it.index, "name", text); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
                    Chip { anchors.verticalCenter: parent.verticalCenter; text: it.modelData.target === "" ? qsTr("смотреть") : qsTr("выход"); hint: qsTr("смотреть — строчка описания; выход — в другую сцену")
                           onClicked: exCol.setItem(it.index, "target", it.modelData.target === "" ? Engine.sceneNames()[0] : "") }
                    TField { visible: it.modelData.target === ""; width: exCol.width - 130 - 90 - 60; text: it.modelData.note; placeholderText: qsTr("что видно"); onTextEdited: exCol.setItem(it.index, "note", text); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
                    ScenePicker { visible: it.modelData.target !== ""; value: it.modelData.target; onPicked: (name) => exCol.setItem(it.index, "target", name) }
                    Rectangle {
                        y: 4; width: 24; height: 24; radius: 6
                        visible: card.f.items.length > 1
                        color: xa.containsMouse ? "#5a2626" : "transparent"
                        Text { anchors.centerIn: parent; text: "✕"; color: Theme.dim; font.pixelSize: 12 }
                        MouseArea { id: xa; anchors.fill: parent; hoverEnabled: true; onClicked: { var o = exCol.items(); o.splice(it.index, 1); card.edit({ items: o }) } }
                    }
                }
            }
            Row {
                spacing: 8
                TButton { text: qsTr("＋ точка"); onClicked: { var o = exCol.items(); o.splice(Math.max(0, o.length - 1), 0, { name: qsTr("Ещё"), x: 500 + 220 * (o.length % 5), y: 520, note: "…", target: "" }); card.edit({ items: o }) } }
                Text { height: 30; verticalAlignment: Text.AlignVCenter; text: qsTr("— у осмотра должен быть хоть один выход"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12 }
            }
        }
    }

    // the phone: numbers to dial, each a scene; a wrong number - the operator, or a scene
    Component {
        id: phoneC
        Column {
            id: phCol
            spacing: 6
            function nums() { var o = []; for (var i = 0; i < card.f.numbers.length; i++) o.push({ num: card.f.numbers[i].num, target: card.f.numbers[i].target }); return o }
            function setNum(i, key, value) { var o = nums(); o[i][key] = value; card.edit({ numbers: o }) }
            Repeater {
                model: card.f.numbers
                Row {
                    id: nr
                    required property var modelData
                    required property int index
                    spacing: 8
                    Text { text: "☎️"; font.family: Theme.emoji; font.pixelSize: 15; height: 32; verticalAlignment: Text.AlignVCenter }
                    TField {
                        width: 130; text: nr.modelData.num; font.family: "Consolas"; font.pixelSize: 16
                        validator: RegularExpressionValidator { regularExpression: /\d{1,10}/ }
                        onTextEdited: if (text !== "") phCol.setNum(nr.index, "num", text)
                        onActiveFocusChanged: if (activeFocus) card.ed.select(card.index)
                    }
                    Text { text: "→"; color: Theme.dim; font.pixelSize: 15; height: 32; verticalAlignment: Text.AlignVCenter }
                    ScenePicker { y: 1; value: nr.modelData.target; onPicked: (name) => phCol.setNum(nr.index, "target", name) }
                    Rectangle {
                        y: 4; width: 24; height: 24; radius: 6
                        visible: card.f.numbers.length > 1
                        color: pd.containsMouse ? "#5a2626" : "transparent"
                        Text { anchors.centerIn: parent; text: "✕"; color: Theme.dim; font.pixelSize: 12 }
                        MouseArea { id: pd; anchors.fill: parent; hoverEnabled: true; onClicked: { var o = phCol.nums(); o.splice(nr.index, 1); card.edit({ numbers: o }) } }
                    }
                }
            }
            Row {
                spacing: 8
                TButton {
                    text: qsTr("＋ номер")
                    onClicked: {
                        var o = phCol.nums(), n = o.length ? o[0].num.length : 5, s = ""
                        for (var i = 0; i < n; i++) s += Math.floor(Math.random() * 10)
                        o.push({ num: s, target: o.length ? o[o.length - 1].target : Engine.sceneNames()[0] })
                        card.edit({ numbers: o })
                    }
                }
                Lbl { text: qsTr("не тот номер —") }
                Chip { anchors.verticalCenter: parent.verticalCenter; text: qsTr("оператор"); hint: qsTr("«Неправильно набран номер» — и набирать снова"); on: card.f.wrong === ""; onClicked: card.edit({ wrong: "" }) }
                Chip { anchors.verticalCenter: parent.verticalCenter; text: qsTr("своя сцена"); on: card.f.wrong !== ""; onClicked: if (card.f.wrong === "") card.edit({ wrong: Engine.sceneNames()[0] }) }
                ScenePicker { visible: card.f.wrong !== ""; value: card.f.wrong; onPicked: (name) => card.edit({ wrong: name }) }
            }
            Text { visible: card.selected; text: qsTr("все номера одной длины — набрал столько цифр, и звонок пошёл"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12 }
        }
    }

    // a picture in pieces: which picture, how many pieces, where to when it is whole
    Component {
        id: puzzleC
        Row {
            spacing: 10
            function focusText() { pz.forceActiveFocus() }
            Rectangle {
                width: 112; height: 63; radius: 5; color: "#0e0e10"; clip: true
                Image { anchors.fill: parent; source: card.f.pic.indexOf("images/") === 0 ? "image://tb/file/" + card.f.pic.split(" + ")[0] : "image://tb/bg/256/" + encodeURIComponent(card.f.pic.split(" + ")[0]); sourceSize.width: 224; fillMode: Image.PreserveAspectCrop; asynchronous: true }
            }
            Column {
                spacing: 6
                TField { id: pz; width: 400; text: card.f.pic; font.family: "Consolas"; placeholderText: "images/…png"; onTextEdited: if (text.trim() !== "") card.edit({ pic: text.trim() }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
                Row {
                    spacing: 6
                    Repeater {
                        model: ["2x2", "3x2", "3x3", "4x3", "4x4"]
                        Chip { required property var modelData; anchors.verticalCenter: parent.verticalCenter; text: modelData.replace("x", "×"); on: card.f.grid === modelData; onClicked: card.edit({ grid: modelData }) }
                    }
                    Text { height: 30; verticalAlignment: Text.AlignVCenter; text: qsTr("собрал —"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12 }
                    ScenePicker { value: card.f.target; onPicked: (name) => card.edit({ target: name }) }
                }
            }
        }
    }

    // the game's hand holding a picture: which picture (a file of the game), the words under it
    Component {
        id: inhandC
        Row {
            spacing: 10
            function focusText() { ih.forceActiveFocus() }
            Rectangle {
                width: 96; height: 72; radius: 5; color: "#0e0e10"; clip: true
                Image { anchors.fill: parent; anchors.margins: 2; source: card.f.pic.indexOf("images/") === 0 ? "image://tb/file/" + card.f.pic : "image://tb/bg/256/" + encodeURIComponent(card.f.pic); sourceSize.width: 192; fillMode: Image.PreserveAspectFit; asynchronous: true }
            }
            Column {
                spacing: 6
                TField { id: ih; width: 380; text: card.f.pic; font.family: "Consolas"; placeholderText: "images/…png"; onTextEdited: if (text.trim() !== "") card.edit({ pic: text.trim() }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
                TField { width: 380; text: card.f.text; placeholderText: qsTr("подпись снизу (можно пусто)"); onTextEdited: card.edit({ text: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
            }
        }
    }

    Component {
        id: achC
        Row {
            spacing: 8
            function focusText() { at.forceActiveFocus() }
            Text { text: "🏆"; font.family: Theme.emoji; font.pixelSize: 18; height: 32; verticalAlignment: Text.AlignVCenter }
            TField { id: at; width: 200; text: card.f.title; font.pixelSize: 15; placeholderText: qsTr("название"); onTextEdited: card.edit({ title: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
            TField { width: 300; text: card.f.text; placeholderText: qsTr("за что"); onTextEdited: card.edit({ text: text }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
        }
    }

    Component {
        id: codeC
        TField {
            function focusText() { forceActiveFocus() }
            text: card.f.code
            font.family: "Consolas"
            font.pixelSize: 14
            placeholderText: qsTr("строка на питоне")
            onTextEdited: card.edit({ code: text })
            onActiveFocusChanged: if (activeFocus) card.ed.select(card.index)
        }
    }

    Component {
        id: gameC
        Row {
            spacing: 8
            function focusText() { gl.forceActiveFocus() }
            Lbl { text: qsTr("в историю игры с метки") }
            TField { id: gl; width: 220; text: card.f.label; font.family: "Consolas"; onTextEdited: if (text.trim() !== "") card.edit({ label: text.trim() }); onActiveFocusChanged: if (activeFocus) card.ed.select(card.index) }
        }
    }

    Component {
        id: plainC
        Text {
            text: card.kind === "end" ? qsTr("История закончена: надпись «Конец» и главное меню игры")
                  : card.kind === "shake" ? qsTr("Экран вздрагивает")
                  : card.kind === "flash" ? qsTr("Белая вспышка")
                  : card.kind === "return" ? qsTr("Вставка кончилась — дальше игра идёт так, как шла") : ""
            color: Theme.dim
            font.family: Theme.ui
            font.pixelSize: 14
        }
    }
}
