import QtQuick

import QGroundControl

OSDCanvas {
    id: _root
    implicitWidth: currentValueFontSize * 5
    implicitHeight: currentValueFontSize * 5
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
    //{name, value}
    property var namedTicks: []
    //{name, from, to, color}
    property var sectors:    []

    property var targetValue

    property var _repaintKeys: [width, height, value, header, footer, namedTicks, sectors, orientation, valueDecimals, wrap, from, to, tickStep, drawer, ticksDrawer]
    on_RepaintKeysChanged: requestPaint()

    function _wrapValue(v) {
        return wrap > 0 ? ((v % wrap) + wrap) % wrap : v
    }

    //standard vale drawer
    function standardValueDrawer(ctx, start, end) {

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

        let totalLenth = end.minus(start)
        _setFont(ctx, currentValueFontSize)
        ctx.textAlign = "center"
        ctx.textBaseline = "middle"
        let textWidth = ctx.measureText(value.toFixed(1)).width + style.fontPadding
        let currPos = _normToCoord(Qt.vector2d(start.x + totalLenth.x * currentValuePos, start.y + totalLenth.y * currentValuePos))

        ctx.beginPath()
        ctx.lineTo(currPos.x + normalizedDir.x * textWidth / 2 + normal.x * minorTickLength, currPos.y + normalizedDir.y * currentValueFontSize / 2 + normal.y * minorTickLength, 8, 8)
        ctx.lineTo(currPos.x + normalizedDir.x * textWidth / 2 + normal.x * (minorTickLength + textWidth), currPos.y + normalizedDir.y * currentValueFontSize / 2 + normal.y * (minorTickLength + currentValueFontSize), 8, 8)
        ctx.lineTo(currPos.x - normalizedDir.x * textWidth / 2 + normal.x * (minorTickLength + textWidth), currPos.y - normalizedDir.y * currentValueFontSize / 2 + normal.y * (minorTickLength + currentValueFontSize), 8, 8)
        ctx.lineTo(currPos.x - normalizedDir.x * textWidth / 2 + normal.x * minorTickLength, currPos.y - normalizedDir.y * currentValueFontSize / 2 + normal.y * minorTickLength, 8, 8)

        ctx.lineTo(currPos.x - normalizedDir.x * currentValueFontSize / 2 + normal.x * minorTickLength, currPos.y - normalizedDir.y * currentValueFontSize / 2 + normal.y * minorTickLength)
        ctx.lineTo(currPos.x, currPos.y)
        ctx.lineTo(currPos.x + normalizedDir.x * currentValueFontSize / 2 + normal.x * minorTickLength, currPos.y + normalizedDir.y * currentValueFontSize / 2 + normal.y * minorTickLength)
        ctx.closePath()

        //clearing Path
        ctx.save()
        ctx.clip()
        ctx.clearRect(0,0,width,height)
        ctx.restore()
        //fill the background
        ctx.fillStyle = style.boxColor
        ctx.fill()
        ctx.fillStyle = style.color
        ctx.stroke()

        ctx.fillText(value.toFixed(1),
                     currPos.x + normal.x * (minorTickLength + textWidth / 2), currPos.y + normal.y * (minorTickLength + currentValueFontSize / 2) + currentValueFontSize / 10) //currentValueFontSize / 10 kinda hack, because textBaseline "center" is not actually at the center
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
                let displayValue = _wrapValue(val).toFixed(valueDecimals)

                if (orientation === Qt.Vertical) {
                    tickEndX += style.fontPadding * (mirrored ? 0.5 : -0.5)
                }

                ctx.fillText(displayValue, tickEndX, tickEndY)
            }
        }

        ctx.textAlign = (orientation === Qt.Horizontal) ? "center" : (mirrored ? "right" : "left")
        ctx.textBaseline = (orientation === Qt.Vertical) ? "middle" : (mirrored ? "top" : "bottom")

        ctx.lineWidth = style.minorLineWidth

        //drawing sectors
        for(let i = 0; i < sectors.length; ++i) {
            let sectorData = sectors[i]
            if( sectorData.from > sectorData.to ||
                    sectorData.from === sectorData.to ||
                    sectorData.from > valueAtEnd ||
                    sectorData.to < valueAtStart) {
                continue
            }

            //calculate start coordinates
            let startCoord = Qt.vector2d(startPx.x,startPx.y)
            if (sectorData.from > valueAtStart) {
                let startValueOffset = sectorData.from - valueAtStart
                let startPixelOffset = (startValueOffset / tickStep) * tickSpacing
                startCoord.x += normalizedDir.x * startPixelOffset
                startCoord.y += normalizedDir.y * startPixelOffset
            }

            //calculate end coordinates
            let endCoord = Qt.vector2d(startPx.x,startPx.y)
            if (sectorData.to < valueAtEnd) {
                let endValueOffset = sectorData.to - valueAtStart
                let endPixelOffset = (endValueOffset / tickStep) * tickSpacing
                endCoord.x += normalizedDir.x * endPixelOffset
                endCoord.y += normalizedDir.y * endPixelOffset
            } else {
                endCoord = Qt.vector2d(endPx.x, endPx.y)
            }

            let rect = Qt.rect(startCoord.x, startCoord.y,
                               endCoord.x - startCoord.x - normal.x * minorTickLength,
                               endCoord.y - startCoord.y - normal.y * minorTickLength)
            ctx.strokeStyle = sectorData.color
            ctx.fillStyle = sectorData.color
            //filling rectangle
            ctx.globalAlpha = 0.3
            ctx.fillRect(rect.x, rect.y, rect.width, rect.height)
            ctx.globalAlpha = 1
            ctx.strokeRect(rect.x, rect.y, rect.width, rect.height)
        }

        ctx.strokeStyle = style.color
        ctx.fillStyle = style.color

        //drawing named ticks
        for (let i = 0; i < namedTicks.length; ++i) {
            let tickData = namedTicks[i]
            if (tickData.value < from || tickData.value > to)
                continue
            let valueOffset = tickData.value - valueAtStart
            let pixelOffset = (valueOffset / tickStep) * tickSpacing

            if (pixelOffset < -currentValuePos || pixelOffset > ladderLengthPx )
                continue

            let tickBaseX = startPx.x + normalizedDir.x * pixelOffset
            let tickBaseY = startPx.y + normalizedDir.y * pixelOffset

            let tickEndX = tickBaseX - normal.x * minorTickLength
            let tickEndY = tickBaseY - normal.y * minorTickLength

            ctx.beginPath()
            ctx.moveTo(tickBaseX, tickBaseY)
            ctx.lineTo(tickEndX, tickEndY)
            ctx.stroke()

            if (orientation === Qt.Vertical) {
                tickEndX += style.fontPadding * (mirrored ? -0.5 : 0.5)
            }
            ctx.fillText(tickData.name, tickEndX, tickEndY)
        }

        //draw target value indicator
        if (isNaN(targetValue) || targetValue < valueAtStart || targetValue > valueAtEnd){
            return
        }

        let targetValueOffset = targetValue - valueAtStart
        let targetPixelOffset = (targetValueOffset / tickStep) * tickSpacing
        let targetCoord = Qt.vector2d(startPx.x + normalizedDir.x * targetPixelOffset, startPx.y + normalizedDir.y * targetPixelOffset)

        ctx.beginPath()

        ctx.moveTo(targetCoord.x + normalizedDir.x * currentValueFontSize + normal.x * minorTickLength, targetCoord.y + normalizedDir.y * currentValueFontSize + normal.y * minorTickLength)
        ctx.lineTo(targetCoord.x + normalizedDir.x * currentValueFontSize - normal.x * minorTickLength, targetCoord.y + normalizedDir.y * currentValueFontSize - normal.y * minorTickLength)
        ctx.lineTo(targetCoord.x - normalizedDir.x * currentValueFontSize - normal.x * minorTickLength, targetCoord.y - normalizedDir.y * currentValueFontSize - normal.y * minorTickLength)
        ctx.lineTo(targetCoord.x - normalizedDir.x * currentValueFontSize + normal.x * minorTickLength, targetCoord.y - normalizedDir.y * currentValueFontSize + normal.y * minorTickLength)
        ctx.lineTo(targetCoord.x - normalizedDir.x * currentValueFontSize / 2 + normal.x * minorTickLength, targetCoord.y - normalizedDir.y * currentValueFontSize / 2 + normal.y * minorTickLength)
        ctx.lineTo(targetCoord.x, targetCoord.y)
        ctx.lineTo(targetCoord.x + normalizedDir.x * currentValueFontSize / 2 + normal.x * minorTickLength, targetCoord.y + normalizedDir.y * currentValueFontSize / 2 + normal.y * minorTickLength)


        ctx.closePath()
        ctx.stroke()
        ctx.globalAlpha = 0.3
        ctx.fill()
        ctx.globalAlpha = 1

    }


    function standardDrawer(ctx) {
        if (isNaN(value)) {
            return
        }

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

        _setFont(ctx, style.defaultFontSize)
        //draw header and footer
        let headerAlign = (orientation === Qt.Horizontal) ? "left" : "center"
        let footerAlign = (orientation === Qt.Horizontal) ? "right" : "center"
        let headerBaseline = ""
        let footerBaseline = ""
        let headerPos = Qt.point(0, 0)
        let footerPos = Qt.point(0, 0)
        if(orientation === Qt.Horizontal) {
            headerPos.x = style.defaultFontSize / 2
            headerPos.y = height * ladderStart.y + minorTickLength + style.defaultFontSize * (mirrored ? -3 : 3)
            footerPos.x = width - style.defaultFontSize / 2
            footerPos.y = height * ladderEnd.y + minorTickLength + style.defaultFontSizea
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
