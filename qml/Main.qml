import QtQuick
import QtQuick.Controls
import QtQuick.Window
import GenryTB

ApplicationWindow {
    id: win
    readonly property bool shot: Engine.shotPage !== ""
    width: 1600
    height: 900
    minimumWidth: 1180
    minimumHeight: 680
    x: shot ? -8000 : (Screen.width - width) / 2
    y: shot ? 0 : Math.max(0, (Screen.height - height) / 2 - 20)
    visible: true
    color: Theme.bg
    title: Engine.projectId !== "" && page === "editor" ? Engine.projectName + " — GenryTB" : qsTr("GenryTB — конструктор модов «Зайчика»")

    property string page: Engine.shotPage.indexOf("editor") === 0 && Engine.projectId !== "" ? "editor" : "launcher"

    // the game is not there: say so, plainly
    Column {
        anchors.centerIn: parent
        visible: !Engine.ready
        spacing: 12
        Text { text: qsTr("Не нашёл игру"); color: Theme.white; font.family: Theme.ui; font.pixelSize: 34; font.bold: true }
        Text { text: Engine.gameError; color: Theme.dim; font.family: Theme.ui; font.pixelSize: 18; wrapMode: Text.WordWrap; width: 700 }
    }

    Loader {
        anchors.fill: parent
        active: Engine.ready && win.page === "launcher"
        sourceComponent: Launcher { onOpenEditor: win.page = "editor" }
    }
    Loader {
        anchors.fill: parent
        active: Engine.ready && win.page === "editor"
        sourceComponent: Editor {
            onBack: { Engine.closeProject(); win.page = "launcher" }
        }
    }

    // what the program has to say (a mod installed, a line to fix)
    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 26
        width: toastText.implicitWidth + 44
        height: 46
        radius: 23
        color: "#f2f2f2"
        opacity: 0
        visible: opacity > 0
        z: 100
        Text {
            id: toastText
            anchors.centerIn: parent
            color: "#101010"
            font.family: Theme.ui
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }
        SequentialAnimation {
            id: toastAnim
            NumberAnimation { target: toast; property: "opacity"; to: 1; duration: 160 }
            PauseAnimation { duration: 3600 }
            NumberAnimation { target: toast; property: "opacity"; to: 0; duration: 400 }
        }
        Connections {
            target: Engine
            function onToastChanged() { toastText.text = Engine.toast; toastAnim.restart() }
        }
    }
}
