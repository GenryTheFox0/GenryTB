import QtQuick
import QtQuick.Controls
import GenryTB

// A field of the launcher: its question in grey, the answer in the game's own hand, a white line under it.
Column {
    id: root
    property string label
    property alias text: field.text
    property alias placeholder: field.placeholderText
    signal accepted()
    spacing: 4
    function forceActiveFocus() { field.forceActiveFocus() }
    Text {
        text: root.label
        font.family: Engine.textFont
        font.pixelSize: 30
        color: "#9a9a9a"
    }
    TextField {
        id: field
        width: root.width
        font.family: Engine.textFont
        font.pixelSize: 44
        color: "#ffffff"
        placeholderTextColor: "#4e4e4e"
        selectionColor: "#ffffff"
        selectedTextColor: "#000000"
        leftPadding: 4
        background: Item {
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: field.activeFocus ? 3 : 2
                color: field.activeFocus ? "#ffffff" : "#6a6a6a"
            }
        }
        onAccepted: root.accepted()
    }
}
