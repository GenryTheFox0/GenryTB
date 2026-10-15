import QtQuick
import QtQuick.Controls
import GenryTB

// A character put together row by row, each choice a picture: who - the feeling - the clothes and the
// light - the pose - the eyes - the mouth. Point at a picture: the preview tries it on.
Item {
    id: root
    property string value                 // the sprite the card has
    signal picked(string sprite)
    property string who: ""
    property var sel: ({})
    property var opts: who === "" ? ({}) : Engine.spriteOptions(who, sel)
    property var owners: Engine.spriteOwners()
    readonly property var dims: [
        { k: "emotion", t: qsTr("Настроение") }, { k: "clothes", t: qsTr("Одежда и свет") }, { k: "pose", t: qsTr("Поза") },
        { k: "look", t: qsTr("Глаза") }, { k: "eyes", t: qsTr("Взгляд") }, { k: "mouth", t: qsTr("Рот") }
    ]

    function load() {
        var d = Engine.spriteDims(value)
        if (d.who === undefined) return
        var s = {}
        for (var i = 0; i < dims.length; i++) s[dims[i].k] = d[dims[i].k]
        who = d.who
        sel = s
    }
    function choose(dim, v) {
        // the rows above stay as chosen; this row takes v; the rows below keep their choice where the game
        // has it with the new one - else take the one the game shows most
        var s = {}
        var i
        for (i = 0; i < dims.length; i++) {
            var k = dims[i].k
            if (k === dim) { s[k] = v; break }
            if (sel[k] !== undefined) s[k] = sel[k]
        }
        for (i = i + 1; i < dims.length; i++) {
            var kk = dims[i].k
            var row = Engine.spriteOptions(who, s)[kk] || []
            var have = false
            for (var r = 0; r < row.length; r++) if (row[r].v === sel[kk]) have = true
            if (have) s[kk] = sel[kk]
            else if (row.length) s[kk] = row[0].v
        }
        sel = s
        var sprite = Engine.spriteOptions(who, s).sprite
        if (sprite !== "") root.picked(sprite)
    }
    function setWho(w) {
        who = w
        sel = ({})
        var sprite = Engine.spriteOptions(w, {}).sprite
        var d = Engine.spriteDims(sprite)
        var s = {}
        for (var i = 0; i < dims.length; i++) s[dims[i].k] = d[dims[i].k]
        sel = s
        if (sprite !== "") root.picked(sprite)
    }
    onValueChanged: if (Engine.spriteDims(value).who !== undefined && value !== (opts.sprite || "")) load()
    Component.onCompleted: load()

    // who
    ListView {
        id: ownersList
        x: 8; y: 8
        width: 150
        height: parent.height - 16
        clip: true
        model: root.owners
        spacing: 2
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }
        delegate: Rectangle {
            id: ow
            required property var modelData
            width: ownersList.width - 10
            height: 44
            radius: 7
            readonly property bool on: modelData.who === root.who
            color: on ? "#2c2c31" : oa.containsMouse ? "#1f1f23" : "transparent"
            border.color: on ? "#9a9aa2" : "transparent"
            Image {
                x: 4; y: 4; width: 36; height: 36
                source: "image://tb/face/72/" + encodeURIComponent(ow.modelData.thumb)
                asynchronous: true
                fillMode: Image.PreserveAspectCrop
            }
            Column {
                x: 46
                anchors.verticalCenter: parent.verticalCenter
                Text { width: ow.width - 52; elide: Text.ElideRight; text: qsTr(ow.modelData.name); color: ow.on ? Theme.white : Theme.text; font.family: Theme.ui; font.pixelSize: 13; font.weight: ow.on ? Font.DemiBold : Font.Normal }
                Text { text: ow.modelData.count + qsTr(" видов"); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 10 }
            }
            MouseArea { id: oa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.setWho(ow.modelData.who) }
        }
    }

    // the rows
    Flickable {
        id: rows
        x: 166; y: 6
        width: parent.width - 172
        height: parent.height - 12
        contentHeight: col.implicitHeight + 10
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }
        Column {
            id: col
            width: rows.width - 12
            spacing: 4
            Repeater {
                model: root.dims
                Column {
                    id: dimRow
                    required property var modelData
                    readonly property var items: root.opts[modelData.k] || []
                    visible: items.length > 0
                    width: col.width
                    spacing: 3
                    Text { text: dimRow.modelData.t.toUpperCase(); color: Theme.faint; font.family: Theme.ui; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.8 }
                    ListView {
                        id: strip
                        width: parent.width
                        height: 96
                        orientation: ListView.Horizontal
                        clip: true
                        spacing: 5
                        model: dimRow.items
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.horizontal: ScrollBar { height: 6 }
                        delegate: Rectangle {
                            id: optCard
                            required property var modelData
                            readonly property bool on: root.opts.is !== undefined && root.opts.is[dimRow.modelData.k] === modelData.v
                            width: 74; height: 88
                            radius: 6
                            color: on ? "#303036" : ca.containsMouse ? "#222226" : "#17171a"
                            border.color: on ? Theme.white : "transparent"
                            border.width: 2
                            Image {
                                x: 4; y: 4; width: 66; height: 62
                                source: dimRow.modelData.k === "pose" || dimRow.modelData.k === "clothes"
                                        ? "image://tb/sprite/128/" + encodeURIComponent(optCard.modelData.thumb)
                                        : "image://tb/face/128/" + encodeURIComponent(optCard.modelData.thumb)
                                asynchronous: true
                                fillMode: Image.PreserveAspectFit
                            }
                            Text {
                                x: 3; y: 68; width: 68
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                                text: optCard.modelData.label
                                color: optCard.on ? Theme.white : Theme.dim
                                font.family: Theme.ui
                                font.pixelSize: 10
                            }
                            MouseArea {
                                id: ca
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onEntered: Engine.tryOn = optCard.modelData.thumb
                                onExited: if (Engine.tryOn === optCard.modelData.thumb) Engine.tryOn = ""
                                onClicked: { Engine.tryOn = ""; root.choose(dimRow.modelData.k, optCard.modelData.v) }
                            }
                            ToolTip.visible: ca.containsMouse
                            ToolTip.delay: 600
                            ToolTip.text: optCard.modelData.label + "  (" + optCard.modelData.v + ")"
                        }
                    }
                }
            }
        }
    }
}
