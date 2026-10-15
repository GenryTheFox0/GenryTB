import QtQuick
import QtQuick.Controls
import QtQuick.Particles
import GenryTB

// The launcher: Tiny Bunny's own main menu, from its own pictures and music - the night, the fog drifting,
// the snow, the Bunny blinking - with the constructor's buttons where the game has its own.
Item {
    id: root
    signal openEditor()
    property string panel: ""            // "" the menu | "mods" | "new" | "support" | "lang"
    // where to support the author: the free constructor lives on these
    readonly property var supportLinks: [
        { name: "Boosty", note: qsTr("подписка, ранний доступ к новым версиям"), url: "https://boosty.to/genrythefox" },
        { name: "DonationAlerts", note: qsTr("разовый донат, любая сумма"), url: "https://www.donationalerts.com/r/genrythefoxmax" },
        { name: "Patreon", note: qsTr("из-за рубежа"), url: "https://www.patreon.com/cw/GenryTheFox" },
        { name: "Telegram", note: qsTr("новости, моды, связь со мной"), url: "https://t.me/teamgenrythefox" }
    ]
    property var projects: []
    property int sel: 0
    property bool quick: Engine.shotPage !== ""      // no intro (checks)
    readonly property var cur: projects.length > sel ? projects[sel] : null

    function refresh() { projects = Engine.projects(); if (sel >= projects.length) sel = Math.max(0, projects.length - 1) }
    function open(id) { if (Engine.openProject(id)) { Engine.menuMusic = false; root.openEditor() } }
    function img(path) { return "image://tb/file/images/interface/main_meny/" + path }
    function count(n, one, few, many) {
        var k = n % 100
        var w = (k >= 11 && k <= 14) ? many : (n % 10 === 1) ? one : (n % 10 >= 2 && n % 10 <= 4) ? few : many
        return n + " " + w
    }

    Component.onCompleted: {
        refresh()
        Engine.menuMusic = true
        if (Engine.shotPage === "mods") panel = "mods"
        if (Engine.shotPage === "new") panel = "new"
        if (Engine.shotPage === "support") panel = "support"
        if (Engine.shotPage === "lang") panel = "lang"
    }
    Connections { target: Engine; function onProjectsChanged() { root.refresh() } }

    Rectangle { anchors.fill: parent; color: "black" }

    Item {
        id: stage
        width: 1920; height: 1080
        anchors.centerIn: parent
        scale: Math.min(root.width / 1920, root.height / 1080)
        clip: true

        Image { source: root.img("meny00.jpg") }

        // the far fog, drifting left
        Item {
            id: fogBack
            Image { source: root.img("meny_002.png") }
            Image { x: 3960; source: root.img("meny_002.png") }
            NumberAnimation on x { from: 0; to: -3960; duration: 160000; loops: Animation.Infinite }
        }
        ParticleSystem { id: snowBack }
        ImageParticle {
            system: snowBack
            source: root.img("partikl_002.png")
            entryEffect: ImageParticle.Fade
        }
        Emitter {
            system: snowBack
            anchors.fill: parent
            emitRate: 30
            lifeSpan: 10000
            lifeSpanVariation: 3000
            size: 16
            sizeVariation: 10
            velocity: PointDirection { x: 70; xVariation: 20; yVariation: 20 }
        }
        // the near fog, drifting right
        Item {
            Image { x: -3960; source: root.img("meny_001.png") }
            Image { source: root.img("meny_001.png") }
            NumberAnimation on x { from: 0; to: 3960; duration: 40000; loops: Animation.Infinite }
        }
        ParticleSystem { id: snowFront }
        ImageParticle {
            system: snowFront
            source: root.img("partikl_001.png")
            entryEffect: ImageParticle.Fade
        }
        Emitter {
            system: snowFront
            anchors.fill: parent
            emitRate: 9
            lifeSpan: 9000
            lifeSpanVariation: 2000
            size: 26
            sizeVariation: 14
            velocity: PointDirection { x: 125; xVariation: 25; yVariation: 20 }
        }

        // the Bunny: looking, blinking now and then
        Image {
            id: bunny
            property int frame: 1
            source: root.img("0" + frame + ".png")
            SequentialAnimation {
                running: true
                loops: Animation.Infinite
                PauseAnimation { duration: 7000 }
                PropertyAction { target: bunny; property: "frame"; value: 2 }
                PauseAnimation { duration: 100 }
                PropertyAction { target: bunny; property: "frame"; value: 3 }
                PauseAnimation { duration: 100 }
                PropertyAction { target: bunny; property: "frame"; value: 2 }
                PauseAnimation { duration: 100 }
                PropertyAction { target: bunny; property: "frame"; value: 1 }
            }
        }
        // (the frames kept ready: no flash of nothing on the first blink)
        Image { source: root.img("02.png"); visible: false }
        Image { source: root.img("03.png"); visible: false }

        // out of the dark, as the game's menu comes
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: root.quick ? 0 : 1
            NumberAnimation on opacity { running: !root.quick; from: 1; to: 0; duration: 2200 }
        }

        // ---- the menu ----
        Item {
            id: menu
            anchors.fill: parent
            opacity: root.panel === "" ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: 250 } }

            Item {
                anchors.fill: parent
                opacity: root.quick ? 1 : 0
                SequentialAnimation on opacity {
                    running: !root.quick
                    PauseAnimation { duration: 1400 }
                    NumberAnimation { from: 0; to: 1; duration: 1200 }
                }
                Image {
                    source: root.img("logo.png")
                    x: (1920 - 552) * 0.47
                    y: (1080 - 167) * 0.09
                }
                Text {
                    x: (1920 - 552) * 0.47
                    width: 552
                    y: (1080 - 167) * 0.09 + 172
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("конструктор модов")
                    font.family: Engine.textFont
                    font.pixelSize: 34
                    color: "#b8b8b8"
                }
                Column {
                    id: column
                    x: (1920 - 250) * 0.47
                    y: (1080 - height) * 0.42 + 40
                    MenuButton {
                        visible: Engine.lastProject !== ""
                        text: qsTr("Продолжить")
                        onClicked: root.open(Engine.lastProject)
                    }
                    MenuButton { text: qsTr("Новый мод"); onClicked: { nameField.text = ""; descField.text = ""; root.panel = "new"; nameField.forceActiveFocus() } }
                    MenuButton { text: qsTr("Мои моды"); onClicked: { root.refresh(); root.panel = "mods"; Engine.click("open") } }
                    MenuButton { text: qsTr("Поддержать"); primary: true; onClicked: root.panel = "support" }
                    MenuButton { text: qsTr("Выход"); onClicked: Qt.quit() }
                }
                Row {
                    id: supportStrip
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 1012
                    spacing: 34
                    Text {
                        text: qsTr("поддержать автора:")
                        font.family: Engine.textFont; font.pixelSize: 30; color: "#9a9a9a"
                    }
                    Repeater {
                        model: root.supportLinks
                        Text {
                            required property var modelData
                            text: modelData.name
                            font.family: Engine.textFont; font.pixelSize: 30
                            font.underline: linkArea.containsMouse
                            color: linkArea.containsMouse ? "#ffffff" : "#e0c27a"
                            MouseArea {
                                id: linkArea
                                anchors.fill: parent
                                anchors.margins: -8
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { Engine.click("click"); Qt.openUrlExternally(parent.modelData.url) }
                            }
                        }
                    }
                }
                // the language, where the game keeps its own: the top left
                Row {
                    x: 60; y: 44
                    spacing: 14
                    Image { source: Engine.flag(Engine.language); width: 48; height: 32; smooth: true; mipmap: true; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: qsTr("ЯЗЫК") + (Engine.language === "ru" ? "" : "  ·  LANGUAGE")
                        font.family: Engine.textFont
                        font.pixelSize: 34
                        color: langArea.containsMouse ? "#ffffff" : "#b8b8b8"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Item { id: langArea; property bool containsMouse: langHover.containsMouse }
                }
                MouseArea {
                    id: langHover
                    x: 40; y: 30; width: 420; height: 60
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: Engine.click("hover")
                    onClicked: { Engine.click("open"); root.panel = "lang" }
                }
                Text {
                    x: 60; y: 1010
                    text: "GenryTB V1"
                    font.family: Engine.textFont
                    font.pixelSize: 30
                    color: "#6e6e6e"
                }
                Text {
                    x: 1920 - 60 - width; y: 1010
                    text: Engine.menuMusic ? qsTr("музыка: вкл") : qsTr("музыка: выкл")
                    font.family: Engine.textFont
                    font.pixelSize: 30
                    color: musicArea.containsMouse ? "#ffffff" : "#6e6e6e"
                    MouseArea {
                        id: musicArea
                        anchors.fill: parent
                        anchors.margins: -10
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Engine.menuMusic = !Engine.menuMusic
                    }
                }
            }
        }

        // ---- a panel over the menu: the night darker ----
        Rectangle {
            anchors.fill: parent
            color: "#000000"
            opacity: root.panel !== "" ? 0.7 : 0
            Behavior on opacity { NumberAnimation { duration: 250 } }
        }

        // ---- Язык ----
        Item {
            anchors.fill: parent
            opacity: root.panel === "lang" ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: 250 } }
            Text {
                x: 130; y: 90
                text: qsTr("Язык")
                font.family: Engine.titleFont
                font.pixelSize: 90
                color: "#ffffff"
            }
            Text {
                x: 136; y: 200
                text: "Language · Мова · Sprache · Idioma · 言語 · 语言 · 언어"
                font.family: Engine.textFont
                font.pixelSize: 30
                color: "#8a8a8a"
            }
            // as Windows speaks
            Rectangle {
                x: 130; y: 262
                width: 1660; height: 74
                radius: 6
                readonly property bool chosen: Engine.languageSetting === "auto" || Engine.languageSetting === ""
                color: chosen ? "#2a2a2a" : autoArea.containsMouse ? "#1c1c1c" : "#00000000"
                border.color: chosen ? "#e0c27a" : "#3a3a3a"
                border.width: chosen ? 2 : 1
                Row {
                    x: 20; anchors.verticalCenter: parent.verticalCenter
                    spacing: 18
                    Image { source: Engine.flag(Engine.systemLanguage()); width: 54; height: 36; smooth: true; mipmap: true; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "⟳  " + qsTr("Как в Windows")
                        font.family: Engine.textFont; font.pixelSize: 36; color: "#ffffff"
                    }
                }
                MouseArea { id: autoArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onEntered: Engine.click("hover")
                    onClicked: { Engine.click("click"); Engine.setLanguage("auto") } }
            }
            Grid {
                x: 130; y: 360
                columns: 4
                columnSpacing: 14
                rowSpacing: 12
                Repeater {
                    model: Engine.languages
                    Rectangle {
                        id: lg
                        required property var modelData
                        width: 404; height: 84; radius: 6
                        readonly property bool chosen: Engine.languageSetting === modelData.code
                        color: chosen ? "#2a2a2a" : lgArea.containsMouse ? "#1c1c1c" : "#00000000"
                        border.color: chosen ? "#e0c27a" : lgArea.containsMouse ? "#6e6e6e" : "#3a3a3a"
                        border.width: chosen ? 2 : 1
                        scale: lgArea.pressed ? 0.97 : 1
                        Behavior on scale { NumberAnimation { duration: 90 } }
                        Row {
                            x: 18; anchors.verticalCenter: parent.verticalCenter
                            spacing: 16
                            Image { source: Engine.flag(lg.modelData.code); width: 54; height: 36; smooth: true; mipmap: true; anchors.verticalCenter: parent.verticalCenter }
                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                Text { text: lg.modelData.name; font.family: Engine.textFont; font.pixelSize: 32; color: "#ffffff" }
                                Text { text: lg.modelData.english; font.family: Engine.textFont; font.pixelSize: 22; color: "#7a7a7a" }
                            }
                        }
                        MouseArea { id: lgArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onEntered: Engine.click("hover")
                            onClicked: { Engine.click("click"); Engine.setLanguage(lg.modelData.code) } }
                    }
                }
            }
            Text {
                x: 136; y: 935
                text: qsTr("Команды мода в тексте остаются русскими: мод пишется одинаково на любом языке.")
                font.family: Engine.textFont; font.pixelSize: 26; color: "#7a7a7a"
            }
            MenuButton { x: 130; y: 970; text: qsTr("Назад"); onClicked: root.panel = "" }
        }

        // ---- Мои моды ----
        Item {
            anchors.fill: parent
            opacity: root.panel === "mods" ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: 250 } }

            Text { x: 140; y: 50; text: qsTr("Мои моды"); font.family: Engine.titleFont; font.pixelSize: 120; color: "white" }
            Image { x: 130; y: 196; source: "image://tb/file/images/interface/polosa.png" }
            Text {
                x: 150; y: 236
                text: root.projects.length ? root.count(root.projects.length, qsTr("мод"), qsTr("мода"), qsTr("модов"))
                                           : qsTr("пока ни одного — начни с «Новый мод»")
                font.family: Engine.textFont; font.pixelSize: 34; color: "#9a9a9a"
            }
            ListView {
                id: list
                x: 130; y: 300; width: 790; height: 600
                clip: true
                spacing: 8
                model: root.projects
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: list.contentHeight > list.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff }
                delegate: Item {
                    id: entry
                    required property var modelData
                    required property int index
                    readonly property bool on: index === root.sel
                    width: 770; height: 104
                    Image {
                        anchors.fill: parent
                        source: root.img("plaska.png")
                        visible: entry.on
                    }
                    Column {
                        x: 40
                        anchors.verticalCenter: parent.verticalCenter
                        Row {
                            spacing: 14
                            Text {
                                width: Math.min(implicitWidth, entry.modelData.adult ? 610 : 690); elide: Text.ElideRight
                                text: entry.modelData.name
                                font.family: Engine.titleFont; font.pixelSize: 46
                                color: entry.on ? "#000000" : "#ffffff"
                            }
                            Text {
                                visible: entry.modelData.adult === true
                                anchors.verticalCenter: parent.verticalCenter
                                text: "18+"
                                font.family: Engine.titleFont; font.pixelSize: 34
                                color: "#d63a3a"
                            }
                        }
                        Text {
                            width: 690; elide: Text.ElideRight
                            text: (entry.modelData.author ? entry.modelData.author + "  ·  " : "") + Qt.formatDateTime(new Date(entry.modelData.modified), "d MMMM, hh:mm")
                            font.family: Engine.textFont; font.pixelSize: 28
                            color: entry.on ? "#3a3a3a" : "#a8a8a8"
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: { if (root.sel !== entry.index) Engine.click("hover"); root.sel = entry.index; confirm.visible = false }
                        onClicked: { Engine.click("click"); root.open(entry.modelData.id) }
                    }
                }
            }
            Item {
                visible: root.cur !== null
                Polaroid {
                    x: 1030; y: 130
                    source: root.cur ? "image://tb/cover/" + root.cur.id + "/" + Engine.projectsRev : ""
                    title: root.cur ? root.cur.name : ""
                }
                Column {
                    x: 1040; y: 700; width: 780; spacing: 10
                    Text {
                        width: 780; wrapMode: Text.WordWrap
                        visible: text !== ""
                        text: root.cur ? root.cur.desc : ""
                        font.family: Engine.textFont; font.pixelSize: 34; color: "#e6e6e6"
                    }
                    Text {
                        text: root.cur ? root.count(root.cur.scenes, qsTr("сцена"), qsTr("сцены"), qsTr("сцен")) +
                                         (root.cur.choices ? "  ·  " + root.count(root.cur.choices, qsTr("выбор"), qsTr("выбора"), qsTr("выборов")) : "") +
                                         (root.cur.problems ? qsTr("  ·  ошибок: ") + root.cur.problems : "") : ""
                        font.family: Engine.textFont; font.pixelSize: 30; color: "#a8a8a8"
                    }
                }
                MenuButton {
                    x: 1560; y: 950
                    primary: true
                    text: qsTr("Открыть")
                    onClicked: if (root.cur) root.open(root.cur.id)
                }
                MenuButton {
                    x: 1290; y: 950
                    text: qsTr("Удалить")
                    visible: !confirm.visible
                    onClicked: confirm.visible = true
                }
                Row {
                    id: confirm
                    x: 1040; y: 950
                    visible: false
                    spacing: 0
                    Text {
                        height: 75; verticalAlignment: Text.AlignVCenter
                        text: qsTr("Убрать мод?  ")
                        font.family: Engine.textFont; font.pixelSize: 34; color: "#e6e6e6"
                    }
                    MenuButton { width: 130; text: qsTr("Да"); onClicked: { confirm.visible = false; if (root.cur) Engine.deleteProject(root.cur.id) } }
                    MenuButton { width: 130; text: qsTr("Нет"); onClicked: confirm.visible = false }
                }
            }
            MenuButton { x: 130; y: 950; text: qsTr("Назад"); onClicked: root.panel = "" }
        }

        // ---- Поддержать ----
        Item {
            anchors.fill: parent
            opacity: root.panel === "support" ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: 250 } }

            Text { x: 140; y: 50; text: qsTr("Поддержать"); font.family: Engine.titleFont; font.pixelSize: 120; color: "white" }
            Image { x: 130; y: 196; source: "image://tb/file/images/interface/polosa.png" }
            Text {
                x: 150; y: 250; width: 1600
                wrapMode: Text.WordWrap
                text: qsTr("Конструктор бесплатный, и таким останется. Я делаю его один, по вечерам. ") +
                      "Каждый донат — это новые фишки, новые версии и время, чтобы доделать. Спасибо, что ты здесь.
— Генри"
                font.family: Engine.textFont; font.pixelSize: 38; color: "#e6e6e6"; lineHeight: 1.15
            }
            Column {
                x: 150; y: 470; spacing: 22
                Repeater {
                    model: root.supportLinks
                    Item {
                        id: link
                        required property var modelData
                        width: 1500; height: 96
                        Image {
                            anchors.fill: parent
                            source: root.img("plaska.png")
                            opacity: linkPanelArea.containsMouse ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 150 } }
                        }
                        Row {
                            x: 40; anchors.verticalCenter: parent.verticalCenter; spacing: 40
                            Text {
                                width: 470
                                text: link.modelData.name
                                font.family: Engine.titleFont; font.pixelSize: 56
                                color: linkPanelArea.containsMouse ? "#000000" : "#ffffff"
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: link.modelData.note
                                font.family: Engine.textFont; font.pixelSize: 34
                                color: linkPanelArea.containsMouse ? "#2a2a2a" : "#a8a8a8"
                            }
                        }
                        MouseArea {
                            id: linkPanelArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onEntered: Engine.click("hover")
                            onClicked: { Engine.click("click"); Qt.openUrlExternally(link.modelData.url) }
                        }
                    }
                }
            }
            MenuButton { x: 130; y: 950; text: qsTr("Назад"); onClicked: root.panel = "" }
        }

        // ---- Новый мод ----
        Item {
            anchors.fill: parent
            opacity: root.panel === "new" ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: 250 } }

            Text { x: 140; y: 50; text: qsTr("Новый мод"); font.family: Engine.titleFont; font.pixelSize: 120; color: "white" }
            Image { x: 130; y: 196; source: "image://tb/file/images/interface/polosa.png" }
            Column {
                x: 150; y: 290; spacing: 34
                MenuField { id: nameField; label: qsTr("Как называется"); placeholder: qsTr("Лиса у дороги"); width: 900; onAccepted: authorField.forceActiveFocus() }
                MenuField { id: authorField; label: qsTr("Кто автор"); placeholder: qsTr("твоё имя"); width: 900; onAccepted: descField.forceActiveFocus() }
                MenuField { id: descField; label: qsTr("О чём он (пара слов — видно в игре, в «Модах»)"); placeholder: qsTr("Ночная дорога, девочка в лисьей маске…"); width: 1300; onAccepted: create.clicked() }
                Text {
                    text: nameField.text.trim() !== "" ? qsTr("папка мода: ") + Engine.modId(nameField.text) : " "
                    font.family: Engine.textFont; font.pixelSize: 28; color: "#7a7a7a"
                }
            }
            MenuButton {
                id: create
                x: 1560; y: 950
                primary: true
                text: qsTr("Создать")
                opacity: nameField.text.trim() !== "" ? 1 : 0.35
                onClicked: {
                    if (nameField.text.trim() === "") { nameField.forceActiveFocus(); return }
                    var id = Engine.createProject(nameField.text, authorField.text, descField.text)
                    if (id !== "") root.open(id)
                }
            }
            MenuButton { x: 130; y: 950; text: qsTr("Назад"); onClicked: root.panel = "" }
        }
    }

    Keys.onEscapePressed: if (panel !== "") panel = ""
    focus: true
}
