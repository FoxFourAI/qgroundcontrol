import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root
    property var vehicle: globals.activeVehicle

    property real fontSize:       ScreenTools.largeFontPointSize * 2
    property real majorLineWidth: 2
    property real minorLineWidth: 1
    property real textPadding : 5
    property color lineColor:     "lightgreen"
    property color fontColor:     "lightgreen"
    property color currentColor:  Qt.rgba(1,0,0,1)

    property color shadowColor:   Qt.rgba(0, 0, 0, 0.65)
    property real  shadowBlur:    2
    property real  shadowOffsetX: 2
    property real  shadowOffsetY: 2

    function mapRange(value, inMin, inMax, outMin, outMax) {
        return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin)
    }

    // odd widths sit on a pixel centre, even widths on a pixel boundary
    function snap(y, lineWidth) {
        return (lineWidth % 2 === 1) ? Math.round(y) + 0.5 : Math.round(y)
    }

    Canvas {
        id: heightCanvas

        property real altitude:       root.vehicle.vehicle.altitudeRelative.rawValue

        property real range:     30
        property real step:      0.5
        property int  ticksPerMajor:  5 / step // meters/steps
        property int  spaceBetweenTicks: 35
        anchors.left:         parent.left
        anchors.leftMargin:   root.textPadding
        anchors.top:          parent.top
        anchors.topMargin:    ScreenTools.toolbarHeight
        anchors.bottom:       parent.bottom
        width: Math.max(120,Math.min(180,root.width * 0.1))

        onAltitudeChanged: requestPaint()
        onHeightChanged:   {range = height / spaceBetweenTicks ; requestPaint()}
        onWidthChanged:    requestPaint()



        onPaint: {
            var ctx = getContext("2d")
            if (!ctx)
                return

            ctx.reset()
            ctx.clearRect(0, 0, width, height)

            ctx.strokeStyle  = root.lineColor
            ctx.fillStyle    = root.fontColor
            ctx.font         = root.fontSize + "px sans-serif"
            ctx.textBaseline = "middle"

            ctx.shadowColor   = root.shadowColor
            ctx.shadowBlur    = root.shadowBlur
            ctx.shadowOffsetX = root.shadowOffsetX
            ctx.shadowOffsetY = root.shadowOffsetY

            let min   = altitude - range * 0.5
            let max   = altitude + range * 0.5
            let first = Math.ceil(min  / step)
            let last  = Math.floor(max / step)
            let n, value, y

            //pass 1: minor ticks
            ctx.lineWidth = root.minorLineWidth
            ctx.beginPath()
            for (n = first; n <= last; ++n) {
                if (n % ticksPerMajor === 0) {
                    continue
                }
                value = n * step
                y = root.snap(root.mapRange(value, min, max, height, 0), root.minorLineWidth)
                ctx.moveTo(width * 0.6, y)
                ctx.lineTo(width - root.shadowOffsetX, y)
            }
            ctx.stroke()

            //pass 2: major ticks + labels
            ctx.lineWidth = root.majorLineWidth
            ctx.beginPath()
            for (n = first; n <= last; ++n) {
                if (n % ticksPerMajor !== 0)
                    continue
                value = n * step
                y = root.snap(root.mapRange(value, min, max, height, 0), root.majorLineWidth)

                var label = value.toFixed(0)
                ctx.fillText(label, 0, y)
                ctx.moveTo(ctx.measureText(label).width + root.textPadding, y)
                ctx.lineTo(width - root.shadowOffsetX, y)
            }
            ctx.stroke()

            //pass 3: current-altitude indicator
            var markerH = root.fontSize * 1.2
            var pointW  = 10
            var markerW = width - pointW - root.shadowOffsetX * 2
            var cy      = Math.round(height * 0.5)

            ctx.fillStyle = root.shadowColor
            ctx.beginPath()
            ctx.moveTo(0, cy + markerH * 0.5)
            ctx.lineTo(0, cy - markerH * 0.5)
            ctx.lineTo(0 + markerW, cy - markerH * 0.5)
            ctx.lineTo(0 + markerW + pointW, cy)
            ctx.lineTo(0 + markerW, cy + markerH * 0.5)
            ctx.closePath()
            ctx.fill()
            ctx.stroke()
            ctx.fillStyle = root.fontColor
            let percision = 3
            while (ctx.measureText(altitude.toFixed(percision)).width > markerW && percision != 0) {
                percision--;
            }
            ctx.fillText(altitude.toFixed(percision),root.textPadding,cy + 3)
        }
    }

    Canvas {
        id: compass
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: ScreenTools.toolbarHeight + root.textPadding
        property var heading: root.vehicle ? root.vehicle.vehicle.heading.rawValue : "---"
        property var rangeAngle: 20
        property var step: 1
        property var ticksPerMajor: 10
        property var headings: ["N","E","S","W"]


        width: Math.max(200,Math.min(600,root.width * 0.4))
        height: Math.max(root.fontSize,Math.min(root.fontSize * 2 ,root.height * 0.1))

        onWidthChanged: {rangeAngle = width / 20; requestPaint()}
        onHeightChanged: requestPaint()
        onHeadingChanged: requestPaint()

        Component.onCompleted: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            if (!ctx)
                return
            ctx.fillStyle = root.fontColor
            ctx.reset()
            ctx.clearRect(0,0,width,height)
            ctx.strokeStyle  = root.lineColor
            ctx.fillStyle    = root.fontColor
            ctx.font         = root.fontSize + "px sans-serif"
            ctx.textBaseline = "alphabetic"

            ctx.shadowColor   = root.shadowColor
            ctx.shadowBlur    = root.shadowBlur
            ctx.shadowOffsetX = root.shadowOffsetX
            ctx.shadowOffsetY = root.shadowOffsetY

            let min = heading - rangeAngle * 0.5;
            let max = heading + rangeAngle * 0.5;
            let first = Math.ceil(min / step)
            let last = Math.floor(max / step)
            let n, value, x
            //pass 1: draw small ticks
            ctx.lineWidth = root.minorLineWidth
            ctx.beginPath()
            for (n = first; n < last; ++n) {
                if (n % ticksPerMajor === 0) {
                    continue
                }
                value = n * step
                x = root.snap(root.mapRange(value,min,max,0,width), root.minorLineWidth)
                ctx.moveTo(x,0)
                ctx.lineTo(x,height * 0.2)
            }
            ctx.stroke()

            ctx.lineWidth = root.majorLineWidth
            ctx.beginPath()
            for (n = first; n < last; ++n) {
                if (n % ticksPerMajor !== 0) {
                    continue
                }
                value = n * step
                x = root.snap(root.mapRange(value,min,max,0,width), root.minorLineWidth)
                ctx.moveTo(x,0)
                ctx.lineTo(x,height * 0.4)
                let text = n < 0 ? n + 360 : n
                if (n % 90 === 0) {
                    let head = n;
                    if (head >= 360)
                        head -= 360
                    text = headings[head / 90]
                }

                ctx.fillText(text, x - ctx.measureText(text).width / 2, height * 0.8)
            }
            ctx.stroke()

            let markerW = ctx.measureText("000").width + root.textPadding * 2
            let markerH = root.fontSize
            let pointH = 10
            ctx.fillStyle = root.shadowColor
            ctx.beginPath()
            let cx = Math.round(width * 0.5)
            ctx.moveTo(cx + markerW * 0.5,height - root.textPadding)
            ctx.lineTo(cx - markerW * 0.5, height - root.textPadding)
            ctx.lineTo(cx - markerW * 0.5, height - markerH - root.textPadding)
            ctx.lineTo(cx , height - markerH - pointH - root.textPadding)
            ctx.lineTo(cx + markerW * 0.5, height - markerH - root.textPadding)
            ctx.closePath()
            ctx.fill()
            ctx.stroke()
            ctx.fillStyle = root.fontColor
            let textMeasure = ctx.measureText(heading)
            ctx.fillText(heading,cx - textMeasure.width * 0.5, height - root.textPadding * 2)
        }
    }

    Canvas {
        id: center
        anchors.centerIn: parent
        height: 15
        width: 120

        Component.onCompleted: requestPaint()

        onPaint: {
            let ctx = getContext("2d")
            ctx.lineWidth = root.majorLineWidth
            ctx.strokeStyle = root.lineColor
            ctx.fillStyle = root.shadowColor
            ctx.shadowColor   = root.shadowColor
            ctx.shadowBlur    = root.shadowBlur
            ctx.shadowOffsetX = root.shadowOffsetX
            ctx.shadowOffsetY = root.shadowOffsetY

            ctx.beginPath()
            ctx.arc(width * 0.5, height * 0.5, height * 0.5 - root.majorLineWidth, 0, 2 * Math.PI)
            ctx.stroke()

            ctx.beginPath()
            ctx.moveTo(0,height * 0.5)
            ctx.lineTo(width * 0.5 - height * 0.5 - root.majorLineWidth - root.textPadding, height * 0.5)
            ctx.moveTo(width * 0.5 + height * 0.5 + root.majorLineWidth + root.textPadding, height * 0.5)
            ctx.lineTo(width, height * 0.5)
            ctx.stroke()
        }
    }
}