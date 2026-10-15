pragma Singleton
import QtQuick

// The constructor's look: the game's own black and white, its fonts for headings; plain UI font for work.
QtObject {
    readonly property color bg: "#0b0b0c"
    readonly property color panel: "#141416"
    readonly property color panel2: "#1c1c1f"
    readonly property color panel3: "#26262a"
    readonly property color line: "#2e2e33"
    readonly property color text: "#ececec"
    readonly property color dim: "#96969c"
    readonly property color faint: "#5c5c62"
    readonly property color white: "#ffffff"
    readonly property color danger: "#e8635f"
    readonly property color ok: "#8fd6a0"
    readonly property string ui: "Segoe UI"
    readonly property string emoji: "Segoe UI Emoji"
    readonly property color blood: "#d63a3a"        // the game's red: the one thing to press

    // a kind's sign (an emoji: a child reads it before the word)
    function kindIcon(kind) {
        switch (kind) {
        case "say": return "💬"
        case "thought": return "💭"
        case "narrator": return "📖"
        case "note": return "📝"
        case "bg": return "🏞️"
        case "show": return "🧍"
        case "hide": return "🚪"
        case "weather": return "❄️"
        case "film": return "📼"
        case "paper": return "✉️"
        case "poster": return "📌"
        case "camera": return "🎥"
        case "move": return "🏃"
        case "shake": return "💥"
        case "flash": return "⚡"
        case "pause": return "⏸️"
        case "window": return "🪟"
        case "music": return "🎵"
        case "amb": return "🌬️"
        case "sound": return "🔔"
        case "voice": return "🎙️"
        case "menu": return "🔀"
        case "jump": return "➡️"
        case "set": case "add": return "⭐"
        case "if": return "❓"
        case "end": return "🏁"
        case "suefa": return "✊"
        case "struggle": return "💪"
        case "riddle": return "🧩"
        case "explore": return "🔍"
        case "snap": return "📷"
        case "title": return "🔠"
        case "epilogue": return "📜"
        case "credits": return "🎬"
        case "button": return "🔘"
        case "game": return "🎮"
        case "return": return "↩️"
        case "code": return "⌨️"
        case "phone": return "☎️"
        case "ach": return "🏆"
        case "inhand": return "✋"
        case "puzzle": return "🖼️"
        case "bad": return "⚠️"
        }
        return "•"
    }

    // a card's kind: its colour, its name, its sign
    function kindColor(kind) {
        switch (kind) {
        case "say": return "#ffd28a"
        case "bg": return "#8fb8ff"
        case "show": return "#9fe0b0"
        case "hide": return "#74b08a"
        case "music": case "amb": case "sound": case "voice": return "#d7a6ff"
        case "menu": return "#ff9a8a"
        case "jump": return "#ffb070"
        case "set": case "add": case "if": return "#7fd6d6"
        case "end": return "#e8635f"
        case "shake": case "flash": case "pause": case "window": return "#e8de8a"
        case "camera": case "move": case "weather": return "#ff9fd6"
        case "film": case "paper": case "poster": return "#e9dcae"
        case "bad": return "#e8635f"
        case "title": case "epilogue": case "credits": case "button": return "#f2c46b"
        case "struggle": case "suefa": case "riddle": case "explore": case "snap": case "phone": case "ach": return "#7fe0ff"
        case "inhand": return "#e9dcae"
        case "puzzle": return "#7fe0ff"
        case "code": case "game": case "return": return "#b0a4ff"
        }
        return "#77777d"
    }
    function kindName(kind) {
        switch (kind) {
        case "say": return qsTr("Реплика")
        case "bg": return qsTr("Фон")
        case "show": return qsTr("Персонаж")
        case "hide": return qsTr("Уходит")
        case "music": return qsTr("Музыка")
        case "amb": return qsTr("Фоновый звук")
        case "sound": return qsTr("Звук")
        case "voice": return qsTr("Голос")
        case "menu": return qsTr("Выбор")
        case "jump": return qsTr("Переход")
        case "set": return qsTr("Запомнить")
        case "add": return qsTr("Изменить")
        case "if": return qsTr("Условие")
        case "end": return qsTr("Конец")
        case "shake": return qsTr("Тряска")
        case "flash": return qsTr("Вспышка")
        case "pause": return qsTr("Пауза")
        case "window": return qsTr("Окно реплик")
        case "note": return qsTr("Заметка")
        case "camera": return qsTr("Камера")
        case "move": return qsTr("Движение")
        case "weather": return qsTr("Погода")
        case "film": return qsTr("Плёнка")
        case "paper": return qsTr("Записка")
        case "poster": return qsTr("Объявление")
        case "bad": return qsTr("Непонятная строка")
        case "title": return qsTr("Надпись")
        case "epilogue": return qsTr("Эпилог")
        case "credits": return qsTr("Титры")
        case "button": return qsTr("Кнопка")
        case "struggle": return qsTr("Борьба")
        case "suefa": return qsTr("Камень-ножницы-бумага")
        case "riddle": return qsTr("Загадка")
        case "explore": return qsTr("Осмотр")
        case "snap": return qsTr("Снимок")
        case "phone": return qsTr("Телефон")
        case "ach": return qsTr("Достижение")
        case "inhand": return qsTr("Предмет в руке")
        case "puzzle": return qsTr("Пазл")
        case "code": return qsTr("Код")
        case "game": return qsTr("В игру")
        case "return": return qsTr("Назад в игру")
        }
        return kind
    }
}
