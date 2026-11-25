import QtQuick

// Unified "world angle -> distorted screen pixel" projection.
//
// Pipeline for every HUD point:
//   (azimuth, elevation) in the level frame
//     -> unit ray in level frame
//     -> rotate by pitch, roll, camera mount tilt  -> ray in camera frame (x right, y down, z forward)
//     -> lens distortion (OpenCV conventions)      -> distorted normalized coords
//     -> intrinsics K, scaled to the viewport      -> screen pixel
QtObject {
    id: cam

    property var   calib: null

    // used when no calibration has been received yet (ideal pinhole)
    property real   fallbackHfov: 103

    // size of the area the video image actually occupies on screen
    property size   viewSize: Qt.size(1, 1)

    // ---- attitude (degrees) ----
    property real roll:      0
    property real pitch:     0
    property real mountTilt: 0      // camera tilted up relative to the body, deg

    // ---- derived intrinsics in viewport pixels ----
    readonly property bool calibrated: !!calib && calib.valid === true
                                       && !!calib.intrinsics && calib.intrinsics.length >= 4
    readonly property real _sx: calibrated ? viewSize.width  / calib.width  : 1
    readonly property real _sy: calibrated ? viewSize.height / calib.height : 1
    // K = [fx, cx, fy, cy]
    readonly property real fx: calibrated ? calib.intrinsics[0] * _sx
                                          : (viewSize.width / 2) / Math.tan(fallbackHfov * Math.PI / 360)
    readonly property real cx: calibrated ? calib.intrinsics[1] * _sx : viewSize.width / 2
    readonly property real fy: calibrated ? calib.intrinsics[2] * _sy : fx
    readonly property real cy: calibrated ? calib.intrinsics[3] * _sy : viewSize.height / 2

    // 0 = ideal pinhole, 1 = OpenCV radtan/rational, 2 = fisheye (equidistant)
    readonly property int _kind: {
        if (!calibrated)
            return 0
        const m = (calib.model || "").toLowerCase()
        if (m.indexOf("fisheye") >= 0 || m.indexOf("equidistant") >= 0 || m.indexOf("kannala") >= 0)
            return 2
        return 1
    }

    readonly property var _d: {
        const o = [0, 0, 0, 0, 0, 0, 0, 0]
        if (calibrated && calib.distortion)
            for (let i = 0; i < Math.min(calib.distortion.length, 8); i++)
                o[i] = Number(calib.distortion[i]) || 0
        return o
    }

    // Polynomial models fold back on themselves outside the calibrated area.
    // Find where the radial mapping stops being monotonic and reject rays beyond it.
    // kind 1: limit on undistorted normalized radius r; kind 2: limit on incidence angle theta.
    readonly property real _limit: _computeLimit(_kind, _d)

    // changes whenever anything affecting the projection changes (use in repaint keys)
    readonly property var revision: [fx, fy, cx, cy, _kind, _d, _limit, roll, pitch, mountTilt]

    readonly property real _D2R: Math.PI / 180

    function _computeLimit(kind, d) {
        if (kind === 1) {
            let prev = 0
            for (let r = 0.01; r <= 10; r += 0.01) {
                const r2 = r * r
                const num = 1 + r2 * (d[0] + r2 * (d[1] + r2 * d[4]))
                const den = 1 + r2 * (d[5] + r2 * (d[6] + r2 * d[7]))
                const f = r * num / den
                if (f <= prev || den <= 0)
                    return r - 0.01
                prev = f
            }
            return 10
        }
        if (kind === 2) {
            let prev = 0
            for (let t = 0.005; t < Math.PI - 0.01; t += 0.005) {
                const t2 = t * t
                const f = t * (1 + t2 * (d[0] + t2 * (d[1] + t2 * (d[2] + t2 * d[3]))))
                if (f <= prev)
                    return t - 0.005
                prev = f
            }
            return Math.PI - 0.01
        }
        return 10
    }

    // ---- rotations (v = [x, y, z]) ----
    // rotation about camera x axis; positive angle = nose/camera up
    function _rotX(v, deg) {
        const c = Math.cos(deg * _D2R), s = Math.sin(deg * _D2R)
        return [v[0], v[1] * c + v[2] * s, -v[1] * s + v[2] * c]
    }
    // rotation about forward axis; positive angle = right wing down
    function _rotZ(v, deg) {
        const c = Math.cos(deg * _D2R), s = Math.sin(deg * _D2R)
        return [v[0] * c + v[1] * s, -v[0] * s + v[1] * c, v[2]]
    }

    // unit ray in level frame: az > 0 right of the nose, el > 0 above the horizon
    function levelDir(azDeg, elDeg) {
        const a = azDeg * _D2R, e = elDeg * _D2R
        return [Math.cos(e) * Math.sin(a), -Math.sin(e), Math.cos(e) * Math.cos(a)]
    }

    function levelToCamera(v) {
        return _rotX(_rotZ(_rotX(v, pitch), roll), mountTilt)
    }

    function cameraToLevel(v) {
        return _rotX(_rotZ(_rotX(v, -mountTilt), -roll), -pitch)
    }

    // ---- core: camera-frame ray -> screen pixel (Qt.vector2d) or null ----
    function projectCam(v) {
        const x = v[0], y = v[1], z = v[2]
        let xd, yd

        if (_kind === 2) {
            const R = Math.hypot(x, y)
            const th = Math.atan2(R, z)
            if (th > _limit)
                return null
            if (R < 1e-12) {
                xd = 0; yd = 0
            } else {
                const t2 = th * th
                const thd = th * (1 + t2 * (_d[0] + t2 * (_d[1] + t2 * (_d[2] + t2 * _d[3]))))
                xd = thd * x / R
                yd = thd * y / R
            }
        } else {
            if (z <= 1e-6)
                return null
            const a = x / z, b = y / z
            const r2 = a * a + b * b
            if (_kind === 1) {
                if (r2 > _limit * _limit)
                    return null
                const d = _d
                const radial = (1 + r2 * (d[0] + r2 * (d[1] + r2 * d[4])))
                             / (1 + r2 * (d[5] + r2 * (d[6] + r2 * d[7])))
                xd = a * radial + 2 * d[2] * a * b + d[3] * (r2 + 2 * a * a)
                yd = b * radial + d[2] * (r2 + 2 * b * b) + 2 * d[3] * a * b
            } else {
                xd = a; yd = b
            }
        }
        return Qt.vector2d(fx * xd + cx, fy * yd + cy)
    }

    // level-frame angles -> screen pixel or null
    function project(azDeg, elDeg) {
        return projectCam(levelToCamera(levelDir(azDeg, elDeg)))
    }

    // Local screen-space frame at a world point, for drawing pixel-sized
    // decorations (ticks, bars, labels) that follow the distorted horizon.
    //   p = anchor pixel, t = unit tangent pointing "right along the horizon",
    //   n = unit normal pointing "down" (toward the ground side when level)
    function localFrame(azDeg, elDeg) {
        const dir = levelDir(azDeg, elDeg)
        const a = azDeg * _D2R
        const eps = 1e-3
        const side = [Math.cos(a) * eps, 0, -Math.sin(a) * eps]   // horizontal "right" vector

        const p = projectCam(levelToCamera(dir))
        if (!p)
            return null
        let q = projectCam(levelToCamera([dir[0] + side[0], dir[1], dir[2] + side[2]]))
        let t
        if (q) {
            t = q.minus(p)
        } else {
            q = projectCam(levelToCamera([dir[0] - side[0], dir[1], dir[2] - side[2]]))
            if (!q)
                return null
            t = p.minus(q)
        }
        if (t.length() < 1e-9)
            return null
        t = t.normalized()
        return { p: p, t: t, n: Qt.vector2d(-t.y, t.x) }
    }

    // Boresight (camera axis) on screen and in level-frame angles
    function boresight() {
        return Qt.vector2d(cx, cy)
    }
    function boresightAzEl() {
        const v = cameraToLevel([0, 0, 1])
        return { az: Math.atan2(v[0], v[2]) / _D2R,
                 el: Math.asin(Math.max(-1, Math.min(1, -v[1]))) / _D2R }
    }

    // Trace a curve fn(s) -> [az, el] into the current path, sampled so that
    // straight world lines become correctly curved on screen.
    function traceCurve(ctx, fn, s0, s1, steps) {
        const maxJump = Math.hypot(viewSize.width, viewSize.height) / 2
        let last = null
        for (let i = 0; i <= steps; i++) {
            const ae = fn(s0 + (s1 - s0) * i / steps)
            const p = project(ae[0], ae[1])
            if (!p) {
                last = null
                continue
            }
            if (last && p.minus(last).length() < maxJump)
                ctx.lineTo(p.x, p.y)
            else
                ctx.moveTo(p.x, p.y)
            last = p
        }
    }
}
