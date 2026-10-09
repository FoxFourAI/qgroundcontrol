import QtQuick

import QGroundControl
import FoxFour.Widgets 1.0
OSDCanvas {
    id: _root
    implicitWidth: style.currentValueFontSize * 5 + style.majorFontSize * 2
    implicitHeight: style.currentValueFontSize * 5 + style.majorFontSize * 2
    width: style.currentValueFontSize * 5 + style.majorFontSize * 2
    height: style.currentValueFontSize * 5 + style.majorFontSize * 2
    property var value
    //named ticks, sectors and target value
    //{name, value}
    property var namedTicks: []
    //{name, from, to, color}
    property var sectors:    []
    property var targetValue
    property var header: ""
    property var footer: ""

    property real   tickSpacing:        style.majorFontSize
    property real   tickStep:           1
    property real   majorEvery:         5
    property real   currentValuePos: 0.5 //maps to 0-1 for currentValuePosition on the ladder
    property real   from:     -Infinity
    property real   to:       Infinity
    property real   wrap:     0 //if set , it will wrap value instead
    property bool   mirrored: false
    property int    valueDecimals: 0

    property real predictInterval: 10
    property bool showTrend: true

    property bool repaintOnValueChange: true
    property int orientation: Qt.Vertical

    onShowTrendChanged: historyHandler.history = []

    ValueHistory {
        id: historyHandler
        samples: 10
        value: showTrend ? _root.value : 0
    }




    //drawers
    property var drawer: standardDrawer
    property var ticksDrawer: standardTickDrawer
    property var valueDrawer: standardValueDrawer

    property var _repaintKeys: [width, height, header, footer, namedTicks, sectors, orientation, valueDecimals, wrap, from, to, tickStep, drawer, ticksDrawer]
    onValueChanged: {
        if (repaintOnValueChange) {
            requestPaint()
        }
    }
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
        _setFontSize(ctx, style.currentValueFontSize)
        ctx.textAlign = "center"
        ctx.textBaseline = "middle"
        let textWidth = ctx.measureText(value.toFixed(1)).width + style.fontPadding
        let currPos = _normToCoord(Qt.vector2d(start.x + totalLenth.x * currentValuePos, start.y + totalLenth.y * currentValuePos))

        ctx.beginPath()
        ctx.lineTo(currPos.x + normalizedDir.x * textWidth / 2 + normal.x * style.minorTickLength, currPos.y + normalizedDir.y * style.currentValueFontSize / 2 + normal.y * style.minorTickLength)
        ctx.lineTo(currPos.x + normalizedDir.x * textWidth / 2 + normal.x * (style.minorTickLength + textWidth), currPos.y + normalizedDir.y * style.currentValueFontSize / 2 + normal.y * (style.minorTickLength + style.currentValueFontSize))
        ctx.lineTo(currPos.x - normalizedDir.x * textWidth / 2 + normal.x * (style.minorTickLength + textWidth), currPos.y - normalizedDir.y * style.currentValueFontSize / 2 + normal.y * (style.minorTickLength + style.currentValueFontSize))
        ctx.lineTo(currPos.x - normalizedDir.x * textWidth / 2 + normal.x * style.minorTickLength, currPos.y - normalizedDir.y * style.currentValueFontSize / 2 + normal.y * style.minorTickLength)

        ctx.lineTo(currPos.x - normalizedDir.x * style.currentValueFontSize / 2 + normal.x * style.minorTickLength, currPos.y - normalizedDir.y * style.currentValueFontSize / 2 + normal.y * style.minorTickLength)
        ctx.lineTo(currPos.x, currPos.y)
        ctx.lineTo(currPos.x + normalizedDir.x * style.currentValueFontSize / 2 + normal.x * style.minorTickLength, currPos.y + normalizedDir.y * style.currentValueFontSize / 2 + normal.y * style.minorTickLength)
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
                     currPos.x + normal.x * (style.minorTickLength + textWidth / 2), currPos.y + normal.y * (style.minorTickLength + style.currentValueFontSize / 2) + style.currentValueFontSize / 10) //style.currentValueFontSize / 10 kinda hack, because textBaseline "center" is not actually at the center


        let valueRate = historyHandler.avgRate() * predictInterval
        if(!showTrend || isNaN(valueRate) || historyHandler.history.length < 2 || Math.abs(valueRate) < tickStep) {
            return
        }
        const totalValueRange = (ladderLengthPx / tickSpacing) * tickStep
        const valueAtStart = value - (currentValuePos * totalValueRange)
        const valueAtEnd = valueAtStart + totalValueRange
        const delta = valueRate > 0 ? 1: -1
        const valueOffset = Math.max(0, Math.min(totalValueRange, (value + valueRate * predictInterval) - valueAtStart))
        const pixelOffset = (valueOffset / tickStep) * tickSpacing
        const pointCoord = Qt.vector2d(startPx.x + normalizedDir.x * pixelOffset - normal.x * style.minorTickLength / 2, startPx.y + normalizedDir.y * pixelOffset - normal.y * style.minorTickLength / 2)
        ctx.beginPath()
        ctx.moveTo(currPos.x, currPos.y)
        ctx.lineTo(currPos.x - normal.x * style.minorTickLength / 2, currPos.y - normal.y * style.minorTickLength / 2)
        ctx.lineTo(pointCoord.x,pointCoord.y)
        ctx.lineTo(pointCoord.x - delta * (normalizedDir.x * style.minorTickLength / 4 + normal.x * style.minorTickLength / 4), pointCoord.y - delta * (normalizedDir.y * style.minorTickLength / 4 + normal.y * style.minorTickLength / 4))
        ctx.lineTo(pointCoord.x - delta * (normalizedDir.x * style.minorTickLength / 4 - normal.x * style.minorTickLength / 4), pointCoord.y - delta * (normalizedDir.y * style.minorTickLength / 4 - normal.y * style.minorTickLength / 4))
        ctx.lineTo(pointCoord.x,pointCoord.y)
        ctx.lineTo(pointCoord.x + normal.x * style.minorTickLength / 2, pointCoord.y + normal.y * style.minorTickLength / 2)
        ctx.stroke()
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
        _setFontSize(ctx,style.defaultFontSize)

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
            let currentTickLength = isMajor ? style.majorTickLength : style.minorTickLength
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
                               endCoord.x - startCoord.x - normal.x * style.minorTickLength,
                               endCoord.y - startCoord.y - normal.y * style.minorTickLength)
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

            let tickEndX = tickBaseX - normal.x * style.minorTickLength
            let tickEndY = tickBaseY - normal.y * style.minorTickLength

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

        ctx.moveTo(targetCoord.x + normalizedDir.x * style.currentValueFontSize + normal.x * style.minorTickLength, targetCoord.y + normalizedDir.y * style.currentValueFontSize + normal.y * style.minorTickLength)
        ctx.lineTo(targetCoord.x + normalizedDir.x * style.currentValueFontSize - normal.x * style.minorTickLength, targetCoord.y + normalizedDir.y * style.currentValueFontSize - normal.y * style.minorTickLength)
        ctx.lineTo(targetCoord.x - normalizedDir.x * style.currentValueFontSize - normal.x * style.minorTickLength, targetCoord.y - normalizedDir.y * style.currentValueFontSize - normal.y * style.minorTickLength)
        ctx.lineTo(targetCoord.x - normalizedDir.x * style.currentValueFontSize + normal.x * style.minorTickLength, targetCoord.y - normalizedDir.y * style.currentValueFontSize + normal.y * style.minorTickLength)
        ctx.lineTo(targetCoord.x - normalizedDir.x * style.currentValueFontSize / 2 + normal.x * style.minorTickLength, targetCoord.y - normalizedDir.y * style.currentValueFontSize / 2 + normal.y * style.minorTickLength)
        ctx.lineTo(targetCoord.x, targetCoord.y)
        ctx.lineTo(targetCoord.x + normalizedDir.x * style.currentValueFontSize / 2 + normal.x * style.minorTickLength, targetCoord.y + normalizedDir.y * style.currentValueFontSize / 2 + normal.y * style.minorTickLength)


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
        let pW = (lw + style.majorFontSize * 3) / width
        let pH = (lw + style.majorFontSize * 2) / height
        let ladderStart = Qt.vector2d(0, 0)
        let ladderEnd   = Qt.vector2d(0, 0)

        _setFontSize(ctx, style.titleFontSize)
        let headerLength = header == "" ? 0 : ((ctx.measureText(header).width) / width)
        let footerLength = footer == "" ? 0 : ((ctx.measureText(footer).width) / width)

        if (orientation === Qt.Horizontal) {
            ladderStart.x = lw / width
            ladderStart.y = pH
            ladderEnd.x = 1 - ladderStart.x          // left -> right increasing
            if (mirrored) ladderStart.y = 1 - ladderStart.y

            ladderEnd.y = ladderStart.y
            ladderStart.x += headerLength
            ladderEnd.x -= footerLength
        } else {
            ladderStart.x = pW
            ladderStart.y = 1 - lw / width
            ladderEnd.y = 1 - ladderStart.y          // bottom -> top increasing
            if (footer !== "") {
                ladderStart.y -= style.titleFontSize / height
            }
            if (header !== "") {
                ladderEnd.y += style.titleFontSize / height
            }


            if (!mirrored) ladderStart.x = 1 - ladderStart.x
            ladderEnd.x = ladderStart.x
        }


        ticksDrawer(ctx, ladderStart, ladderEnd)
        valueDrawer(ctx, ladderStart, ladderEnd)


        //draw header and footer
        _setFontSize(ctx, style.titleFontSize)
        let headerAlign
        let footerAlign
        let headerBaseline
        let footerBaseline
        let headerPos
        let footerPos

        if(orientation === Qt.Horizontal) {
            headerAlign = "left"
            footerAlign = "right"
            headerBaseline = "middle"
            footerBaseline = "middle"
            headerPos = Qt.vector2d(0, ladderStart.y * height)
            footerPos = Qt.vector2d(width, ladderEnd.y * height)
        } else {
            headerAlign = "center"
            footerAlign = "center"
            headerBaseline = "bottom"
            footerBaseline = "top"
            headerPos = _normToCoord(ladderEnd)
            footerPos = _normToCoord(ladderStart)
        }

        ctx.textAlign = headerAlign
        ctx.textBaseline = headerBaseline
        ctx.fillText(header,headerPos.x, headerPos.y)
        ctx.textAlign = footerAlign
        ctx.textBaseline = footerBaseline
        ctx.fillText(footer,footerPos.x, footerPos.y)
    }

    onPaint: {
        if (!visible) {
            return
        }

        let ctx = begin()
        drawer(ctx)
    }

}
