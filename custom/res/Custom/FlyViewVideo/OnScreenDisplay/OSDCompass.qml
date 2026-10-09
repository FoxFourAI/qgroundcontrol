import QtQuick
import FoxFour.Widgets 1.0

OSDCanvas {
    id: root

    property real value: 0

    property real radius: Math.min(width, height) / 2
    Component.onCompleted: requestPaint()
    onValueChanged: requestPaint()

    function drawCompass(ctx) {
        ctx.textBaseline = "top"
        ctx.textAlign = "center"

        var centerX = width / 2
        var centerY = height
        var r     = centerX - style.minorFontSize / 2
        var toRad = Math.PI / 180
        var cardinals = { 0: "N", 90: "E", 180: "S", 270: "W" }

        ctx.translate(centerX, centerY)
        ctx.rotate(-value * toRad)
        ctx.translate(-centerX, -centerY)
        ctx.lineWidth = style.minorLineWidth

        for (var d = 0; d < 360; d += 10) {
            var s = Math.sin(d * toRad)
            var k = Math.cos(d * toRad)
            ctx.beginPath()
            ctx.moveTo(centerX + r * s, centerY - r * k)
            ctx.lineTo(centerX + (r - style.minorTickLength) * s, centerY - (r - style.minorTickLength) * k)
            ctx.stroke()
        }

        _setFontSize(ctx, style.defaultFontSize)
        ctx.textAlign    = "center"
        ctx.textBaseline = "top"
        for (var l = 0; l < 360; l += 30) {
            ctx.save()
            ctx.translate(centerX, centerY)
            ctx.rotate(l * toRad)
            ctx.fillText(cardinals[l] !== undefined ? cardinals[l] : (l / 10).toString(), 0, -r + style.fontPadding)
            ctx.restore()
        }
    }


    onPaint: {
        if(!visible) {
            return
        }

        var ctx = begin()

        drawCompass(ctx)
        ctx.resetTransform()
        ctx.beginPath()
        ctx.moveTo(width / 2 - style.minorFontSize / 2, style.majorLineWidth / 2)
        ctx.lineTo(width / 2 , style.majorLineWidth + style.minorFontSize)
        ctx.lineTo(width / 2 + style.minorFontSize / 2, style.majorLineWidth / 2)
        ctx.closePath()
        ctx.save()
        ctx.clip()
        ctx.clearRect(0, 0, width, height)
        ctx.restore()
        ctx.lineWidth = style.majorLineWidth
        ctx.stroke()

    }
}
