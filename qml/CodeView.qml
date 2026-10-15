import QtQuick
import QtQuick.Controls
import GenryTB

// The story as it is written (story.txt): the same mod as the cards, for those who like to type. What is
// typed here is what the cards show - and the other way round.
Rectangle {
    id: root
    color: Theme.bg
    property alias area: area

    function goToLine(line) {
        var pos = 0, n = 1, t = area.text
        while (n < line) { var i = t.indexOf("\n", pos); if (i < 0) break; pos = i + 1; n++ }
        area.cursorPosition = pos
        area.forceActiveFocus()
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: Math.max(width, area.implicitWidth + gutter.width + 20)
        contentHeight: area.implicitHeight + 40
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }
        ScrollBar.horizontal: ScrollBar { }

        function ensureVisible(r) {
            if (contentY >= r.y - 20) contentY = Math.max(0, r.y - 20)
            else if (contentY + height <= r.y + r.height + 20) contentY = r.y + r.height + 20 - height
        }

        // the line the cursor is on
        Rectangle {
            x: 0
            y: area.y + area.cursorRectangle.y
            width: flick.contentWidth
            height: area.cursorRectangle.height
            color: "#18181b"
        }
        Column {
            id: gutter
            x: 0; y: area.y + area.topPadding
            width: 46
            Repeater {
                model: area.lineCount
                Text {
                    required property int index
                    width: 38
                    height: fm.height
                    horizontalAlignment: Text.AlignRight
                    text: index + 1
                    color: index + 1 === Engine.cursorLine ? Theme.text : Theme.faint
                    font: area.font
                }
            }
        }
        FontMetrics { id: fm; font: area.font }
        TextArea {
            id: area
            x: 52; y: 8
            width: Math.max(flick.width - 60, implicitWidth)
            font.family: "Consolas"
            font.pixelSize: 15
            color: Theme.text
            selectionColor: "#ffffff"
            selectedTextColor: "#101010"
            selectByMouse: true
            wrapMode: TextEdit.NoWrap
            padding: 0
            topPadding: 2
            background: null
            Component.onCompleted: text = Engine.text
            onTextChanged: if (text !== Engine.text) Engine.text = text
            onCursorRectangleChanged: flick.ensureVisible(cursorRectangle)
            onCursorPositionChanged: {
                var n = 1, t = text
                for (var i = 0; i < cursorPosition && i < t.length; i++) if (t.charCodeAt(i) === 10) n++
                Engine.cursorLine = n
            }
        }
        StoryHighlighter { document: area.textDocument; problems: Engine.problems }
    }
    // the cards changed the story: here it is
    Connections {
        target: Engine
        function onTextChanged() {
            if (area.text !== Engine.text) {
                var pos = area.cursorPosition
                area.text = Engine.text
                area.cursorPosition = Math.min(pos, area.length)
            }
        }
    }
}
