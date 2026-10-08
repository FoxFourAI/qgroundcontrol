import QtQuick
import FoxFour.Widgets 1.0

OSDCanvas {
    id: _root
    property var value
    onValueChanged: { requestPaint(); pointer.requestPaint() }
    property var _prevTicks: []
    // clip: true

    OSDCanvas {
        id: pointer
        anchors.fill: parent
        onPaint: {
            let ctx = begin()
            let r = width / 2 - style.defaultFontSize - style.fontPadding * 2
            let center = Qt.vector2d(width / 2, r)
            ctx.translate(center.x,center.y)
            ctx.rotate((360 - _root.value) * Math.PI / 180)
            ctx.beginPath()
            ctx.moveTo(0, -r + style.minorTickLength)
            ctx.lineTo(- style.minorFontSize / 2 , - r + style.minorTickLength * 2)
            ctx.lineTo(style.minorFontSize / 2 , - r + style.minorTickLength * 2)
            ctx.closePath()
            ctx.stroke()
        }
    }

    onPaint: {
        let ticks = [0, 10, 20, 30]
        let absAngle = Math.abs(value)
        if (absAngle >= 35) ticks.push(45)
        if (absAngle >= 50) ticks.push(60)
        if (_prevTicks === ticks) {
            return
        }
        _prevTicks = ticks
        let ctx = begin()

        ctx.textBaseline = "bottom"
        ctx.textAlign = "center"
        _setFontSize(ctx,style.defaultFontSize)
        let r = width / 2 - style.defaultFontSize - style.fontPadding * 2
        let center = Qt.vector2d(width / 2, r)
        let toRad = Math.PI / 180
        ctx.beginPath()
        for (let i in ticks) {
            let rad = ticks[i] * toRad
            let s = Math.sin(rad)
            let k = Math.cos(rad)
            if (ticks[i] === 0) {
                ctx.moveTo(center.x - style.minorFontSize / 2, center.y - r)
                ctx.lineTo(center.x, center.y - r + style.minorTickLength)
                ctx.lineTo(center.x + style.minorFontSize / 2, center.y - r)
                ctx.lineTo(center.x - style.minorFontSize / 2, center.y - r)
            } else {
                ctx.moveTo(center.x + r * s, center.y - r * k)
                ctx.lineTo(center.x + (r - style.minorTickLength) * s, center.y - (r - style.minorTickLength) * k)
                ctx.moveTo(center.x - r * s, center.y - r * k)
                ctx.lineTo(center.x - (r - style.minorTickLength) * s, center.y - (r - style.minorTickLength) * k)
            }
        }
        ctx.stroke()


    }
}
