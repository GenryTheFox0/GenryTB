import QtQuick
import GenryTB

// A mod's photograph: its first scene on white card, its name written under it (as in the game's «Моды»).
Item {
    id: root
    property string source
    property string title
    width: 800
    height: 540
    rotation: -1.6
    Rectangle {
        anchors.fill: parent
        color: "#efefe9"
    }
    Rectangle {
        x: 20; y: 20; width: 760; height: 428
        color: "#161616"
        Image {
            anchors.fill: parent
            source: root.source
            asynchronous: true
            fillMode: Image.PreserveAspectCrop
            retainWhileLoading: true
        }
    }
    Text {
        x: 28; y: 458
        width: 744
        elide: Text.ElideRight
        text: root.title
        font.family: Engine.titleFont
        font.pixelSize: 44
        color: "#141414"
    }
}
