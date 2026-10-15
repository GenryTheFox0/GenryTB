import QtQuick
import QtQuick.Controls
import GenryTB

// A text field of the work screens.
TextField {
    id: root
    font.family: Theme.ui
    font.pixelSize: 14
    color: Theme.text
    placeholderTextColor: Theme.faint
    selectionColor: "#ffffff"
    selectedTextColor: "#101010"
    selectByMouse: true
    leftPadding: 10
    rightPadding: 10
    implicitHeight: 32
    background: Rectangle {
        radius: 7
        color: Theme.bg
        border.color: root.activeFocus ? "#9a9aa2" : Theme.line
    }
}
