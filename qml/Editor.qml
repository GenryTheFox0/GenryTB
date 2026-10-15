import QtQuick
import QtQuick.Controls
import GenryTB

// The workshop: the mod's scenes on the left, the scene's cards in the middle, on the right what the
// player sees at the selected card and everything that card can be made of.
Item {
    id: editor
    signal back()
    property int row: -1                     // the selected card
    property int animateRow: -1              // the card about to be added (it slides in)
    property bool textMode: false
    readonly property var blk: { Engine.frameRev; return Engine.blockAt(row) }
    readonly property string kind: blk.kind !== undefined ? blk.kind : ""
    property var faces: ({})                 // speaker -> a sprite of theirs (for the faces on the cards)
    property bool playing: false             // the scene plays card by card, right here
    function replay() { preview.replay++ }

    // ---- the scene played in the editor: a card after a card, its voice and its moves ----
    function cardSeconds(b) {
        switch (b.kind) {
        case "say": return Math.max(1.6, String(b.f.text).length * 0.065) + (b.f.voice ? 0.6 : 0)
        case "pause": return Number(b.f.sec)
        case "bg": case "show": case "hide": return 0.9
        case "camera": case "move": case "weather": case "film": return 1.4
        case "shake": case "flash": return 0.6
        }
        return 0.35
    }
    function playCard() {
        var b = Engine.blockAt(row)
        if (b.kind === undefined) { stopPlay(); return }
        if (b.kind === "say" && b.f.voice) Engine.play(b.f.voice)
        else if (b.kind === "sound" && b.f.file !== "") Engine.play(b.f.file)
        if (b.kind === "menu") { stopPlay(); Engine.say(qsTr("Выбор: дальше решает игрок")); return }
        if (b.kind === "end") { stopPlay(); return }
        playTimer.interval = Math.round(cardSeconds(b) * 1000)
        playTimer.restart()
    }
    function playNext() {
        if (!playing) return
        var b = Engine.blockAt(row)
        if (b.kind === "jump") {
            var i = Engine.sceneNames().indexOf(b.f.target)
            if (i < 0) { stopPlay(); return }
            Engine.scene = i
            if (Engine.blockCount() === 0) { stopPlay(); return }
            select(0); blocks.list.positionViewAtBeginning(); playCard()
            return
        }
        if (row >= Engine.blockCount() - 1) { stopPlay(); return }
        select(row + 1)
        blocks.reveal(row)
        playCard()
    }
    function stopPlay() { playing = false; playTimer.stop() }
    function togglePlay() {
        if (playing) { stopPlay(); Engine.stop(); return }
        if (Engine.blockCount() === 0) return
        if (row < 0 || row >= Engine.blockCount() - 1) select(0)
        playing = true
        blocks.reveal(row)
        replay()
        playCard()
    }
    Timer { id: playTimer; onTriggered: editor.playNext() }

    function select(r) {
        if (r >= Engine.blockCount()) r = Engine.blockCount() - 1
        row = r
        Engine.tryOn = ""
        Engine.cursorLine = Engine.blockLine(r)
    }
    function add(kind, fields) {
        animateRow = Math.max(0, row + 1)
        var r = Engine.addBlock(row, kind, fields || {})
        if (r < 0) return
        select(r)
        blocks.reveal(r)
        if (kind === "say" || kind === "thought" || kind === "narrator") blocks.focusText(r)
        else editor.forceActiveFocus()
    }
    function removeRow(r) {
        Engine.removeBlock(r)
        select(Math.min(r, Engine.blockCount() - 1))
    }
    function focusRow(r) { blocks.focusText(r) }
    function openScene(i) {
        Engine.scene = i
        select(Engine.blockCount() > 0 ? 0 : -1)
        blocks.list.positionViewAtBeginning()
    }
    function faceOf(key) {
        var t = faces[key]
        return t ? "image://tb/face/72/" + encodeURIComponent(t) : ""
    }
    function firstAudio(kind) { var l = kind === "music" ? Engine.music("") : Engine.sounds("fon", ""); return l.length ? l[0].file : "" }
    function goToProblem() {
        var p = Engine.problems
        if (!p.length) return
        Engine.scene = Engine.sceneOfLine(p[0].line)
        var r = Engine.rowAtLine(p[0].line)
        select(Math.max(0, r))
        blocks.reveal(Math.max(0, r))
        if (textMode && codeLoader.item) codeLoader.item.goToLine(p[0].line)
    }
    function selectKind(kind) {
        for (var i = 0; i < Engine.blockCount(); i++)
            if (Engine.blockAt(i).kind === kind) { select(i); blocks.reveal(i); return }
    }

    Component.onCompleted: {
        var sp = Engine.speakers(), m = {}
        for (var i = 0; i < sp.length; i++) if (sp[i].thumb !== "") m[sp[i].key] = sp[i].thumb
        faces = m
        select(Engine.blockCount() > 0 ? 0 : -1)
        var page = Engine.shotPage
        if (page.indexOf("editor:") === 0) {
            // editor:<kind>[@seconds] - that card selected, its moves that far in (checks)
            var what = page.substring(7), at = what.indexOf("@")
            var secs = at >= 0 ? Number(what.substring(at + 1)) : 0
            if (at >= 0) what = what.substring(0, at)
            var parts = what.split(",")                       // editor:scene1,row3@2
            for (var k = 0; k < parts.length; k++) {
                var w = parts[k]
                if (w === "text") textMode = true
                else if (w.indexOf("row") === 0) { select(Number(w.substring(3))); blocks.reveal(row) }
                else if (w.indexOf("scene") === 0) openScene(Number(w.substring(5)))
                else selectKind(w)
            }
            if (secs > 0) Qt.callLater(function () { preview.clock = secs })
        }
        forceActiveFocus()
    }

    focus: true
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Z && (event.modifiers & Qt.ControlModifier)) { if (event.modifiers & Qt.ShiftModifier) Engine.redo(); else Engine.undo(); event.accepted = true }
        else if (event.key === Qt.Key_Y && (event.modifiers & Qt.ControlModifier)) { Engine.redo(); event.accepted = true }
        else if (event.key === Qt.Key_Delete && !textMode && row >= 0) { removeRow(row); event.accepted = true }
        else if (event.key === Qt.Key_Space && !textMode) { togglePlay(); event.accepted = true }
        else if (event.key === Qt.Key_Up && !textMode && row > 0) { select(row - 1); blocks.reveal(row); event.accepted = true }
        else if (event.key === Qt.Key_Down && !textMode && row < Engine.blockCount() - 1) { select(row + 1); blocks.reveal(row); event.accepted = true }
    }
    Shortcut { sequence: "F5"; onActivated: Engine.run(false) }
    Shortcut { sequence: "F6"; onActivated: Engine.run(true) }

    // ---- the top bar ----
    Rectangle {
        id: bar
        width: parent.width
        height: 54
        color: Theme.panel
        z: 5
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }
        Row {
            x: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            TButton { text: qsTr("‹ Меню"); onClicked: editor.back() }
            Item {
                width: nameText.implicitWidth + 34; height: 34
                Text {
                    id: nameText
                    anchors.verticalCenter: parent.verticalCenter
                    text: Engine.projectName
                    color: Theme.white
                    font.family: Engine.titleFont
                    font.pixelSize: 30
                }
                Text { x: nameText.implicitWidth + 10; anchors.verticalCenter: parent.verticalCenter; text: "✎"; color: na.containsMouse ? Theme.white : Theme.faint; font.pixelSize: 16 }
                MouseArea { id: na; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { var h = Engine.header; hName.text = h.name; hAuthor.text = h.author; hDesc.text = h.desc; hAdult.on = h.adult === true; headerPopup.open() } }
            }
            TButton { text: "↶"; hint: qsTr("отменить (Ctrl+Z)"); enabled: Engine.canUndo; onClicked: Engine.undo() }
            TButton { text: "↷"; hint: qsTr("вернуть (Ctrl+Y)"); enabled: Engine.canRedo; onClicked: Engine.redo() }
        }
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            Rectangle {
                visible: Engine.problems.length > 0
                anchors.verticalCenter: parent.verticalCenter
                width: pt.implicitWidth + 24; height: 30; radius: 15
                color: "#3a1c1c"; border.color: Theme.danger
                Text { id: pt; anchors.centerIn: parent; text: qsTr("ошибок: ") + Engine.problems.length + qsTr(" — показать"); color: "#ffb3b0"; font.family: Theme.ui; font.pixelSize: 13 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: editor.goToProblem() }
            }
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 0
                Chip { text: qsTr("Карточки"); on: !editor.textMode; onClicked: editor.textMode = false }
                Chip { text: qsTr("Текст"); on: editor.textMode; onClicked: editor.textMode = true }
            }
            TButton { text: qsTr("В игру"); hint: qsTr("положить мод в игру: главное меню → «Моды»"); onClicked: Engine.install() }
            TButton { text: qsTr("▶ Эта сцена"); hint: qsTr("запустить игру прямо в этой сцене (F6)"); onClicked: Engine.run(true) }
            TButton { text: Engine.running ? qsTr("Игра запущена") : qsTr("▶ Играть"); primary: true; hint: qsTr("запустить мод в игре с начала (F5)"); onClicked: Engine.run(false) }
        }
    }

    // ---- scenes | cards | preview + what the card is made of ----
    ScenesPanel {
        id: scenes
        ed: editor
        y: bar.height
        width: 220
        height: parent.height - bar.height
    }
    Item {
        id: middle
        x: scenes.width
        y: bar.height
        width: parent.width - scenes.width - right.width
        height: parent.height - bar.height
        BlocksView { id: blocks; anchors.fill: parent; ed: editor; visible: !editor.textMode }
        Loader { id: codeLoader; anchors.fill: parent; active: editor.textMode; sourceComponent: CodeView { } }
    }
    Rectangle {
        id: right
        anchors.right: parent.right
        y: bar.height
        width: Math.round(Math.min(parent.width * 0.42, 760))
        height: parent.height - bar.height
        color: Theme.panel
        Rectangle { width: 1; height: parent.height; color: Theme.line; z: 3 }
        PreviewPane {
            id: preview
            ed: editor
            x: 1
            width: parent.width - 1
            height: Math.round(width * 9 / 16)
        }
        Rectangle {
            id: caption
            x: 1; y: preview.height
            width: parent.width - 1; height: 30
            color: "#101012"
            Rectangle {
                id: playBtn
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                width: playLabel.implicitWidth + 20; height: 22; radius: 11
                color: editor.playing ? Theme.white : pa.containsMouse ? Theme.panel3 : Theme.panel2
                border.color: editor.playing ? Theme.white : Theme.line
                Text { id: playLabel; anchors.centerIn: parent; text: editor.playing ? qsTr("■ стоп") : qsTr("▶ просмотр"); color: editor.playing ? "#101010" : Theme.text; font.family: Theme.ui; font.pixelSize: 12 }
                MouseArea { id: pa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: editor.togglePlay() }
                ToolTip.visible: pa.containsMouse
                ToolTip.text: qsTr("проиграть сцену прямо здесь, карточка за карточкой (пробел)")
                ToolTip.delay: 450
            }
            Text {
                x: 12
                width: parent.width - 24 - playBtn.width - 8
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                color: Theme.dim
                font.family: Theme.ui; font.pixelSize: 12
                text: {
                    var i = Engine.sceneInfo
                    var who = []
                    for (var k = 0; k < i.sprites.length; k++) who.push(i.sprites[k].who)
                    return qsTr("сцена «") + i.scene + qsTr("»   ·   фон: ") + (i.bg === "" ? "—" : i.bg) + qsTr("   ·   на экране: ") + (who.length ? who.join(", ") : qsTr("никого")) +
                           (i.music !== "" ? "   ·   ♪ " + i.music.replace("music/", "").replace(".ogg", "") : "")
                }
            }
        }
        Inspector {
            ed: editor
            x: 1
            y: preview.height + caption.height
            width: parent.width - 1
            height: parent.height - y
        }
    }

    // ---- the mod's name, author, what it is about ----
    Popup {
        id: headerPopup
        x: 100; y: 60
        width: 520
        padding: 16
        modal: true
        background: Rectangle { radius: 12; color: "#1c1c20"; border.color: "#3a3a40" }
        contentItem: Column {
            spacing: 10
            Text { text: qsTr("Мод"); color: Theme.white; font.family: Engine.titleFont; font.pixelSize: 34 }
            Text { text: qsTr("Название"); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 12 }
            TField { id: hName; width: parent.width }
            Text { text: qsTr("Автор"); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 12 }
            TField { id: hAuthor; width: parent.width }
            Text { text: qsTr("О чём (видно в игре, в «Модах»)"); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 12 }
            TField { id: hDesc; width: parent.width }
            Row {
                visible: Engine.developer          // (18+ mods: the developer's build only, for now)
                spacing: 10
                Chip { id: hAdult; text: "18+"; accent: Theme.blood; hint: qsTr("для взрослых: в игре перед запуском спросит, есть ли игроку 18"); onClicked: on = !on }
                Text { height: 28; verticalAlignment: Text.AlignVCenter; text: qsTr("только для взрослых (взрослые спрайты 5 эпизода)"); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 12 }
            }
            Row {
                spacing: 8
                TButton { text: qsTr("Сохранить"); primary: true; onClicked: { if (hName.text.trim() !== "") Engine.setHeader(hName.text, hAuthor.text, hDesc.text, hAdult.on); headerPopup.close() } }
                TButton { text: qsTr("Отмена"); onClicked: headerPopup.close() }
            }
        }
    }

    Connections {
        target: Engine
        // undone / redone / typed in the text: the selected card may be gone
        function onStoryChanged() { if (editor.row >= Engine.blockCount()) editor.select(Engine.blockCount() - 1) }
    }
}
