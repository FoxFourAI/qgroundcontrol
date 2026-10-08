import QtQuick

import QGroundControl.Controls
import QGroundControl
import FoxFour.Widgets 1.0

Item{
    id: _root
    property var playerSize: Qt.size()
    property OSDStyle style: OSDStyle{}
    property real roll: 0
    property real pitch: 0
    property real heading: 0
    property real spacing: style.majorFontSize * 5

    property var _videoManager:     QGroundControl.videoManager
    property real _baseHorFov: (_videoManager && _videoManager.hfov > 5 ? _videoManager.hfov : 103)
    property real _halfHorFov: (_baseHorFov / 2) * Math.PI / 180
    property real _fHorDisp: (playerSize.width / 2) / Math.tan(_halfHorFov)
    property real _fVertDisp: _fHorDisp * (playerSize.width / playerSize.height)

    clip:true
    OSDLadder {
        style.majorTickLength: _root.spacing
        style.minorTickLength: style.majorTickLength * 0.8
        anchors.centerIn: parent
        width: playerSize.width
        height: playerSize.height
        from: -90
        to: 90
        mirrored: true
        _repaintKeys: [_root.pitch, value, _root.roll, width, height]
        value: _root.heading

        drawer: function(ctx) {
            //rotating ny roll
            const cx = width / 2
            const cy = height / 2

            ctx.translate(cx, cy)
            ctx.rotate((360 - _root.roll) * Math.PI / 180)
            ctx.translate(-cx, -cy)

            //applying style to draw the pitch
            style.majorTickLength = width / 20
            style.minorTickLength = style.majorTickLength * 0.8
            from = -90
            to = 90
            majorEvery = 10
            wrap = 0
            tickStep = 5

            let ladderStart = Qt.vector2d(0.5, 1)
            let ladderEnd = Qt.vector2d(0.5, 0)
            drawPitch(ctx,ladderStart,ladderEnd)

            //applying styles to draw heading
            style.majorTickLength = _root.style.majorTickLength
            style.minorTickLength = _root.style.minorTickLength
            majorEvery = 90
            wrap = 360
            from = -Infinity
            to = Infinity
            tickStep = 10

            ctx.setLineDash([])
            ladderStart = Qt.vector2d(0, 0.5)
            ladderEnd = Qt.vector2d(1, 0.5)
            drawHeading(ctx,ladderStart, ladderEnd)
        }

        function drawHeading(ctx, start, end) {
            if (tickStep <= 0 || tickSpacing <= 0 || value === undefined)
                return

            // translate noramal coords to pixels
            const startPx = _normToCoord(start)
            const endPx = _normToCoord(end)
            let centerPx = startPx.plus(endPx).times(0.5)

            const direction = endPx.minus(startPx)
            const ladderLengthPx = direction.length()
            const normalizedDir = direction.normalized()

            //noraml vector to draw ticks
            let normal = Qt.vector2d(-normalizedDir.y, normalizedDir.x)

            if (mirrored) {
                normal = normal.times(-1)
            }


            // drawing base line
            ctx.beginPath()
            ctx.moveTo(startPx.x, startPx.y)
            ctx.lineTo(endPx.x, endPx.y)
            ctx.stroke()
            _setFontSize(ctx,style.defaultFontSize)

            //calculate value range
            const halfFov = _baseHorFov / 2

            // Absolute bearing range visible by the camera
            const visibleMin = value - halfFov
            const visibleMax = value + halfFov

            // Find the first tick aligned to tickStep
            const firstTickIndex = Math.floor(visibleMin / tickStep)
            const lastTickIndex = Math.ceil(visibleMax / tickStep)

            ctx.textAlign = "center"
            ctx.textBaseline = "bottom"
            ctx.beginPath()
            ctx.lineWidth = style.minorLineWidth

            for (let i = firstTickIndex; i <= lastTickIndex; i++ ) {
                let val = i * tickStep

                const rel = _wrapValue(val -  value)

                const tickBaseX = centerPx.x + _fHorDisp * Math.tan(rel * Math.PI / 180)
                const tickBaseY = centerPx.y

                const currentTickLength = style.minorTickLength
                const tickEndX = tickBaseX + normal.x * currentTickLength
                const tickEndY = tickBaseY + normal.y * currentTickLength

                const isMajor = (val % majorEvery === 0)
                ctx.moveTo(tickBaseX, tickBaseY)
                ctx.lineTo(tickEndX, tickEndY)

                if (isMajor) {
                    let displayValue = _wrapValue(val).toFixed(0)
                    ctx.fillText(displayValue, tickEndX, tickEndY)
                }
            }
            ctx.stroke()
        }

        function drawPitch(ctx, start, end) {
            if (tickStep <= 0 || tickSpacing <= 0 || _root.pitch === undefined)
                return

            // translate noramal coords to pixels
            const startPx = _normToCoord(start)
            const endPx = _normToCoord(end)
            const centerPx = startPx.plus(endPx).times(0.5)

            const direction = endPx.minus(startPx)
            const ladderLengthPx = direction.length()
            const normalizedDir = direction.normalized()

            //noraml vector to draw ticks
            let normal = Qt.vector2d(-normalizedDir.y, normalizedDir.x)
            let spaceOffset = normal.times(_root.spacing / 2)
            let textOffset = normal.times(style.fontPadding)

            //calculate value range
            const halfFov = _baseHorFov / 2

            // Absolute bearing range visible by the camera
            const visibleMin = _root.pitch - halfFov
            const visibleMax = _root.pitch + halfFov

            // Find the first tick aligned to tickStep
            const firstTickIndex = Math.floor(visibleMin / tickStep)
            const lastTickIndex = Math.ceil(visibleMax / tickStep)
            ctx.textBaseLine = "middle"
            console.log("first:", firstTickIndex, "last:", lastTickIndex, "")
            for (let i = firstTickIndex; i <= lastTickIndex; i++ ) {
                const val = i * tickStep
                if (val < from || val > to)
                    continue

                const rel = val - _root.pitch
                const tickOffset = Qt.vector2d(0, -_fVertDisp * Math.tan(rel * Math.PI / 180))

                const isMajor = (val % majorEvery === 0)
                const currentTickLength = isMajor ? style.majorTickLength : style.minorTickLength
                const tickLengthOffset = spaceOffset.plus(normal.times(currentTickLength))
                if (val == 0) {
                    continue
                }

                const tip = normalizedDir.times(
                              val > 0
                              ? -style.defaultFontSize / 2
                              : style.defaultFontSize / 2
                              )

                if (val > 0) {
                    ctx.setLineDash([])
                } else {
                    ctx.setLineDash([_root.style.majorLineWidth])
                }

                ctx.beginPath()
                ctx.lineWidth = isMajor ? style.majorLineWidth : style.minorLineWidth
                ctx.moveTo(centerPx.x + tickOffset.x + spaceOffset.x,               centerPx.y + tickOffset.y + spaceOffset.y)
                ctx.lineTo(centerPx.x + tickOffset.x + tickLengthOffset.x,          centerPx.y + tickOffset.y + tickLengthOffset.y)
                ctx.lineTo(centerPx.x + tickOffset.x + tickLengthOffset.x + tip.x,  centerPx.y + tickOffset.y + tickLengthOffset.y + tip.y)
                ctx.moveTo(centerPx.x + tickOffset.x - spaceOffset.x,               centerPx.y + tickOffset.y - spaceOffset.y)
                ctx.lineTo(centerPx.x + tickOffset.x - tickLengthOffset.x,          centerPx.y + tickOffset.y - tickLengthOffset.y)
                ctx.lineTo(centerPx.x + tickOffset.x - tickLengthOffset.x + tip.x,  centerPx.y + tickOffset.y - tickLengthOffset.y + tip.y)
                ctx.stroke()

                if (isMajor) {
                    let displayValue = _wrapValue(val).toFixed(valueDecimals)
                    ctx.textAlign= "right"
                    ctx.fillText(displayValue, centerPx.x + tickOffset.x - tickLengthOffset.x - textOffset.x + tip.x / 2, centerPx.y + tickOffset.y - tickLengthOffset.y - textOffset.y + tip.y / 2)
                    ctx.textAlign= "left"
                    ctx.fillText(displayValue, centerPx.x + tickOffset.x + tickLengthOffset.x + textOffset.x + tip.x / 2, centerPx.y + tickOffset.y + tickLengthOffset.y + textOffset.y + tip.y / 2)

                }
            }
        }
    }

}
