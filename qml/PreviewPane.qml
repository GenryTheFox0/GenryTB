import QtQuick
import GenryTB

// What the player sees at the selected card - and it moves as the game moves it: the scene in layers (the
// background, each sprite, the snow), the camera's and the sprites' moves played from the very ATL that
// goes into the game; over it, still, the text box. A click plays the moves again.
Rectangle {
    id: root
    color: "#000000"
    clip: true
    property var ed
    readonly property var st: Engine.stage
    readonly property real k: width / 1920
    property real clock: 0                   // seconds the moves have been playing
    property int replay: 0                   // (bumped: every move starts over)
    // the selected card is the game's picture choice: its spots are here to be dragged where they belong
    readonly property bool spots: ed && ed.kind === "menu" && ed.blk.f.style === "spots"
    // ... or a camera that looks at a point: the point is here to be dragged
    readonly property bool camPoint: ed && ed.kind === "camera" && ed.blk.f.point === true
    readonly property bool moving: {
        if (st.camera !== "" || st.weather !== "" || st.film !== "") return true
        for (var i = 0; i < st.sprites.length; i++) if (st.sprites[i].motion !== "" || st.sprites[i].once !== "") return true
        return false
    }
    function moveSpot(i, x, y) {
        var src = ed.blk.f.options, o = []
        for (var n = 0; n < src.length; n++) o.push({ text: src[n].text, target: src[n].target, x: src[n].x, y: src[n].y })
        o[i].x = Math.round(Math.max(0, Math.min(1920, x)))
        o[i].y = Math.round(Math.max(0, Math.min(1080, y)))
        Engine.editBlock(ed.row, { options: o })
    }
    function moveCam(x, y) {
        Engine.editBlock(ed.row, { x: Math.round(Math.max(0, Math.min(1920, x))), y: Math.round(Math.max(0, Math.min(1080, y))) })
    }
    function wrap(v, m) { return ((v % m) + m) % m }

    FrameAnimation {
        running: root.visible && root.moving
        onTriggered: root.clock += frameTime
    }

    // ---- the camera ----
    property real camStart: 0
    readonly property string camKey: st.camera + "|" + st.cx + "|" + st.cy + "|" + replay
    onCamKeyChanged: camStart = clock
    readonly property var cam: st.camera !== "" ? Engine.motionAt(st.camera, true, clock - camStart, st.cx, st.cy) : ({})

    Item {
        id: scene
        anchors.fill: parent
        layer.enabled: root.st.film === "vhs" || root.st.film === "noise" || root.st.film === "tv"
        layer.effect: ShaderEffect {
            property real time: root.clock
            fragmentShader: root.st.film === "noise" ? "qrc:/shaders/shaders/noise.frag.qsb" : "qrc:/shaders/shaders/vhs.frag.qsb"
        }
        // the old television: the middle of the picture under the middle of its glass (the hub's genry_film_tv)
        transform: Translate { x: root.st.film === "tv" ? -123 * root.k : 0; y: root.st.film === "tv" ? -33 * root.k : 0 }
        Item {
            id: camItem
            width: root.width
            height: root.height
            readonly property real zm: root.cam.zoom !== undefined ? root.cam.zoom : 1
            transform: [
                Scale {
                    origin.x: (root.cam.xalign !== undefined ? root.cam.xalign : 0.5) * root.width
                    origin.y: (root.cam.yalign !== undefined ? root.cam.yalign : 0.5) * root.height
                    xScale: camItem.zm
                    yScale: camItem.zm
                },
                Rotation { origin.x: root.width / 2; origin.y: root.height / 2; angle: root.cam.rotate !== undefined ? root.cam.rotate : 0 },
                Translate {
                    x: (root.cam.xoffset !== undefined ? root.cam.xoffset : 0) * root.k
                    y: (root.cam.yoffset !== undefined ? root.cam.yoffset : 0) * root.k
                }
            ]
            Image {
                anchors.fill: parent
                source: root.st.bg !== "" ? "image://tb/bg/1280/" + encodeURIComponent(root.st.bg) : ""
                asynchronous: true
                retainWhileLoading: true
                smooth: true
            }
            Repeater {
                model: root.st.sprites.length
                Item {
                    id: sp
                    required property int index
                    readonly property var d: index < root.st.sprites.length ? root.st.sprites[index] : ({ sprite: "", pos: "", dx: 0, motion: "", once: "" })
                    // the move that lasts and the one played once, side by side (as the game stacks them)
                    property real start: 0
                    readonly property string key: d.motion + "|" + root.replay
                    onKeyChanged: start = root.clock
                    readonly property var mv: d.motion !== "" ? Engine.motionAt(d.motion, false, root.clock - start) : ({})
                    property real onceStart: 0
                    readonly property string onceKey: d.once + "|" + root.replay
                    onOnceKeyChanged: onceStart = root.clock
                    readonly property var ov: d.once !== "" ? Engine.motionAt(d.once, false, root.clock - onceStart) : ({})
                    readonly property real zm: mv.zoom !== undefined ? mv.zoom : 1
                    width: root.width
                    height: root.height
                    transform: [
                        Scale {
                            origin.x: (960 + sp.d.dx) * root.k
                            origin.y: root.height
                            xScale: sp.zm * (sp.mv.xzoom !== undefined ? sp.mv.xzoom : 1)
                            yScale: sp.zm * (sp.mv.yzoom !== undefined ? sp.mv.yzoom : 1)
                        },
                        Translate {
                            x: ((sp.mv.xoffset !== undefined ? sp.mv.xoffset : 0) + (sp.ov.xoffset !== undefined ? sp.ov.xoffset : 0)) * root.k
                            y: ((sp.mv.yoffset !== undefined ? sp.mv.yoffset : 0) + (sp.ov.yoffset !== undefined ? sp.ov.yoffset : 0)) * root.k
                        }
                    ]
                    Image {
                        anchors.fill: parent
                        source: sp.d.sprite !== "" ? "image://tb/stage/" + (sp.d.pos !== "" ? sp.d.pos : "-") + "@" + (sp.d.size !== undefined ? sp.d.size : 100) +
                                                     "/" + encodeURIComponent(sp.d.sprite) : ""
                        asynchronous: true
                        retainWhileLoading: true
                        smooth: true
                    }
                }
            }
            // the snow: the game's own snow pictures drifting, as the hub's genry_tb_snow / genry_tb_blizzard
            // (seconds for a picture to cross the screen: across, down)
            Item {
                id: snow
                anchors.fill: parent
                visible: root.st.weather !== ""
                clip: true
                Repeater {
                    model: !snow.visible ? [] : root.st.weather === "blizzard" ? [["snow02", 14, 12], ["snow01", 8, 7], ["snow00", 5, 4.5]]
                                                                              : [["snow02", 120, 24], ["snow01", 80, 14]]
                    Image {
                        required property var modelData
                        source: "image://tb/file/images/effect/snow/" + modelData[0] + ".png"
                        sourceSize: Qt.size(Math.round(root.width), Math.round(root.height))
                        width: root.width * 2
                        height: root.height * 2
                        fillMode: Image.Tile
                        horizontalAlignment: Image.AlignLeft
                        verticalAlignment: Image.AlignTop
                        asynchronous: true
                        x: -root.width + (root.clock * root.width / modelData[1]) % root.width
                        y: -root.height + (root.clock * root.height / modelData[2]) % root.height
                    }
                }
            }
        }
    }

    // over the picture, not moving with the camera (the hub's genry_tb_film screen): the old television set;
    // a memory's scratched frame, its old film's scratches flashing now and then
    Image {
        anchors.fill: parent
        visible: root.st.film === "tv"
        source: visible ? "image://tb/file/images/goosehunt/tvframe.png" : ""
        asynchronous: true
    }
    Image {
        anchors.fill: parent
        visible: root.st.film === "memory"
        source: visible ? "image://tb/file/images/effect/ramka/ramka4.png" : ""
        asynchronous: true
    }
    Image {
        readonly property real t: root.clock % 6.6
        readonly property int n: t >= 1.0 && t < 1.1 ? 1 : t >= 1.15 && t < 1.25 ? 2 : t >= 3.25 && t < 3.35 ? 3 : t >= 3.4 && t < 3.5 ? 4 : 0
        anchors.fill: parent
        visible: root.st.film === "memory" && n > 0
        source: root.st.film === "memory" ? "image://tb/file/images/effect/old_movie/old_movie_00" + Math.max(1, n) + ".png" : ""
        asynchronous: true
    }

    // the choice and the text box: over the scene, still (the game's screens layer)
    Image {
        anchors.fill: parent
        source: "image://tb/ui/" + Engine.frameRev
        asynchronous: true
        cache: false
        retainWhileLoading: true
        smooth: true
        mipmap: true
    }

    // a click: the moves play again
    MouseArea {
        anchors.fill: parent
        onClicked: root.replay++
    }

    // ---- the spots of a picture choice ----
    Repeater {
        model: root.spots ? root.ed.blk.f.options : []
        Item {
            id: spot
            required property var modelData
            required property int index
            x: modelData.x / 1920 * root.width
            y: modelData.y / 1080 * root.height
            // a ring round the word (the word stays readable), its number on the ring
            Rectangle {
                anchors.centerIn: parent
                width: 74; height: 44; radius: 22
                color: sa.pressed ? "#33ffffff" : "transparent"
                border.color: sa.containsMouse || sa.pressed ? "#ffffff" : "#b0ffffff"
                border.width: 2
            }
            Rectangle {
                x: 24; y: -34
                width: 20; height: 20; radius: 10
                color: "#ffffff"
                Text { anchors.centerIn: parent; text: spot.index + 1; color: "#101010"; font.family: Theme.ui; font.pixelSize: 12; font.bold: true }
            }
            MouseArea {
                id: sa
                x: -40; y: -26; width: 80; height: 52
                hoverEnabled: true
                preventStealing: true
                cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                property real last: 0
                function put(mouse) {
                    var p = mapToItem(root, mouse.x, mouse.y)
                    root.moveSpot(spot.index, p.x / root.width * 1920, p.y / root.height * 1080)
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    var now = Date.now()
                    if (now - last < 60) return
                    last = now
                    put(mouse)
                }
                onReleased: (mouse) => put(mouse)
            }
        }
    }

    // ---- the point the camera looks at ----
    Item {
        id: aim
        visible: root.camPoint
        x: root.camPoint ? root.ed.blk.f.x / 1920 * root.width : 0
        y: root.camPoint ? root.ed.blk.f.y / 1080 * root.height : 0
        Rectangle { anchors.centerIn: parent; width: 38; height: 38; radius: 19; color: ca.pressed ? "#44ffffff" : "#22000000"; border.color: "#ffffff"; border.width: 2 }
        Rectangle { anchors.centerIn: parent; width: 2; height: 54; color: "#ffffff" }
        Rectangle { anchors.centerIn: parent; width: 54; height: 2; color: "#ffffff" }
        MouseArea {
            id: ca
            x: -27; y: -27; width: 54; height: 54
            preventStealing: true
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
            property real last: 0
            function put(mouse) {
                var p = mapToItem(root, mouse.x, mouse.y)
                root.moveCam(p.x / root.width * 1920, p.y / root.height * 1080)
            }
            onPositionChanged: (mouse) => {
                if (!pressed) return
                var now = Date.now()
                if (now - last < 60) return
                last = now
                put(mouse)
            }
            onReleased: (mouse) => put(mouse)
        }
    }

    // trying something on: said in the corner
    Rectangle {
        visible: Engine.tryOn !== ""
        x: 10; y: 10
        width: t.implicitWidth + 18; height: 24; radius: 12
        color: "#e8ffffff"
        Text { id: t; anchors.centerIn: parent; text: qsTr("примерка"); color: "#101010"; font.family: Theme.ui; font.pixelSize: 11; font.weight: Font.DemiBold }
    }
}
