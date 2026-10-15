import QtQuick
import QtQuick.Controls
import GenryTB

// The game's places as pictures: point at one - the preview tries it on; click - the card takes it.
Item {
    id: root
    property string value
    signal picked(string name)
    property bool scenesOnly: true
    property var items: Engine.backgrounds(filter.text, scenesOnly, 400)

    Row {
        id: bar
        x: 10; y: 8
        spacing: 8
        TField { id: filter; width: 220; placeholderText: qsTr("найти: kitchen, road, school…") }
        Chip { anchors.verticalCenter: parent.verticalCenter; text: qsTr("места"); on: root.scenesOnly; onClicked: root.scenesOnly = true }
        Chip { anchors.verticalCenter: parent.verticalCenter; text: qsTr("все картинки"); on: !root.scenesOnly; onClicked: root.scenesOnly = false }
        Text { anchors.verticalCenter: parent.verticalCenter; text: root.items.length + (root.items.length >= 400 ? "+" : ""); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 12 }
    }
    GridView {
        id: grid
        anchors.fill: parent
        anchors.topMargin: 48
        anchors.leftMargin: 10
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / 176)))
        cellHeight: cellWidth * 9 / 16 + 22
        clip: true
        model: root.items
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }
        delegate: Item {
            id: cell
            required property var modelData
            width: grid.cellWidth
            height: grid.cellHeight
            readonly property bool on: modelData.name === root.value
            Rectangle {
                x: 0; y: 0
                width: parent.width - 8
                height: width * 9 / 16
                radius: 5
                color: "#000"
                border.color: cell.on ? Theme.white : a.containsMouse ? "#8a8a92" : "transparent"
                border.width: 2
                clip: true
                Image {
                    anchors.fill: parent
                    anchors.margins: 2
                    source: "image://tb/bg/256/" + encodeURIComponent(cell.modelData.name)
                    asynchronous: true
                    fillMode: Image.PreserveAspectCrop
                }
            }
            Text {
                y: (parent.width - 8) * 9 / 16 + 3
                width: parent.width - 8
                elide: Text.ElideRight
                text: cell.modelData.name
                color: cell.on ? Theme.white : Theme.dim
                font.family: Theme.ui
                font.pixelSize: 11
            }
            MouseArea {
                id: a
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onEntered: Engine.tryOn = "bg:" + cell.modelData.name
                onExited: if (Engine.tryOn === "bg:" + cell.modelData.name) Engine.tryOn = ""
                onClicked: { Engine.tryOn = ""; root.picked(cell.modelData.name) }
            }
        }
    }
}
