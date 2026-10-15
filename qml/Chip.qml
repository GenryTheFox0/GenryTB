import QtQuick
import QtQuick.Controls
import GenryTB

// A small choice: one of several (lit = white), or a plain button.
Rectangle {
    id: root
    property string text
    property bool on: false
    property color accent: Theme.white
    property bool enabledChip: true
    property string hint
    signal clicked()
    implicitWidth: label.implicitWidth + 22
    implicitHeight: 28
    radius: 14
    color: on ? accent : area.containsMouse ? Theme.panel3 : Theme.panel2
    scale: area.pressed ? 0.93 : 1
    Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }
    Behavior on color { ColorAnimation { duration: 140 } }
    border.color: on ? accent : Theme.line
    opacity: enabledChip ? 1 : 0.4
    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        font.family: Theme.ui
        font.pixelSize: 13
        font.weight: root.on ? Font.DemiBold : Font.Normal
        color: root.on ? "#101010" : Theme.text
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabledChip
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
    ToolTip.visible: area.containsMouse && root.hint !== ""
    ToolTip.text: root.hint
    ToolTip.delay: 350
}
