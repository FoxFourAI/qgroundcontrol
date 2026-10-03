import QtQuick

import QGroundControl

OSDCanvas {
    id: _root
    property real   tickSpacing:        style.majorFontSize
    property real   tickStep:           1
    property real   majorEvery:         5
    property real   majorTickLength:    style.majorFontSize
    property real   minorTickLength:    style.minorFontSize
    property real   currentValuePos: 0.5 //maps to 0-1 for currentValuePosition on the ladder
    property real   currentValueFontSize : style.majorFontSize * 1.3

    property real   from:     -Infinity
    property real   to:       Infinity
    property real   wrap:     0 //if set , it will wrap value instead
    property int    valueDecimals: 1
    property bool   mirrored: false

    property int orientation: Qt.Vertical

    //drawers
    property var drawer: standardDrawer
    property var ticksDrawer: standardTickDrawer
    property var valueDrawer: standardValueDrawer

    property var value
    property var header: ""
    property var footer: ""

    //named ticks, sectors and target value
    property var namedTicks: []
    property var sectors:    []
    property var targetValue

    property var _repaintKeys: [width, height, value, header, footer, namedTicks, sectors, orientation, valueDecimals, wrap, from, to, tickStep, drawer, ticksDrawer]
    on_RepaintKeysChanged: requestPaint()

    function _wrapValue(v) {
        return wrap > 0 ? ((v % wrap) + wrap) % wrap : v
    }

    //standard vale drawer
    function standardValueDrawer(ctx, start, end) {
        let totalLenth = end.minus(start)
        _setFont(ctx, currentValueFontSize)
        let textWidth = ctx.measureText("000.0").width
        let currPos = _normToCoord(Qt.vector2d(start.x + totalLenth.x * currentValuePos, start.y + totalLenth.y * currentValuePos))
        let fillRect = Qt.rect(0,0,10,10)
        if(orientation === Qt.Vertical) {
            currPos.x += majorTickLength * (mirrored? 1 : -1)
            fillRect.x = currPos.x + textWidth * (mirrored ? 0 : -1)
            fillRect.y = currPos.y - currentValueFontSize / 2
            fillRect.width = textWidth
            fillRect.height = currentValueFontSize

        } else {
            currPos.y += majorTickLength * (mirrored? -1 : 1)
            fillRect.x = currPos.x - textWidth / 2
            fillRect.y = currPos.y - currentValueFontSize * (mirrored ? 1 :0)
            fillRect.width = textWidth
            fillRect.height = currentValueFontSize
        }

        ctx.clearRect(fillRect.x,fillRect.y,fillRect.width,fillRect.height)
        ctx.strokeRect(fillRect.x,fillRect.y,fillRect.width,fillRect.height)

    }

    //standard tick drawer draw ticks from the middle to the edges
    function standardTickDrawer(ctx, start, end) {
        if (tickStep <= 0 || tickSpacing <= 0 || value === undefined)
            return

        // translate noramal coords to pixels
        const startPx = _normToCoord(start)
        const endPx = _normToCoord(end)

        const direction = endPx.minus(startPx)
        const ladderLengthPx = direction.length()
        const normalizedDir = direction.normalized()

        //noraml vector to draw ticks
        let normal = Qt.vector2d(-normalizedDir.y, normalizedDir.x)

        if (!mirrored) {
            normal = normal.times(-1)
        }
        if (orientation == Qt.Horizontal) {
            normal = normal.times(-1)
        }

        // drawing base line
        ctx.beginPath()
        ctx.moveTo(startPx.x, startPx.y)
        ctx.lineTo(endPx.x, endPx.y)
        ctx.stroke()

        //calculate value range
        const totalValueRange = (ladderLengthPx / tickSpacing) * tickStep
        const valueAtStart = value - (currentValuePos * totalValueRange)
        const valueAtEnd = valueAtStart + totalValueRange

        const firstTickIndex = Math.floor(Math.min(valueAtStart, valueAtEnd) / tickStep)
        const lastTickIndex = Math.ceil(Math.max(valueAtStart, valueAtEnd) / tickStep)

        ctx.textAlign = (orientation === Qt.Horizontal) ? "center" : (mirrored ? "left" : "right")
        ctx.textBaseline = (orientation === Qt.Vertical) ? "middle" : (mirrored ? "bottom" : "top")

        for (let i = firstTickIndex; i <= lastTickIndex; i++ ) {

            let val = i * tickStep
            if (val < from || val > to)
                continue

            let valueOffset = val - valueAtStart
            let pixelOffset = (valueOffset / tickStep) * tickSpacing

            if (pixelOffset < -currentValuePos || pixelOffset > ladderLengthPx )
                continue

            let tickBaseX = startPx.x + normalizedDir.x * pixelOffset
            let tickBaseY = startPx.y + normalizedDir.y * pixelOffset

            let isMajor = (val % majorEvery === 0)
            let currentTickLength = isMajor ? majorTickLength : minorTickLength
            let tickEndX = tickBaseX + normal.x * currentTickLength
            let tickEndY = tickBaseY + normal.y * currentTickLength

            ctx.beginPath()
            ctx.moveTo(tickBaseX, tickBaseY)
            ctx.lineTo(tickEndX, tickEndY)
            ctx.lineWidth = isMajor ? (style.majorLineWidth) : (style.minorLineWidth)
            ctx.stroke()

            if (isMajor) {
                let displayValue = _wrapValue(val)
                let textX = tickEndX
                let textY = tickEndY

                if (orientation === Qt.Vertical) {
                    textX += style.majorFontSize * mirrored ? 2.5 : -2.5
                }

                ctx.fillText(displayValue.toFixed(valueDecimals), textX, textY)
            }
        }
    }


    function standardDrawer(ctx) {
        const lw = style.majorLineWidth
        let pW = (lw + style.defaultFontSize * 3) / width
        let pH = (lw + style.defaultFontSize * 2) / height
        let ladderStart = Qt.vector2d(0, 0)
        let ladderEnd   = Qt.vector2d(0, 0)

        if (orientation === Qt.Horizontal) {
            ladderStart.x = lw / width
            ladderStart.y = pH
            ladderEnd.x = 1 - ladderStart.x          // left -> right increasing
            if (mirrored) ladderStart.y = 1 - ladderStart.y
            ladderEnd.y = ladderStart.y
        } else {
            ladderStart.x = pW
            ladderStart.y = 1 - lw / width
            ladderEnd.y = 1 - ladderStart.y          // bottom -> top increasing
            if (footer !== "") {
                ladderStart.y -= style.majorFontSize / height
            }
            if (header !== "") {
                ladderEnd.y += style.majorFontSize / height
            }


            if (!mirrored) ladderStart.x = 1 - ladderStart.x
            ladderEnd.x = ladderStart.x
        }


        ticksDrawer(ctx, ladderStart, ladderEnd)
        valueDrawer(ctx, ladderStart, ladderEnd)

        _setFont(ctx, style.majorFontSize)
        //draw header and footer
        let headerAlign = (orientation === Qt.Horizontal) ? "left" : "center"
        let footerAlign = (orientation === Qt.Horizontal) ? "right" : "center"
        let headerBaseline = ""
        let footerBaseline = ""
        let headerPos = Qt.point(0, 0)
        let footerPos = Qt.point(0, 0)
        if(orientation === Qt.Horizontal) {
            headerPos.x = style.majorFontSize / 2
            headerPos.y = height * ladderStart.y + majorTickLength * (mirrored ? -2 : 2)
            footerPos.x = width - style.majorFontSize / 2
            footerPos.y = height * ladderEnd.y + majorTickLength * (mirrored ? -2 : 2)
            headerBaseline = mirrored ? "bottom": "top"
            footerBaseline = mirrored ? "bottom": "top"
        } else {
            footerPos = _normToCoord(ladderStart)
            headerPos = _normToCoord(ladderEnd)
            headerBaseline = "bottom"
            footerBaseline = "top"
        }

        ctx.textAlign = headerAlign
        ctx.textBaseline = headerBaseline
        ctx.fillText(header,headerPos.x, headerPos.y)
        ctx.textAlign = footerAlign
        ctx.textBaseline = footerBaseline
        ctx.fillText(footer,footerPos.x, footerPos.y)
    }

    onPaint: {
        let ctx = begin()
        drawer(ctx)
    }

}
