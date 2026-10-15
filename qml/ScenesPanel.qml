import QtQuick
import QtQuick.Controls
import GenryTB

// The mod's scenes, in the order they are written (the first is where the mod starts). Click - its cards;
// double click - its name; drag by the grip - its place.
Rectangle {
    id: root
    property var ed
    color: Theme.panel
    property int dragFrom: -1
    property int dragTo: -1
    property int renaming: -1

    Text {
        x: 14; y: 12
        text: qsTr("СЦЕНЫ")
        color: Theme.faint
        font.family: Theme.ui; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.8
    }
    ListView {
        id: list
        x: 6; y: 34
        width: parent.width - 8
        height: parent.height - 34 - 54
        clip: true
        spacing: 2
        model: Engine.scenes
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }
        delegate: Rectangle {
            id: sc
            required property var modelData
            required property int index
            readonly property bool on: index === Engine.scene
            width: list.width - 8
            height: 44
            radius: 8
            color: on ? "#2a2a2f" : a.containsMouse ? "#1e1e22" : "transparent"
            border.color: on ? "#b8b8be" : "transparent"
            opacity: root.dragFrom === index ? 0.35 : 1
            Rectangle { visible: root.dragTo === sc.index && root.dragFrom >= 0 && root.dragFrom !== sc.index; y: root.dragTo > root.dragFrom ? parent.height : -3; width: parent.width; height: 3; radius: 1.5; color: Theme.white; z: 5 }
            MouseArea {
                id: a
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.ed.openScene(sc.index)
                onDoubleClicked: root.renaming = sc.index
            }
            Text { x: 6; anchors.verticalCenter: parent.verticalCenter; text: "⋮⋮"; color: g.containsMouse || g.pressed ? Theme.white : Theme.faint; font.pixelSize: 13 }
            MouseArea {
                id: g
                x: 0; width: 22; height: parent.height
                hoverEnabled: true
                preventStealing: true
                cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                onPressed: { root.dragFrom = sc.index; root.dragTo = sc.index }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    var y = mapToItem(list.contentItem, mouse.x, mouse.y).y
                    root.dragTo = Math.max(0, Math.min(list.count - 1, Math.floor(y / 46)))
                }
                onReleased: {
                    var from = root.dragFrom, to = root.dragTo
                    root.dragFrom = -1; root.dragTo = -1
                    if (from >= 0 && to >= 0 && from !== to) { Engine.moveScene(from, to); root.ed.openScene(Engine.scene) }
                }
                onCanceled: { root.dragFrom = -1; root.dragTo = -1 }
            }
            Column {
                x: 26
                visible: root.renaming !== sc.index
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    width: sc.width - 26 - 34
                    elide: Text.ElideRight
                    text: sc.modelData.name
                    color: sc.on ? Theme.white : Theme.text
                    font.family: Theme.ui; font.pixelSize: 14; font.weight: sc.on ? Font.DemiBold : Font.Normal
                }
                Text {
                    text: (sc.index === 0 ? qsTr("начало  ·  ") : "") + sc.modelData.blocks + qsTr(" карт.")
                    color: Theme.faint
                    font.family: Theme.ui; font.pixelSize: 10
                }
            }
            Rectangle {
                visible: sc.modelData.problems > 0 && root.renaming !== sc.index
                anchors.right: parent.right; anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 20; height: 20; radius: 10
                color: Theme.danger
                Text { anchors.centerIn: parent; text: sc.modelData.problems; color: "#fff"; font.family: Theme.ui; font.pixelSize: 11; font.bold: true }
            }
            Rectangle {
                visible: sc.on && a.containsMouse && list.count > 1 && sc.modelData.problems === 0 && root.renaming !== sc.index
                anchors.right: parent.right; anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                width: 22; height: 22; radius: 6
                color: da.containsMouse ? "#5a2626" : "transparent"
                Text { anchors.centerIn: parent; text: "✕"; color: Theme.dim; font.pixelSize: 12 }
                MouseArea {
                    id: da
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: { Engine.removeScene(sc.index); Engine.say(qsTr("Сцена убрана. Ctrl+Z — вернуть")); root.ed.openScene(Math.min(sc.index, Engine.scenes.length - 1)) }
                }
            }
            TField {
                id: edit
                visible: root.renaming === sc.index
                x: 24; y: 6
                width: parent.width - 30
                onVisibleChanged: if (visible) { text = sc.modelData.name; forceActiveFocus(); selectAll() }
                Component.onCompleted: if (visible) { text = sc.modelData.name; forceActiveFocus(); selectAll() }
                onAccepted: {
                    if (!Engine.renameScene(sc.index, text)) Engine.say(qsTr("Такая сцена уже есть"))
                    root.renaming = -1
                }
                onActiveFocusChanged: if (!activeFocus && root.renaming === sc.index) root.renaming = -1
                Keys.onEscapePressed: root.renaming = -1
            }
        }
    }
    TButton {
        x: 10
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        width: parent.width - 20
        text: qsTr("＋ Новая сцена")
        onClicked: {
            var i = Engine.addScene(Engine.freeSceneName(qsTr("сцена")))
            if (i >= 0) { root.ed.openScene(i); root.renaming = i; Qt.callLater(function () { list.positionViewAtEnd() }) }
        }
    }
    Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.line }
}
