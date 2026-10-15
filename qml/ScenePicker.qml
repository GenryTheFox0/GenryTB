import QtQuick
import QtQuick.Controls
import GenryTB

// Where a choice or a jump leads: a scene of the mod, picked from the list - or a new one made on the spot.
Rectangle {
    id: root
    property string value
    signal picked(string name)
    implicitWidth: Math.min(260, label.implicitWidth + 44)
    implicitHeight: 30
    radius: 7
    readonly property bool exists: { Engine.frameRev; return Engine.sceneNames().indexOf(value) >= 0 }
    color: area.containsMouse ? Theme.panel3 : Theme.panel2
    border.color: exists ? Theme.line : Theme.danger
    Row {
        anchors.verticalCenter: parent.verticalCenter
        x: 10
        spacing: 6
        Text { text: "→"; color: Theme.kindColor("jump"); font.family: Theme.ui; font.pixelSize: 15; font.bold: true }
        Text {
            id: label
            width: Math.min(implicitWidth, 200)
            elide: Text.ElideRight
            text: root.value === "" ? qsTr("куда?") : root.value
            color: root.exists ? Theme.text : Theme.danger
            font.family: Theme.ui
            font.pixelSize: 14
        }
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: { list.model = Engine.sceneNames(); popup.open() }
    }
    Popup {
        id: popup
        y: root.height + 4
        width: 280
        padding: 6
        background: Rectangle { radius: 9; color: "#202024"; border.color: "#3a3a40" }
        contentItem: Column {
            spacing: 2
            Text { text: qsTr("Куда ведёт"); color: Theme.dim; font.family: Theme.ui; font.pixelSize: 12; leftPadding: 8; bottomPadding: 4 }
            ListView {
                id: list
                width: parent.width
                height: Math.min(contentHeight, 300)
                clip: true
                delegate: Rectangle {
                    required property string modelData
                    width: list.width
                    height: 30
                    radius: 6
                    color: itemArea.containsMouse ? "#34343a" : "transparent"
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        x: 10
                        width: parent.width - 20
                        elide: Text.ElideRight
                        text: parent.modelData
                        color: parent.modelData === root.value ? Theme.white : Theme.text
                        font.family: Theme.ui
                        font.pixelSize: 14
                        font.weight: parent.modelData === root.value ? Font.DemiBold : Font.Normal
                    }
                    MouseArea {
                        id: itemArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: { popup.close(); root.picked(parent.modelData) }
                    }
                }
            }
            Rectangle { width: parent.width; height: 1; color: "#3a3a40" }
            Row {
                spacing: 6
                topPadding: 4
                TField {
                    id: fresh
                    width: 180
                    placeholderText: qsTr("новая сцена")
                    onAccepted: make.clicked()
                }
                TButton {
                    id: make
                    text: qsTr("Создать")
                    onClicked: {
                        var name = Engine.freeSceneName(fresh.text.trim() === "" ? qsTr("сцена") : fresh.text)
                        var was = Engine.scene
                        if (Engine.addScene(name) >= 0) {
                            Engine.scene = was
                            fresh.text = ""
                            popup.close()
                            root.picked(name)
                        }
                    }
                }
            }
        }
    }
}
