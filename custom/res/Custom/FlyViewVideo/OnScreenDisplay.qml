pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects

import QGroundControl
import QGroundControl.Controls

Item {

    id: _root
    property var _activeVehicle:  globals.activeVehicle
    property var _videoManager:     QGroundControl.videoManager
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
    property bool  shadowEnabled: true
    property color shadowColor:   Qt.rgba(0, 0, 0, 0.7)
    property real  shadowBlur:    4     // px
    property real  shadowOffsetX: 2
    property real  shadowOffsetY: 2
    property real  tickStep: 10

    // Scale configuration
    property real pitchRangeDeg:  22  // degrees of pitch from screen centre to top edge (hardcoded for now, need to be taken from the camera fov)
    property real speedRange:     20    // visible span of the speed tape (display units)
    property real speedMinorStep: 1
    property real speedMajorStep: 5
    property real altRange:       50    // visible span of the altitude tape (display units)
    property real altMinorStep:   5
    property real altMajorStep:   10
    property real zoomFactor:     1     // optical / digital zoom; narrows the FOV used for the pitch scale
    property real horizonFontScale: 0.8 // heading labels on the horizon relative to the small font

    // Trend arrows: where speed / altitude will be after trendSeconds at the current acceleration / climb rate
    property real trendSeconds:   10
    property real accelFilterTau: 1.0   // s, low-pass time constant of the acceleration estimate

    // Flight path marker and ground course marker are hidden below this ground speed (m/s)
    property real fpvMinGroundSpeed: 2

    // Target markers: shown while a speed / altitude controlling mode provides a setpoint
    property bool speedTargetActive: !isNaN(speedTarget)
    property bool altTargetActive:   !isNaN(altTarget)

    // Wind: true when the autopilot reports the direction the wind comes FROM (ArduPilot WIND)
    property bool windDirectionIsFrom: true


    // Telemetry (roll/pitch/heading in degrees, speeds/altitude in user display units)
    readonly property real   rollDeg:        _num(activeGroup ? activeGroup.roll.rawValue                : NaN)
    readonly property real   pitchDeg:       _num(activeGroup ? activeGroup.pitch.rawValue               : NaN)
    readonly property real   headingDeg:     _num(activeGroup ? activeGroup.heading.rawValue             : NaN)
    readonly property real   groundSpeed:    _num(activeGroup ? activeGroup.groundSpeed.value            : NaN)
    readonly property real   airSpeed:       _num(activeGroup ? activeGroup.airSpeed.value               : NaN)
    readonly property real   speedTarget:    _num(activeGroup ? activeGroup.airSpeedSetpoint.value       : NaN)
    readonly property real   altAMSL:     _num(activeGroup ? activeGroup.altitudeAMSL.rawValue        : NaN)
    readonly property real   altTarget:      _num(activeGroup ? activeGroup.altitudeTuningSetpoint.value : NaN)
    readonly property real   altRelative:    _num(activeGroup ? activeGroup.altitudeRelative.value       : NaN)
    readonly property real   groundSpeedRaw: _num(activeGroup ? activeGroup.groundSpeed.rawValue         : NaN)   // m/s
    readonly property real   climbRateRaw:   _num(activeGroup ? activeGroup.climbRate.rawValue           : NaN)   // m/s
    readonly property string speedUnits:     activeGroup ? activeGroup.groundSpeed.units      : ""
    readonly property string altUnits:       activeGroup ? activeGroup.altitudeAMSL.units     : ""

    // Raw (m) -> display altitude units, for the vertical speed trend
    readonly property real   _altScale:      1

    // Ground course and wind
    readonly property real   _gpsCourse:     _num(_activeVehicle && _activeVehicle.gps ? _activeVehicle.gps.courseOverGround.rawValue : NaN)
    property real            groundCourseDeg: _gpsCourse
    readonly property real   windDirDeg:     _num(_activeVehicle && _activeVehicle.wind ? _activeVehicle.wind.direction.rawValue : NaN)
    readonly property real   windSpeed:      _num(_activeVehicle && _activeVehicle.wind ? _activeVehicle.wind.speed.value       : NaN)
    readonly property string windUnits:      _activeVehicle && _activeVehicle.wind ? _activeVehicle.wind.speed.units : ""
    // Direction the wind blows TO, relative to the nose (0 = tailwind, arrow up)
    readonly property real   _windArrowDeg:  (isNaN(windDirDeg) || isNaN(headingDeg)) ? NaN
                                             : windDirDeg + (windDirectionIsFrom ? 180 : 0) - headingDeg

    // Canvas fonts are set in px; convert from points the same way QML Text does
    readonly property int _largePx: Math.max(1, Math.round(fontSize * Screen.logicalPixelDensity * 25.4 / 72))
    readonly property int _smallPx: Math.max(1, Math.round(_largePx * smallFontScale))

    // ---------------------------------------------------------------- layout (shared by all layers)
    readonly property real _cx:         width / 2
    readonly property real _cy:         height / 2
    readonly property real _pad:        textPadding
    readonly property real _margin:     textPadding * 3
    readonly property real _smallH:     _smallPx * 1.2
    readonly property real _largeH:     _largePx * 1.2
    readonly property real _boxH:       _largePx + 2 * _pad
    readonly property real _arrow:      _boxH * 0.3
    readonly property real _tickMajor:  _smallH * 0.6
    readonly property real _tickMinor:  _smallH * 0.3
    readonly property real _index:      _tickMajor * 0.8     // size of the triangular indices
    readonly property real _boxW:       _boxMetrics.advanceWidth + 2 * _pad

    readonly property real _tapeH:       height * 0.5
    readonly property real _tapeTop:     _cy - _tapeH / 2
    readonly property real _tapeBottom:  _cy + _tapeH / 2
    readonly property real _tapeReserve: Math.max(_smallH, _largeH) + _pad * 2    // header above / readout below a tape
    readonly property real _speedX:      Math.max(_margin + _boxW + _arrow, width * 0.15)
    readonly property real _altX:        Math.min(width - _margin - _boxW - _arrow, width * 0.82)

    readonly property real _ppd:        (height / 2) / pitchRangeDeg
    readonly property real _rollR:      height * 0.12
    readonly property real _rollCy:     _margin + _index + _tickMajor + _rollR
    readonly property real _roseR:      Math.min(height * 0.1, width * 0.1)
    readonly property real _roseCy:     height - _margin - _smallH
    readonly property real _roseTop:    _roseCy - _roseR - _tickMajor - _index
    readonly property real _roseCardR:  _roseR + _tickMajor + _pad

    readonly property real _clipLeft:   _speedX + _arrow + _pad * 2
    readonly property real _clipRight:  _altX - _arrow - _pad * 2
    readonly property real _clipTop:    0
    readonly property real _clipBottom: _roseTop - _pad

    // Status bar: driven by a HorizontalFactValueGrid, columns split around the roll scale
    property var  statusGrid:          null
    readonly property int  _statusCols:     statusGrid && statusGrid.columns ? statusGrid.columns.count : 0
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

    // ---------------------------------------------------------------- acceleration estimate for the speed trend
    property real _speedAccel:     0      // display units per second, low-pass filtered
    property real _accelLastSpeed: NaN
    property real _accelLastTime:  0

    Timer {
        interval: 100
        repeat:   true
        running:  !!_root.activeGroup
        onTriggered: _root._sampleAccel()
    }

    function _sampleAccel() {
        var now = Date.now() / 1000
        var v   = airSpeed
        if (isNaN(v)) {
            _accelLastSpeed = NaN
            _speedAccel     = 0
            return
        }
        if (!isNaN(_accelLastSpeed)) {
            var dt = now - _accelLastTime
            if (dt > 0) {
                var k = Math.min(1, dt / accelFilterTau)
                _speedAccel += ((v - _accelLastSpeed) / dt - _speedAccel) * k
            }
        }
        _accelLastSpeed = v
        _accelLastTime  = now
    }

    // ---------------------------------------------------------------- repaint triggers
    onRollDegChanged:         pitchLadder.requestPaint()
    onPitchDegChanged:        pitchLadder.requestPaint()
    onHeadingDegChanged:      pitchLadder.requestPaint()     // heading ticks on the horizon
    onGroundSpeedRawChanged:  pitchLadder.requestPaint()     // flight path marker
    onGroundCourseDegChanged: pitchLadder.requestPaint()
    onZoomFactorChanged:      pitchLadder.requestPaint()
    onClimbRateRawChanged: {
        pitchLadder.requestPaint()
        altTape.requestPaint()
    }
    onAirSpeedChanged:          speedTape.requestPaint()
    onGroundSpeedChanged:       speedTape.requestPaint()
    onSpeedTargetChanged:       speedTape.requestPaint()
    onSpeedTargetActiveChanged: speedTape.requestPaint()
    on_SpeedAccelChanged:       speedTape.requestPaint()
    onAltAMSLChanged:           altTape.requestPaint()
    onAltTargetChanged:         altTape.requestPaint()
    onAltTargetActiveChanged:   altTape.requestPaint()
    onAltRelativeChanged:       altTape.requestPaint()

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
        courseMarker.requestPaint()
        windArrow.requestPaint()
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

        // Pitch ladder + horizon + flight path marker: the only full-width layer that repaints
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

            property var   _fact:  instrumentValueData ? instrumentValueData.fact : null
            property color _color: instrumentValueData && instrumentValueData.isValidColor(instrumentValueData.currentColor)
                                            ? instrumentValueData.currentColor : _root.fontColor
            property alias header: headerLabel
            property alias value: valueLabel
            opacity: instrumentValueData ? instrumentValueData.currentOpacity : 1

            Text {
                id: headerLabel
                width:               parent.width
                visible:             !!(cell.instrumentValueData && cell.instrumentValueData.text) || _fact
                horizontalAlignment: Text.AlignHCenter
                elide:               Text.ElideRight
                text:                cell.instrumentValueData ? cell.instrumentValueData.text : cell._fact.name
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
                id: valueLabel
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

        // Top status bar: autopilot / mission / navigation / link values (everything except bottomStatusFacts)
        Repeater {
            model: _root.statusGrid ? _root.statusGrid.columns : null

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

        // Bottom status area: engine values (bottomStatusFacts), bottom right next to the altitude tape
        Column {
            anchors.right:        parent.right
            anchors.rightMargin:  _root.width - _root._altX + _root._margin
            anchors.bottom:       parent.bottom
            anchors.bottomMargin: _root._margin
            spacing:              _root._pad
        }

        OsdValueCell {
            anchors.right: parent.right
            anchors.top: altTape.bottom
            anchors.topMargin: _root._largeH
            header.text: qsTr("Throttle %")
            header.font.pixelSize: _root._largePx
            value.font.pixelSize: _root._largePx
            width: _root._statusSlotW
            _fact: activeGroup.throttlePct
            visible: true
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
            x:      _root._cx - _root._roseCardR
            y:      _root._roseCy - _root._roseCardR
            width:  _root._roseCardR * 2
            height: _root._roseCardR + (_root._roseR + _root._tickMajor) * 0.18
            clip:   true

            LayerCanvas {
                id:       roseCard
                width:    _root._roseCardR * 2
                height:   _root._roseCardR * 2
                visible:  !isNaN(_root.headingDeg)
                rotation: isNaN(_root.headingDeg) ? 0 : -_root.headingDeg
                onPaint: {
                    var ctx = _root._begin(this, false)
                    _root._drawRoseCard(ctx, width / 2)
                }
            }

            // Ground course (track) marker on the card ring
            LayerCanvas {
                id:       courseMarker
                width:    _root._roseCardR * 2
                height:   _root._roseCardR * 2
                visible:  !isNaN(_root.groundCourseDeg) && !isNaN(_root.headingDeg)
                          && _root.groundSpeedRaw >= _root.fpvMinGroundSpeed
                rotation: visible ? _root.groundCourseDeg - _root.headingDeg : 0
                onPaint: {
                    var ctx = _root._begin(this, false)
                    var c   = width / 2
                    var y   = c - _root._roseR - _root._tickMajor * 0.5
                    var s   = _root._index * 0.6
                    ctx.lineWidth = _root.majorLineWidth
                    ctx.beginPath()
                    ctx.moveTo(c,     y - s)
                    ctx.lineTo(c + s, y)
                    ctx.lineTo(c,     y + s)
                    ctx.lineTo(c - s, y)
                    ctx.closePath()
                    ctx.stroke()
                }
            }
        }

        // Wind relative to the aircraft: arrow up = tailwind, left of the compass rose
        Item {
            readonly property real size: _root._smallH * 2.5

            visible: !isNaN(_root._windArrowDeg) && !isNaN(_root.windSpeed)
            width:   Math.max(size, windText.implicitWidth)
            height:  size + windText.implicitHeight
            x:       _root._cx - _root._roseCardR - _root._smallH * 2 - width
            y:       _root._roseCy - _root._roseR - size / 2

            LayerCanvas {
                id:       windArrow
                anchors.horizontalCenter: parent.horizontalCenter
                width:    parent.size
                height:   parent.size
                rotation: isNaN(_root._windArrowDeg) ? 0 : _root._windArrowDeg
                onPaint: {
                    var ctx = _root._begin(this, false)
                    var c   = width / 2
                    var ts  = _root._index
                    ctx.lineWidth = _root.majorLineWidth
                    ctx.beginPath()
                    ctx.moveTo(c, height * 0.9)
                    ctx.lineTo(c, height * 0.1 + ts)
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.moveTo(c, height * 0.1)
                    ctx.lineTo(c - ts * 0.6, height * 0.1 + ts)
                    ctx.lineTo(c + ts * 0.6, height * 0.1 + ts)
                    ctx.closePath()
                    ctx.fillStyle = _root.lineColor
                    ctx.fill()
                }
            }

            Text {
                id:                       windText
                anchors.top:              windArrow.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text:                     isNaN(_root.windSpeed) ? "" : _root.windSpeed.toFixed(1) + " " + _root.windUnits
                color:                    _root.fontColor
                font.pixelSize:           _root._smallPx
                font.bold:                true
                font.family:              "sans-serif"
            }
        }

        // Air speed tape (left): scale outside, target bug + acceleration trend inside, ground speed readout below
        LayerCanvas {
            id:     speedTape
            x:      0
            y:      _root._tapeTop - _root._tapeReserve
            width:  _root._clipLeft + _root._boxW / 2
            height: _root._tapeH + _root._tapeReserve * 2
            onPaint: {
                var ctx = _root._begin(this, true)
                var ppu = _root._tapeH / _root.speedRange
                var v   = _root.airSpeed

                _root._drawTapeScale(ctx, _root._speedX, -1, true, v,
                                     _root.speedRange, _root.speedMinorStep, _root.speedMajorStep, 0)
                if (!isNaN(v)) {
                    if (_root.speedTargetActive)
                        _root._drawTargetBug(ctx, _root._speedX, 1, v, _root.speedTarget, ppu)
                    _root._drawTrendArrow(ctx, _root._speedX, 1, _root._speedAccel * _root.trendSeconds, ppu)
                }
                _root._drawValueBoxH(ctx, _root._speedX, _root._cy, 1, isNaN(v) ? "---" : v.toFixed(1))

                _root._setFont(ctx, true)
                ctx.textAlign    = "right"
                ctx.textBaseline = "top"
                ctx.fillText("GS " + (isNaN(_root.groundSpeed) ? "---" : _root.groundSpeed.toFixed(1)),
                             _root._speedX, _root._tapeBottom + _root._pad)
            }
        }

        // Altitude tape (right, AMSL): scale outside, target bug + vertical speed trend inside, AGL readout below
        LayerCanvas {
            id:     altTape
            x:      _root._altX - _root._tickMajor * 3 - _root._pad
            y:      _root._tapeTop - _root._tapeReserve
            width:  Math.max(0, _root.width - x)
            height: _root._tapeH + _root._tapeReserve * 2
            onPaint: {
                var ctx = _root._begin(this, true)
                var ppu = _root._tapeH / _root.altRange
                var v   = _root.altAMSL

                _root._drawTapeScale(ctx, _root._altX, 1, true, v,
                                     _root.altRange, _root.altMinorStep, _root.altMajorStep, -Infinity)
                if (!isNaN(v)) {
                    if (_root.altTargetActive)
                        _root._drawTargetBug(ctx, _root._altX, -1, v, _root.altTarget, ppu)
                    _root._drawTrendArrow(ctx, _root._altX, -1,
                                          _root.climbRateRaw * _root.trendSeconds * _root._altScale, ppu)
                }
                _root._drawValueBoxH(ctx, _root._altX, _root._cy, -1, isNaN(v) ? "---" : v.toFixed(1))

                _root._setFont(ctx, true)
                ctx.textAlign    = "left"
                ctx.textBaseline = "top"
                ctx.fillText("AGL " + (isNaN(_root.altRelative) ? "---" : _root.altRelative.toFixed(1)),
                             _root._altX, _root._tapeBottom + _root._pad)
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

    function deg2rad(d) { return d * Math.PI / 180.0 }

    function rad2deg(r) { return r * 180.0 / Math.PI }

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

    // Target bug on the inner side of a tape: notched bracket pointing at the axis,
    // pinned to the tape end while the target is out of range. innerDir = +1 right of the axis, -1 left.
    function _drawTargetBug(ctx, axisX, innerDir, value, target, ppu) {
        if (isNaN(value) || isNaN(target))
            return
        var tickH  = _largeH + _pad
        var h = tickH * 1.7
        var w  = _tickMajor * 2
        var y  = Math.max(_tapeTop + h / 2, Math.min(_tapeBottom - h / 2, _cy - (target - value) * ppu))
        axisX  -= w / 2 * innerDir
        var x1 = axisX + innerDir * w

        ctx.lineWidth = majorLineWidth
        ctx.beginPath()
        ctx.moveTo(axisX, y - tickH / 2)
        ctx.lineTo(axisX, y - h / 2)
        ctx.lineTo(x1,    y - h / 2)
        ctx.lineTo(x1,    y + h / 2)
        ctx.lineTo(axisX, y + h / 2)
        ctx.lineTo(axisX, y + tickH / 2)
        ctx.lineTo(axisX + innerDir * w * 0.5, y)
        ctx.closePath()
        ctx.stroke()
    }

    // Trend arrow on the inner side of a tape: from the current value to value + delta (clamped to the tape)
    function _drawTrendArrow(ctx, axisX, innerDir, delta, ppu) {
        if (isNaN(delta))
            return
        var len = delta * ppu
        if (Math.abs(len) < _tickMinor)
            return
        var x  = axisX + innerDir * (_tickMajor * 1.5 + _pad)   // clear of the target bug
        var y1 = Math.max(_tapeTop, Math.min(_tapeBottom, _cy - len))
        var hs = (y1 < _cy) ? _index * 0.6 : -_index * 0.6

        ctx.lineWidth = majorLineWidth
        ctx.beginPath()
        ctx.moveTo(x, _cy)
        ctx.lineTo(x, y1)
        ctx.moveTo(x - _index * 0.5, y1 + hs)
        ctx.lineTo(x, y1)
        ctx.lineTo(x + _index * 0.5, y1 + hs)
        ctx.stroke()
    }

    // Flight path marker ("bird") at screen position (x, y), kept level with the screen
    function _drawFlightPathMarker(ctx, x, y) {
        var r = _smallH * 0.45
        ctx.lineWidth = majorLineWidth
        ctx.beginPath()
        ctx.arc(x, y, r, 0, 2 * Math.PI, false)
        ctx.moveTo(x - r * 2.4, y); ctx.lineTo(x - r, y)
        ctx.moveTo(x + r, y);       ctx.lineTo(x + r * 2.4, y)
        ctx.moveTo(x, y - r);       ctx.lineTo(x, y - r * 1.8)
        ctx.stroke()
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
        ctx.fillText("AS", _speedX , _tapeTop - _pad)
        ctx.textAlign    = "left"
        ctx.fillText("ASL", _altX + _pad / 2, _tapeTop - _pad)
    }

    // Full 360 degree card for heading 0, centred at (c, c) in the card canvas. 10 degree ticks only.
    function _drawRoseCard(ctx, c) {
        var r     = _roseR
        var toRad = Math.PI / 180
        var cardinals = { 0: "N", 90: "E", 180: "S", 270: "W" }

        ctx.lineWidth = majorLineWidth
        ctx.beginPath()
        for (var d = 0; d < 360; d += 10) {
            var s = Math.sin(d * toRad)
            var k = Math.cos(d * toRad)
            ctx.moveTo(c + r * s,                c - r * k)
            ctx.lineTo(c + (r + _tickMajor) * s, c - (r + _tickMajor) * k)
        }
        ctx.stroke()

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

        var halfW     = width * 0.11
        var gap       = width * 0.055
        var endTick   = _smallH * 0.4
        var labels    = []
        var rollRad   = -rollDeg * Math.PI / 180
        var smallFont = "bold " + _smallPx + "px sans-serif"
        var tinyFont  = "bold " + Math.max(1, Math.round(_smallPx * horizonFontScale)) + "px sans-serif"

        // Camera projection; zoom narrows the effective FOV
        const baseHfov = (_videoManager && _videoManager.hfov > 5 ? _videoManager.hfov : 103)
        const halfHfov = Math.atan(Math.tan(deg2rad(baseHfov / 2)) / Math.max(1e-3, zoomFactor))
        const fDisp = (width / 2) / Math.tan(halfHfov)
        const fHDisp = fDisp * (width/height)
        var headingToX = function (relHead) {
            return fDisp * Math.tan(relHead * Math.PI / 180)
        }

        var pitchToY = function (relDeg) {
            return fHDisp * Math.tan(deg2rad(relDeg))
        }

        ctx.save()
        ctx.translate(_cx, _cy)
        ctx.rotate(rollRad)

        // Horizon line + small heading ticks; only even ticks are labelled to reduce clutter
        var yh = pitchToY(pitchDeg)
        ctx.lineWidth = majorLineWidth
        ctx.beginPath()
        ctx.moveTo(-width, yh)
        ctx.lineTo(width, yh)
        const halfSpan = Math.ceil(rad2deg(Math.atan(0.5 * Math.tan(halfHfov))))
        if (!isNaN(headingDeg)) {
            const left = headingDeg - halfSpan
            const first = Math.floor(left / _root.tickStep) * tickStep
            for( let b = first; b < headingDeg + halfSpan + _root.tickStep; b += tickStep) {
                const rel = _norm360(b - headingDeg) - 180
                const tickX = fDisp * Math.tan(deg2rad(rel))
                ctx.moveTo(tickX, yh)
                ctx.lineTo(tickX, yh - _tickMinor)
                if (Math.abs(Math.round(b / tickStep) % 2) === 0)
                    labels.push({ text: _fmtTick(_norm360(b)), x: tickX, y: yh - _tickMinor - _pad,
                                  align: "center", baseline: "bottom", tiny: true })
            }
        }
        ctx.stroke()

        // Current heading mark: boresight projected onto the horizon
        if (!isNaN(headingDeg)) {
            var ts = _index * 0.8
            ctx.beginPath()
            ctx.moveTo(0, yh + 1)
            ctx.lineTo(-ts * 0.6, yh + ts)
            ctx.lineTo( ts * 0.6, yh + ts)
            ctx.closePath()
            ctx.stroke()
        }

        // Pitch lines in four strokes: (nose up / nose down) x (major / minor)
        for (var g = 0; g < 4; g++) {
            var positive = (g < 2)
            var major    = (g % 2 === 0)
            var w        = major ? halfW : halfW * 0.85
            var t        = positive ? endTick : -endTick      // end ticks point towards the horizon

            ctx.lineWidth = major ? majorLineWidth : minorLineWidth
            if (positive) {
                ctx.setLineDash([])
            } else {
                var dash = (w - gap) / (major ? 14 : 7)
                major ? ctx.setLineDash([dash, dash]) : ctx.setLineDash([dash / 1.4, dash / 1.4])
            }

            ctx.beginPath()
            for (var a = 5; a <= 90; a += 5) {
                if ((a % 10 === 0) !== major)
                    continue
                var pa = positive ? a : -a
                var rel = pitchDeg - pa
                if (Math.abs(rel) >= 89)
                    continue
                var y  = pitchToY(rel)
                if (Math.abs(y) > height)
                    continue
                ctx.moveTo(-w, y + t); ctx.lineTo(-w, y); ctx.lineTo(-gap, y)
                ctx.lineDashOffset = 0
                ctx.moveTo(w, y + t);    ctx.lineTo(w, y);  ctx.lineTo(gap, y)
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
            ctx.font         = lb.tiny ? tinyFont : smallFont
            ctx.textAlign    = lb.align
            ctx.textBaseline = lb.baseline
            ctx.fillText(lb.text, lb.x, lb.y)
        }
        ctx.restore()

        // Flight path marker: where the aircraft is actually moving (velocity vector)
        if (!isNaN(groundSpeedRaw) && groundSpeedRaw >= fpvMinGroundSpeed && !isNaN(climbRateRaw)) {
            var fpa   = rad2deg(Math.atan2(climbRateRaw, groundSpeedRaw))
            var drift = (isNaN(groundCourseDeg) || isNaN(headingDeg)) ? 0
                                                                      : _norm360(groundCourseDeg - headingDeg + 180) - 180
            if (Math.abs(drift) < 80 && Math.abs(pitchDeg - fpa) < 89) {
                var fx = fDisp * Math.tan(deg2rad(drift))
                var fy = pitchToY(pitchDeg - fpa)
                _drawFlightPathMarker(ctx,
                                      _cx + fx * Math.cos(rollRad) - fy * Math.sin(rollRad),
                                      _cy + fx * Math.sin(rollRad) + fy * Math.cos(rollRad))
            }
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
