import QtQuick
import QtQuick.Controls
import GenryTB

// A button of the work screens: plain, or the one to press (primary: white).
Rectangle {
    id: root
    property string text
    property bool primary: false
    property bool danger: false
    property string hint
    property color dot: "transparent"       // a kind's colour in front (the add bar)
    signal clicked()
    implicitWidth: row.implicitWidth + 26
    implicitHeight: 34
    radius: 8
    color: primary ? (area.containsMouse ? "#ffffff" : "#eaeaea") : area.containsMouse ? Theme.panel3 : Theme.panel2
    scale: area.pressed ? 0.94 : 1
    Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }
    Behavior on color { ColorAnimation { duration: 120 } }
    border.color: primary ? "transparent" : Theme.line
    opacity: enabled ? 1 : 0.4
    Row {
        id: row
        anchors.centerIn: parent
        spacing: 7
        Rectangle {
            visible: root.dot.a > 0
            width: 8; height: 8; radius: 4
            color: root.dot
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.text
            font.family: Theme.ui
            font.pixelSize: 14
            font.weight: root.primary ? Font.DemiBold : Font.Normal
            color: root.primary ? "#101010" : root.danger ? Theme.danger : Theme.text
        }
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
    ToolTip.visible: area.containsMouse && root.hint !== ""
    ToolTip.text: root.hint
    ToolTip.delay: 450
}
