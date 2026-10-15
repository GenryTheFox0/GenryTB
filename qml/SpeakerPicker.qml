import QtQuick
import QtQuick.Controls
import GenryTB

// Who says the line: the game's people by their faces, the narrator, Anton's thoughts - or a name of your
// own. Under them: the game's recorded lines of that voice, found by their words - take one with its sound.
Item {
    id: root
    property string value                 // the card's speaker ("" narrator, "мысли", a name)
    property string who                   // as shown
    property string voice                 // the card's voice file
    signal picked(string key)
    signal voicePicked(string file, string text)
    property var people: { Engine.frameRev; return Engine.speakers() }
    property var lines: Engine.voices(root.value === "" || root.value === "мысли" ? "*" : root.who, find.text, 120)

    Flickable {
        id: top
        x: 10; y: 8
        width: parent.width - 20
        height: Math.min(flow.implicitHeight, parent.height * 0.42)
        contentHeight: flow.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }
        Flow {
            id: flow
            width: top.width - 10
            spacing: 5
            Repeater {
                model: root.people
                Rectangle {
                    id: p
                    required property var modelData
                    readonly property bool on: modelData.key === root.value
                    width: label.implicitWidth + (pic.visible ? 44 : 22)
                    height: 32
                    radius: 16
                    color: on ? Theme.white : a.containsMouse ? Theme.panel3 : Theme.panel2
                    border.color: on ? Theme.white : Theme.line
                    Image {
                        id: pic
                        x: 3; y: 3; width: 26; height: 26
                        visible: p.modelData.thumb !== ""
                        source: p.modelData.thumb !== "" ? "image://tb/face/72/" + encodeURIComponent(p.modelData.thumb) : ""
                        asynchronous: true
                        fillMode: Image.PreserveAspectCrop
                    }
                    Text {
                        id: label
                        x: pic.visible ? 34 : 11
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr(p.modelData.name)
                        color: p.on ? "#101010" : Theme.text
                        font.family: Theme.ui
                        font.pixelSize: 13
                        font.weight: p.on ? Font.DemiBold : Font.Normal
                    }
                    MouseArea { id: a; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.picked(p.modelData.key) }
                }
            }
            Row {
                spacing: 5
                TField { id: own; width: 150; placeholderText: qsTr("своё имя…"); onAccepted: if (text.trim() !== "") { root.picked(text.trim()); text = "" } }
                TButton { text: qsTr("Взять"); implicitHeight: 32; onClicked: own.accepted() }
            }
        }
    }

    Rectangle { x: 10; y: top.y + top.height + 8; width: parent.width - 20; height: 1; color: Theme.line }
    Row {
        id: findRow
        x: 10; y: top.y + top.height + 16
        spacing: 8
        Text { text: qsTr("ОЗВУЧКА ИЗ ИГРЫ"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.8; height: 32; verticalAlignment: Text.AlignVCenter }
        TField { id: find; width: 220; placeholderText: qsTr("слова из реплики") }
        TButton { visible: root.voice !== ""; text: qsTr("убрать голос"); implicitHeight: 32; onClicked: root.voicePicked("", "") }
    }
    ListView {
        id: list
        x: 10
        y: findRow.y + 40
        width: parent.width - 14
        height: parent.height - y - 6
        clip: true
        model: root.lines
        spacing: 2
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }
        delegate: Rectangle {
            id: v
            required property var modelData
            width: list.width - 12
            height: Math.max(34, vt.implicitHeight + 12)
            radius: 7
            readonly property bool on: modelData.file === root.voice
            color: on ? "#2c2c31" : va.containsMouse ? "#1f1f23" : "transparent"
            border.color: on ? "#9a9aa2" : "transparent"
            MouseArea { id: va; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.voicePicked(v.modelData.file, v.modelData.text) }
            Rectangle {
                x: 4; y: 4; width: 26; height: 26; radius: 13
                color: Engine.playing === v.modelData.file ? "#d7a6ff" : Theme.panel3
                Text { anchors.centerIn: parent; text: Engine.playing === v.modelData.file ? "■" : "▶"; color: Engine.playing === v.modelData.file ? "#101010" : Theme.text; font.pixelSize: 11 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Engine.play(v.modelData.file) }
            }
            Text {
                id: vt
                x: 40; y: 6
                width: parent.width - 48
                wrapMode: Text.WordWrap
                text: (root.value === "" || root.value === "мысли" ? "<font color='#ffd28a'>" + qsTr(v.modelData.name) + ":</font> " : "") + v.modelData.text.replace(/&/g, "&amp;").replace(/</g, "&lt;")
                textFormat: Text.StyledText
                color: Theme.text
                font.family: Theme.ui
                font.pixelSize: 13
            }
        }
    }
}
