pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

Item {

    id: _root
    property var _activeVehicle:  globals.activeVehicle
    property var _cameraManager:  _activeVehicle.cameraManager
    property var _camera:         _cameraManager.currentCameraInstance
    property var _streamInfo:     _camera.currentStreamInstance
    property var activeGroup:     _activeVehicle ? _activeVehicle.vehicle : null
    property real fontSize:       ScreenTools.largeFontPointSize * 1.5
    property real smallFontScale: 0.7       // tick labels / captions relative to fontSize
    property real majorLineWidth: 3
    property real minorLineWidth: majorLineWidth * smallFontScale
    property real textPadding:    ScreenTools.defaultFontPixelWidth
    property color lineColor:     Qt.rgba(100 / 255, 1,0,1)
    property color fontColor:     Qt.rgba(100 / 255, 1,0,1)
    property color boxColor:      Qt.rgba(0, 0, 0, 0.45)
    // Drop shadow, rendered once on the GPU for the whole OSD
    property bool  shadowEnabled: false
    property color shadowColor:   Qt.rgba(0, 0, 0, 0.9)
    property real  shadowBlur:    4     // px
    property real  shadowOffsetX: 2
    property real  shadowOffsetY: 2

    // Scale configuration
    property real pitchRangeDeg:  22  // degrees of pitch from screen centre to top edge (hardcoded for now, need to be taken from the camera fov)
    property real speedRange:     20    // visible span of the speed tape (display units)
    property real speedMinorStep: 1
    property real speedMajorStep: 5
    property real altRange:       50    // visible span of the altitude tape (display units)
    property real altMinorStep:   5
    property real altMajorStep:   10

    // Telemetry (roll/pitch/heading in degrees, speeds/altitude in user display units)
    readonly property real   rollDeg:     _num(activeGroup ? activeGroup.roll.rawValue          : NaN)
    readonly property real   pitchDeg:    _num(activeGroup ? activeGroup.pitch.rawValue         : NaN)
    readonly property real   headingDeg:  _num(activeGroup ? activeGroup.heading.rawValue       : NaN)
    readonly property real   groundSpeed: _num(activeGroup ? activeGroup.groundSpeed.value      : NaN)
    readonly property real   airSpeed:    _num(activeGroup ? activeGroup.airSpeed.value         : NaN)
    readonly property real   altRelative: _num(activeGroup ? activeGroup.altitudeRelative.value : NaN)
    readonly property string speedUnits:  activeGroup ? activeGroup.groundSpeed.units      : ""
    readonly property string altUnits:    activeGroup ? activeGroup.altitudeRelative.units : ""

    // Canvas fonts are set in px; convert from points the same way QML Text does
    readonly property int _largePx: Math.max(1, Math.round(fontSize * Screen.logicalPixelDensity * 25.4 / 72))
    readonly property int _smallPx: Math.max(1, Math.round(_largePx * smallFontScale))

    // ---------------------------------------------------------------- layout (shared by all layers)
    readonly property real _cx:         width / 2
    readonly property real _cy:         height / 2
    readonly property real _pad:        textPadding
    readonly property real _margin:     textPadding * 3
    readonly property real _smallH:     _smallPx * 1.2
    readonly property real _boxH:       _largePx + 2 * _pad
    readonly property real _arrow:      _boxH * 0.3
    readonly property real _tickMajor:  _smallH * 0.6
    readonly property real _tickMinor:  _smallH * 0.3
    readonly property real _index:      _tickMajor * 0.8     // size of the triangular indices
    readonly property real _boxW:       _boxMetrics.advanceWidth + 2 * _pad

    readonly property real _tapeH:      height * 0.5
    readonly property real _tapeTop:    _cy - _tapeH / 2
    readonly property real _tapeBottom: _cy + _tapeH / 2
    readonly property real _speedX:     Math.max(_margin + _boxW + _arrow, width * 0.15)
    readonly property real _altX:       Math.min(width - _margin - _boxW - _arrow, width * 0.82)

    readonly property real _ppd:        (height / 2) / pitchRangeDeg
    readonly property real _rollR:      height * 0.12
    readonly property real _rollCy:     _margin + _index + _tickMajor + _rollR
    readonly property real _roseR:      Math.min(height * 0.1, width * 0.1)
    readonly property real _roseCy:     height - _margin - _smallH
    readonly property real _roseTop:    _roseCy - _roseR - _tickMajor - _index

    readonly property real _clipLeft:   _speedX + _arrow + _boxW + _pad
    readonly property real _clipRight:  _altX - _pad
    readonly property real _clipTop:    0
    readonly property real _clipBottom: _roseTop - _pad

    // Status bar: driven by a HorizontalFactValueGrid, columns split around the roll scale
    property var  statusGrid:          null

    readonly property int  _statusCols:     statusGrid.columns ? statusGrid.columns.count : 0
    readonly property int  _statusLeftCols: Math.ceil(_statusCols / 2)
    readonly property int  _statusPerSide:  Math.max(1, _statusLeftCols)
    readonly property real _statusGap:      _rollR + _tickMajor + _pad * 2     // half-width kept free for the roll scale
    readonly property real _statusSlotW:    Math.max(0, (_cx - _statusGap - _margin) / _statusPerSide)



    TextMetrics {
        id:             _boxMetrics
        font.pixelSize: _root._largePx
        font.bold:      true
        font.family:    "sans-serif"
        text:           "8.8"
    }

    // ---------------------------------------------------------------- repaint triggers
    onRollDegChanged:     pitchLadder.requestPaint()
    onPitchDegChanged:    pitchLadder.requestPaint()
    onHeadingDegChanged:  pitchLadder.requestPaint()     // heading ticks on the horizon
    onGroundSpeedChanged: speedTape.requestPaint()
    onAirSpeedChanged:    speedTape.requestPaint()
    onAltRelativeChanged: altTape.requestPaint()

    onActiveGroupChanged:    refreshAll()
    on_LargePxChanged:       refreshAll()
    onSpeedUnitsChanged:     refreshAll()
    onAltUnitsChanged:       refreshAll()
    onLineColorChanged:      refreshAll()
    onFontColorChanged:      refreshAll()
    onMajorLineWidthChanged: refreshAll()
    onPitchRangeDegChanged:  refreshAll()

    function refreshAll() {
        staticLayer.requestPaint()
        rollPointer.requestPaint()
        roseCard.requestPaint()
        pitchLadder.requestPaint()
        speedTape.requestPaint()
        altTape.requestPaint()
    }

    // Canvas that repaints itself when its geometry changes
    component LayerCanvas: Canvas {
        onXChanged:      requestPaint()
        onYChanged:      requestPaint()
        onWidthChanged:  requestPaint()
        onHeightChanged: requestPaint()
    }

    Item {
        id:           _content
        anchors.fill: parent
        visible:      !!_root.activeGroup

        layer.enabled: _root.shadowEnabled
        layer.effect: MultiEffect {
            shadowEnabled:          true
            shadowColor:            _root.shadowColor
            blurMax:                Math.max(1, Math.round(_root.shadowBlur))
            shadowBlur:             1.0
            shadowHorizontalOffset: _root.shadowOffsetX
            shadowVerticalOffset:   _root.shadowOffsetY
        }

        // Everything that never moves: painted only on resize / style changes
        LayerCanvas {
            id:           staticLayer
            anchors.fill: parent
            onPaint: {
                var ctx = _root._begin(this, true)
                _root._drawRollTicks(ctx)
                _root._drawRoseLubber(ctx)
                _root._drawAircraftSymbol(ctx)
                _root._drawTapeAxis(ctx, _root._speedX)
                _root._drawTapeAxis(ctx, _root._altX)
                _root._drawTapeCaptions(ctx)
            }
        }

        // Pitch ladder + horizon: the only full-width layer that repaints
        LayerCanvas {
            id:     pitchLadder
            x:      _root._clipLeft
            y:      _root._clipTop
            width:  Math.max(0, _root._clipRight - _root._clipLeft)
            height: Math.max(0, _root._clipBottom - _root._clipTop)
            onPaint: {
                var ctx = _root._begin(this, true)
                _root._drawPitchLadder(ctx)
            }
        }

        // One grid value in OSD style: label (or icon) above the value
        component OsdValueCell: Column {
            id: cell

            property var instrumentValueData

            readonly property var   _fact:  instrumentValueData ? instrumentValueData.fact : null
            readonly property color _color: instrumentValueData && instrumentValueData.isValidColor(instrumentValueData.currentColor)
                                            ? instrumentValueData.currentColor : _root.fontColor

            opacity: instrumentValueData ? instrumentValueData.currentOpacity : 1

            Text {
                width:               parent.width
                visible:             !!(cell.instrumentValueData && cell.instrumentValueData.text)
                horizontalAlignment: Text.AlignHCenter
                elide:               Text.ElideRight
                text:                cell.instrumentValueData ? cell.instrumentValueData.text : ""
                color:               cell._color
                font.pixelSize:      _root._smallPx
                font.bold:           true
                font.family:         "sans-serif"
            }

            QGCColoredImage {
                anchors.horizontalCenter: parent.horizontalCenter
                visible:          !!(cell.instrumentValueData && !cell.instrumentValueData.text && cell.instrumentValueData.icon)
                source:           visible ? "/InstrumentValueIcons/" + cell.instrumentValueData.currentIcon : ""
                height:           _root._smallPx * 1.2
                width:            height
                sourceSize.height: height
                fillMode:         Image.PreserveAspectFit
                mipmap:           true
                color:            cell._color
            }

            Text {
                width:               parent.width
                horizontalAlignment: Text.AlignHCenter
                elide:               Text.ElideRight
                text:                cell._fact
                                     ? cell._fact.enumOrValueString + (cell.instrumentValueData.showUnits && cell._fact.units ? " " + cell._fact.units : "")
                                     : "--.--"
                color:               cell._color
                font.pixelSize:      _root._smallPx
                font.bold:           true
                font.family:         "sans-serif"
            }
        }

        Repeater {
            model: statusGrid.columns

            delegate: Item {
                id: gridColumn

                required property var object        // this column's list of InstrumentValueData
                required property int index

                readonly property bool onLeft: index < _root._statusLeftCols

                width:  _root._statusSlotW - _root._pad
                height: cells.height
                x:      onLeft ? _root._cx - _root._statusGap - (_root._statusLeftCols - index) * _root._statusSlotW
                               : _root._cx + _root._statusGap + (index - _root._statusLeftCols) * _root._statusSlotW + _root._pad
                y:      _root._margin

                Column {
                    id:      cells
                    width:   parent.width
                    spacing: _root._pad

                    Repeater {
                        model: gridColumn.object
                        delegate: OsdValueCell {
                            required property var object
                            width:               cells.width
                            instrumentValueData: object
                        }
                    }
                }
            }
        }

        // Sky pointer: painted once, rotated on the GPU
        LayerCanvas {
            id:       rollPointer
            x:        _root._cx - _root._rollR
            y:        _root._rollCy - _root._rollR
            width:    _root._rollR * 2
            height:   _root._rollR * 2
            visible:  !isNaN(_root.rollDeg)
            rotation: isNaN(_root.rollDeg) ? 0 : -_root.rollDeg
            onPaint: {
                var ctx = _root._begin(this, false)
                var c   = width / 2
                var r   = _root._rollR
                var ts  = _root._index
                ctx.lineWidth = _root.majorLineWidth
                ctx.beginPath()
                ctx.moveTo(c, c - r + 1)
                ctx.lineTo(c - ts * 0.6, c - r + ts)
                ctx.lineTo(c + ts * 0.6, c - r + ts)
                ctx.closePath()
                ctx.stroke()
            }
        }

        Text {
            x:              _root._cx - width / 2
            y:              _root._rollCy - _root._rollR + _root._index + _root._pad
            text:           "H " + (isNaN(_root.headingDeg) ? "---" : _root._heading3(_root.headingDeg))
            color:          _root.fontColor
            font.pixelSize: _root._smallPx
            font.bold:      true
            font.family:    "sans-serif"
        }

        // Compass rose: card painted once for heading 0 and rotated on the GPU.
        // The container clips it to roughly +-100 degrees around the lubber line.
        Item {
            readonly property real cardR: _root._roseR + _root._tickMajor + _root._pad

            x:      _root._cx - cardR
            y:      _root._roseCy - cardR
            width:  cardR * 2
            height: cardR + (_root._roseR + _root._tickMajor) * 0.18
            clip:   true

            LayerCanvas {
                id:       roseCard
                width:    parent.cardR * 2
                height:   parent.cardR * 2
                visible:  !isNaN(_root.headingDeg)
                rotation: isNaN(_root.headingDeg) ? 0 : -_root.headingDeg
                onPaint: {
                    var ctx = _root._begin(this, false)
                    _root._drawRoseCard(ctx, width / 2)
                }
            }
        }

        LayerCanvas {
            id:     speedTape
            x:      0
            y:      _root._tapeTop - _root._smallH
            width:  _root._clipLeft + _root._boxW      // room for the GS box to grow
            height: _root._tapeH + _root._smallH * 2
            onPaint: {
                var ctx = _root._begin(this, true)
                _root._drawTapeScale(ctx, _root._speedX, -1, true, _root.groundSpeed,
                                     _root.speedRange, _root.speedMinorStep, _root.speedMajorStep, 0)
                _root._drawTapeScale(ctx, _root._speedX,  1, true, _root.airSpeed,
                                     _root.speedRange, _root.speedMinorStep, _root.speedMajorStep, 0)
                _root._drawValueBoxH(ctx, _root._speedX, _root._cy,  1,
                                     isNaN(_root.groundSpeed) ? "---" : _root.groundSpeed.toFixed(1))
                _root._drawValueBoxH(ctx, _root._speedX, _root._cy, -1,
                                     isNaN(_root.airSpeed) ? "---" : _root.airSpeed.toFixed(1))
            }
        }

        LayerCanvas {
            id:     altTape
            x:      _root._altX - _root._pad * 2
            y:      _root._tapeTop - _root._smallH
            width:  Math.max(0, _root.width - x)
            height: _root._tapeH + _root._smallH * 2
            onPaint: {
                var ctx = _root._begin(this, true)
                _root._drawTapeScale(ctx, _root._altX, 1, true, _root.altRelative,
                                     _root.altRange, _root.altMinorStep, _root.altMajorStep, -Infinity)
                _root._drawValueBoxH(ctx, _root._altX, _root._cy, -1,
                                     isNaN(_root.altRelative) ? "---" : _root.altRelative.toFixed(1))
            }
        }
    }

    // ---------------------------------------------------------------- helpers

    function _num(v) {
        return (v === undefined || v === null || v === "") ? NaN : Number(v)
    }

    // Clears the canvas and sets the common style.
    // toRoot: shift the origin so drawing code can use _root coordinates.
    function _begin(canvas, toRoot) {
        var ctx = canvas.getContext("2d")
        ctx.reset()
        ctx.clearRect(0, 0, canvas.width, canvas.height)
        ctx.lineCap     = "round"
        ctx.lineJoin    = "round"
        ctx.strokeStyle = lineColor
        ctx.fillStyle   = fontColor
        if (toRoot)
            ctx.translate(-canvas.x, -canvas.y)
        return ctx
    }

    function _setFont(ctx, large) {
        ctx.font      = "bold " + (large ? _largePx : _smallPx) + "px sans-serif"
        ctx.fillStyle = fontColor
    }

    function _fmtTick(v) {
        return Number(v.toFixed(2)).toString()
    }

    function _norm360(d) {
        return ((d % 360) + 360) % 360
    }

    function _heading3(d) {
        return ("00" + (Math.round(_norm360(d)) % 360)).slice(-3)
    }

    // Box with an arrow pointing horizontally at (tipX, tipY); dir = +1 points right, -1 points left
    function _drawValueBoxH(ctx, tipX, tipY, dir, text) {
        _setFont(ctx, true)
        var bw    = Math.max(_boxW, ctx.measureText(text).width + 2 * _pad)
        var bh    = _boxH
        var nearX = tipX - dir * _arrow
        var farX  = nearX - dir * bw

        ctx.beginPath()
        ctx.moveTo(tipX, tipY)
        ctx.lineTo(nearX, tipY - bh / 2)
        ctx.lineTo(farX,  tipY - bh / 2)
        ctx.lineTo(farX,  tipY + bh / 2)
        ctx.lineTo(nearX, tipY + bh / 2)
        ctx.closePath()
        ctx.fillStyle = boxColor
        ctx.fill()
        ctx.lineWidth = majorLineWidth
        ctx.stroke()

        _setFont(ctx, true)
        ctx.textAlign    = "center"
        ctx.textBaseline = "middle"
        ctx.fillText(text, (nearX + farX) / 2, tipY)
    }

    // ---------------------------------------------------------------- static parts

    function _drawRollTicks(ctx) {
        var r     = _rollR
        var cx    = _cx
        var cy    = _rollCy
        var ts    = _index
        var toRad = Math.PI / 180
        var marks = [-45, -30, -20, -10, 10, 20, 30, 45]

        for (var pass = 0; pass < 2; pass++) {
            var major = (pass === 0)
            ctx.lineWidth = major ? majorLineWidth : minorLineWidth
            ctx.beginPath()
            for (var i = 0; i < marks.length; i++) {
                var m = marks[i]
                if ((Math.abs(m) === 30) !== major)
                    continue
                var len = major ? _tickMajor : _tickMinor * 1.5
                var s   = Math.sin(m * toRad)
                var c   = Math.cos(m * toRad)
                ctx.moveTo(cx + r * s,         cy - r * c)
                ctx.lineTo(cx + (r + len) * s, cy - (r + len) * c)
            }
            ctx.stroke()
        }

        // Fixed zero index
        ctx.lineWidth = majorLineWidth
        ctx.beginPath()
        ctx.moveTo(cx, cy - r)
        ctx.lineTo(cx - ts * 0.6, cy - r - ts)
        ctx.lineTo(cx + ts * 0.6, cy - r - ts)
        ctx.closePath()
        ctx.stroke()
    }

    function _drawRoseLubber(ctx) {
        var tipY = _roseCy - _roseR - _tickMajor
        var ts   = _index
        ctx.lineWidth = majorLineWidth
        ctx.beginPath()
        ctx.moveTo(_cx, tipY)
        ctx.lineTo(_cx - ts * 0.6, tipY - ts)
        ctx.lineTo(_cx + ts * 0.6, tipY - ts)
        ctx.closePath()
        ctx.stroke()
    }

    function _drawAircraftSymbol(ctx) {
        var cx = _cx
        var cy = _cy
        var w  = width * 0.04

        ctx.lineWidth = majorLineWidth
        ctx.beginPath()
        ctx.moveTo(cx - w,       cy)
        ctx.lineTo(cx - w * 0.2, cy)
        ctx.lineTo(cx,           cy + w * 0.2)
        ctx.lineTo(cx + w * 0.2, cy)
        ctx.lineTo(cx + w,       cy)
        ctx.stroke()
    }

    function _drawTapeAxis(ctx, axisX) {
        ctx.lineWidth = majorLineWidth
        ctx.beginPath()
        ctx.moveTo(axisX, _tapeTop)
        ctx.lineTo(axisX, _tapeBottom)
        ctx.stroke()
    }

    function _drawTapeCaptions(ctx) {
        _setFont(ctx, false)
        ctx.textBaseline = "bottom"
        ctx.textAlign    = "right"
        ctx.fillText("GS", _speedX - _pad, _tapeTop - _pad)
        ctx.textAlign    = "left"
        ctx.fillText("AS", _speedX + _pad, _tapeTop - _pad)
        ctx.fillText("ALT " + altUnits, _altX, _tapeTop - _pad)

        ctx.textAlign    = "center"
        ctx.textBaseline = "top"
        ctx.fillText(speedUnits, _speedX, _tapeBottom + _pad)
    }

    // Full 360 degree card for heading 0, centred at (c, c) in the card canvas
    function _drawRoseCard(ctx, c) {
        var r     = _roseR
        var toRad = Math.PI / 180
        var cardinals = { 0: "N", 90: "E", 180: "S", 270: "W" }

        for (var pass = 0; pass < 2; pass++) {
            var major = (pass === 0)
            var len   = major ? _tickMajor : _tickMinor
            ctx.lineWidth = major ? majorLineWidth : minorLineWidth
            ctx.beginPath()
            for (var d = 0; d < 360; d += 5) {
                if ((d % 10 === 0) !== major)
                    continue
                var s = Math.sin(d * toRad)
                var k = Math.cos(d * toRad)
                ctx.moveTo(c + r * s,         c - r * k)
                ctx.lineTo(c + (r + len) * s, c - (r + len) * k)
            }
            ctx.stroke()
        }

        _setFont(ctx, false)
        ctx.textAlign    = "center"
        ctx.textBaseline = "top"
        for (var l = 0; l < 360; l += 30) {
            ctx.save()
            ctx.translate(c, c)
            ctx.rotate(l * toRad)
            ctx.fillText(cardinals[l] !== undefined ? cardinals[l] : (l / 10).toString(), 0, -r + _pad)
            ctx.restore()
        }
    }

    // ---------------------------------------------------------------- dynamic parts

    function _drawPitchLadder(ctx) {
        if (isNaN(rollDeg) || isNaN(pitchDeg))
            return

        var ppd     = _ppd
        var halfW   = width * 0.11
        var gap     = width * 0.055
        var endTick = _smallH * 0.4
        var labels  = []

        ctx.translate(_cx, _cy)
        ctx.rotate(-rollDeg * Math.PI / 180)

        // Horizon line + conformal heading ticks, one stroke
        var yh = pitchDeg * ppd
        ctx.lineWidth = majorLineWidth
        ctx.beginPath()
        ctx.moveTo(-width, yh)
        ctx.lineTo(width, yh)
        if (!isNaN(headingDeg)) {
            var compassHalfSpan = 102 / 4 // HARDCODED, switch to the _streamInfo.hfov and recalcualte the ticks!!!
            var imageWidth = 1920 //HARDCODED, swithc to the _streamInfo.resolution.width !!!!
            var leftBearing = headingDeg - compassHalfSpan
            var rightBearing = headingDeg  + compassHalfSpan
            var first = Math.ceil(leftBearing / 5)
            var last  = Math.floor(rightBearing / 5)
            for (var i = first; i <= last; i++) {
                var d    = i * 5
                var x = imageWidth / 2 + Math.tan((d - headingDeg) * (Math.PI / 180))
                var norm = _norm360(d)
                var ten  = (norm % 10 === 0)
                ctx.moveTo(x, yh)
                ctx.lineTo(x, yh - (ten ? _tickMajor : _tickMinor))
                if (ten)
                    labels.push({ text: norm.toString(), x: x, y: yh - _pad * 2, align: "center", baseline: "bottom" })
            }
        }
        ctx.stroke()

        // Pitch lines in four strokes: (nose up / nose down) x (major / minor)
        for (var g = 0; g < 4; g++) {
            var positive = (g < 2)
            var major    = (g % 2 === 0)
            var w        = major ? halfW : halfW * 0.6
            var t        = positive ? endTick : -endTick      // end ticks point towards the horizon

            ctx.lineWidth = major ? majorLineWidth : minorLineWidth
            if (positive) {
                ctx.setLineDash([])
            } else {
                var dash = (w - gap) / (major ? 14 : 7)
                ctx.setLineDash([dash, dash])
            }

            ctx.beginPath()
            for (var a = 5; a <= 90; a += 5) {
                if ((a % 10 === 0) !== major)
                    continue
                var pa = positive ? a : -a
                var y  = (pitchDeg - pa) * ppd
                if (Math.abs(y) > height)
                    continue
                ctx.moveTo(-w, y + t); ctx.lineTo(-w, y); ctx.lineTo(-gap, y)
                ctx.moveTo(gap, y);    ctx.lineTo(w, y);  ctx.lineTo(w, y + t)
                if (major) {
                    labels.push({ text: pa.toString(), x: -w - _pad, y: y, align: "right", baseline: "middle" })
                    labels.push({ text: pa.toString(), x:  w + _pad, y: y, align: "left",  baseline: "middle" })
                }
            }
            ctx.stroke()
        }
        ctx.setLineDash([])

        _setFont(ctx, false)
        for (var j = 0; j < labels.length; j++) {
            var lb = labels[j]
            ctx.textAlign    = lb.align
            ctx.textBaseline = lb.baseline
            ctx.fillText(lb.text, lb.x, lb.y)
        }
    }

    // Scrolling ticks and labels on one side of a tape axis, centred on value.
    // tickDir = +1: ticks point right, -1: ticks point left.
    // labelsWithTicks: labels sit beyond the tick ends; otherwise on the opposite side of the axis.
    function _drawTapeScale(ctx, axisX, tickDir, labelsWithTicks, value, range, minorStep, majorStep, minValue) {
        if (isNaN(value))
            return

        var ppu      = _tapeH / range
        var perMajor = Math.max(1, Math.round(majorStep / minorStep))
        var first    = Math.ceil((value - range / 2) / minorStep)
        var last     = Math.floor((value + range / 2) / minorStep)
        var labelX   = labelsWithTicks ? axisX + tickDir * (_tickMajor + _pad)
                                       : axisX - tickDir * _pad
        var labels   = []

        for (var pass = 0; pass < 2; pass++) {
            var major = (pass === 0)
            ctx.lineWidth = major ? majorLineWidth : minorLineWidth
            ctx.beginPath()
            for (var i = first; i <= last; i++) {
                if ((i % perMajor === 0) !== major)
                    continue
                var v = i * minorStep
                if (v < minValue)
                    continue
                var y = _cy - (v - value) * ppu
                if(y > _cy - _boxH / 2 && y < _cy + _boxH / 2) {
                    continue
                }

                ctx.moveTo(axisX, y)
                ctx.lineTo(axisX + tickDir * (major ? _tickMajor : _tickMinor), y)
                if (major)
                    labels.push({ text: _fmtTick(v), y: y })
            }
            ctx.stroke()
        }

        _setFont(ctx, false)
        ctx.textAlign    = (labelsWithTicks === (tickDir > 0)) ? "left" : "right"
        ctx.textBaseline = "middle"
        for (var j = 0; j < labels.length; j++)
            ctx.fillText(labels[j].text, labelX, labels[j].y)
    }
}