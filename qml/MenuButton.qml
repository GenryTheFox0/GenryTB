import QtQuick
import GenryTB

// A button of the game's own menu: the white word; under the pointer its brush stroke comes up with the
// word in black, flickering a little as the game's does.
Item {
    id: root
    property string text
    property bool primary: false          // always on its stroke (the one thing to press)
    property int fontSize: 40
    signal clicked()
    width: 250
    height: 75

    Text {
        anchors.centerIn: parent
        text: root.text
        font.family: Engine.titleFont
        font.pixelSize: root.fontSize
        color: "#ffffff"
        visible: !root.primary
    }
    Item {
        id: stroke
        anchors.fill: parent
        opacity: root.primary ? 1 : 0
        Image {
            anchors.fill: parent
            source: "image://tb/file/images/interface/main_meny/plaska.png"
        }
        Text {
            anchors.centerIn: parent
            text: root.text
            font.family: Engine.titleFont
            font.pixelSize: root.fontSize
            color: "#000000"
        }
        Behavior on opacity { NumberAnimation { duration: 250 } }
    }
    // the game's jitter: the stroke shivers while pointed at
    Timer {
        interval: 60
        repeat: true
        running: area.containsMouse && !root.primary
        onTriggered: { stroke.x = Math.random() * 4 - 2; stroke.y = Math.random() * 4 - 2 }
        onRunningChanged: if (!running) { stroke.x = 0; stroke.y = 0 }
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: { if (!root.primary) stroke.opacity = 1; Engine.click("hover") }
        onExited: if (!root.primary) stroke.opacity = 0
        onClicked: { Engine.click("click"); root.clicked() }
    }
    scale: area.pressed ? 0.97 : 1
}
