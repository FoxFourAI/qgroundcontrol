import QtQuick

QtObject {
    property var value: 0
    property int samples: 100
    property var history: []

    function avg () {
        return history.reduce((partSum, a) => partSum + a.value, 0) / history.length
    }

    function avgDiff() {
        if (history.length < 2){
            return 0
        }
        let sum = 0

        for (var i = 1; i < history.length; ++i) {
            sum += history[i].value - history[i - 1].value
        }

        return sum / (history.length - 1)
    }

    function avgRate() {
        if (history.length < 2)
            return 0

        var first = history[0]
        var last = history[history.length - 1]

        var dt = (last.date - first.date) / 1000.0

        if (dt <= 0)
            return 0

        return (last.value - first.value) / dt
    }

    onValueChanged: {
        let data = history.slice()
        let date = Date.now()
        data.push({date: date, value: value})

        if (data.length > samples) {
            data.shift()
        }
        history = data
    }

    onSamplesChanged: {
        var data = history.slice()

        while (data.length > samples) {
            data.shift()
        }

        history = data
    }
}

