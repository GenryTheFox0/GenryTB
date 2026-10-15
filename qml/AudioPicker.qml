import QtQuick
import QtQuick.Controls
import GenryTB

// The game's music and sounds: ▶ to listen, a click on the name - the card takes it.
Item {
    id: root
    property string mode: "music"         // music | amb | sound
    property string value
    signal picked(string file)
    property string folder: mode === "amb" ? "fon" : (folders.length ? folders[0] : "")
    property var folders: mode === "sound" ? Engine.soundFolders() : []
    property var items: mode === "music" ? Engine.music(filter.text) : Engine.sounds(folder, filter.text)

    Flow {
        id: bar
        x: 10; y: 8
        width: parent.width - 20
        spacing: 6
        TField { id: filter; width: 200; placeholderText: qsTr("найти по названию") }
        Repeater {
            model: root.folders
            Chip { required property string modelData; text: modelData; on: root.folder === modelData; onClicked: root.folder = modelData }
        }
    }
    ListView {
        id: list
        anchors.fill: parent
        anchors.topMargin: bar.height + 16
        anchors.leftMargin: 10
        clip: true
        model: root.items
        spacing: 2
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }
        delegate: Rectangle {
            id: rowItem
            required property var modelData
            width: list.width - 14
            height: 34
            radius: 7
            readonly property bool on: modelData.file === root.value
            color: on ? "#2c2c31" : a.containsMouse ? "#1f1f23" : "transparent"
            border.color: on ? "#9a9aa2" : "transparent"
            MouseArea { id: a; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.picked(rowItem.modelData.file) }
            Rectangle {
                x: 4; y: 4; width: 26; height: 26; radius: 13
                color: Engine.playing === rowItem.modelData.file ? "#d7a6ff" : Theme.panel3
                Text { anchors.centerIn: parent; text: Engine.playing === rowItem.modelData.file ? "■" : "▶"; color: Engine.playing === rowItem.modelData.file ? "#101010" : Theme.text; font.pixelSize: 11 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Engine.play(rowItem.modelData.file) }
            }
            Text {
                x: 40
                width: parent.width - 50
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                text: rowItem.modelData.name
                color: rowItem.on ? Theme.white : Theme.text
                font.family: Theme.ui
                font.pixelSize: 14
            }
        }
    }
}
