import QtQuick

import QGroundControl.Controls
import QGroundControl
import FoxFour.Widgets 1.0

Item {
    id: _root
    property var      playerSize: Qt.size()      // size of the visible video image
    property OSDStyle style: OSDStyle {}
    property real     roll: 0
    property real     pitch: 0
    property real     heading: 0
    property real     spacing: style.majorFontSize * 5
    property CameraModel cam: null

    property var _videoManager: QGroundControl.videoManager

    clip: true

    OSDLadder {
        id: ladder
        style: _root.style
        x: -_root.x          // _root = the OSDHorizon item; puts the canvas origin at the OSD origin
        y: -_root.y
        width:  playerSize.width
        height: playerSize.height
        property real pitchMajorTickLength: width / 20
        property real pitchMinorTickLength: pitchMajorTickLength * 0.8
        _repaintKeys: [_root.heading, cam.revision, width, height]

        drawer: function(ctx) {
            ctx.save()
            drawBoresight(ctx)
            drawPitchLadder(ctx)
            drawHeadingTape(ctx)
            ctx.restore()
        }

        function _onScreen(p, margin) {
            return p && p.x >= -margin && p.x <= width + margin
                     && p.y >= -margin && p.y <= height + margin
        }

        // Fixed aircraft symbol at the camera axis (principal point, not screen center)
        // plus the dashed vertical from it down/up to the true horizon.
        function drawBoresight(ctx) {
            const b = cam.boresight()
            const half = (_root.spacing - style.fontPadding) / 2
            const s = style.minorFontSize

            ctx.setLineDash([])
            ctx.beginPath()
            ctx.moveTo(b.x - half, b.y)
            ctx.lineTo(b.x - s, b.y)
            ctx.lineTo(b.x, b.y + s)
            ctx.lineTo(b.x + s, b.y)
            ctx.lineTo(b.x + half, b.y)
            ctx.stroke()

            const ae = cam.boresightAzEl()
            ctx.setLineDash([style.majorLineWidth])
            ctx.beginPath()
            cam.traceCurve(ctx, function(e) { return [ae.az, e] }, ae.el, 0, 32)
            ctx.stroke()
            ctx.setLineDash([])
        }

        function drawPitchLadder(ctx) {
            const gap = _root.spacing / 2
            const tipLen = style.defaultFontSize / 2
            const margin = gap + pitchMajorTickLength + style.defaultFontSize * 3
            ctx.textBaseline = "middle"

            for (let val = -90; val <= 90; val += 5) {
                if (val === 0)
                    continue
                const f = cam.localFrame(0, val)
                if (!f || !_onScreen(f.p, margin))
                    continue

                const isMajor = (val % 10 === 0)
                const len = isMajor ? pitchMajorTickLength : pitchMinorTickLength
                // n points toward the ground side, so the tip always points at the horizon
                const tip = f.n.times(val > 0 ? tipLen : -tipLen)

                ctx.setLineDash(val > 0 ? [] : [len / (isMajor ? 17 : 14)])
                ctx.lineWidth = isMajor ? style.majorLineWidth : style.minorLineWidth
                ctx.beginPath()
                for (const side of [1, -1]) {
                    const a = f.p.plus(f.t.times(side * gap))
                    const e = f.p.plus(f.t.times(side * (gap + len)))
                    const c = e.plus(tip)
                    ctx.moveTo(a.x, a.y)
                    ctx.lineTo(e.x, e.y)
                    ctx.lineTo(c.x, c.y)
                }
                ctx.stroke()

                if (isMajor) {
                    const text = val.toFixed(0)
                    const off = gap + len + style.fontPadding
                    const r = f.p.plus(f.t.times(off)).plus(tip.times(0.5))
                    const l = f.p.minus(f.t.times(off)).plus(tip.times(0.5))
                    ctx.textAlign = "left"
                    ctx.fillText(text, r.x, r.y)
                    ctx.textAlign = "right"
                    ctx.fillText(text, l.x, l.y)
                }
            }
            ctx.setLineDash([])
        }

        function drawHeadingTape(ctx) {
            ctx.setLineDash([])
            ctx.lineWidth = style.minorLineWidth

            // true horizon: a straight world line, curved on screen by the lens
            ctx.beginPath()
            cam.traceCurve(ctx, function(az) { return [az, 0] }, -180, 180, 240)
            ctx.stroke()

            _setFontSize(ctx, style.defaultFontSize)
            ctx.textAlign = "center"
            ctx.textBaseline = "bottom"
            ctx.beginPath()
            for (let val = 0; val < 360; val += 10) {
                const rel = _root.wrap180(val - _root.heading)
                const f = cam.localFrame(rel, 0)
                if (!f || !_onScreen(f.p, style.defaultFontSize * 2))
                    continue
                const up = f.n.times(-1)
                const e = f.p.plus(up.times(style.minorTickLength))
                ctx.moveTo(f.p.x, f.p.y)
                ctx.lineTo(e.x, e.y)
                if (val % 90 === 0)
                    ctx.fillText(val.toFixed(0), e.x, e.y)
            }
            ctx.stroke()
        }
    }
    function wrap180(deg) {
        let v = ((deg + 180) % 360 + 360) % 360 - 180
        return v
    }
}
