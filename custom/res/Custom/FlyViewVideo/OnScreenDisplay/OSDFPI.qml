import QtQuick
import QtQuick3D
import QGroundControl
import QGroundControl.Controls
import FoxFour.Widgets 1.0

OSDCanvas{
    id: _root
    property real roll: 0
    property real pitch: 0
    property real heading: 0
    property real radius : 10
    property real minFpiSpeed: 1.0
    property vector3d velNED: Qt.vector3d(0, 0, 0)
    property var _repaintKeys: [roll, pitch, heading, velNED, playerSize, velNED]
    on_RepaintKeysChanged: requestPaint()

    property var playerSize: Qt.size()
    property var _videoManager:     QGroundControl.videoManager
    property real _baseHorFov: {
                if (_settings.hudHorFov.value <= 1)
                        return (_videoManager && _videoManager.hfov > 5 ? _videoManager.hfov : 103)
                return _settings.hudHorFov.value
    }
    property real _halfHorFov: (_baseHorFov / 2) * Math.PI / 180
    property real _fHorDisp: (playerSize.width / 2) / Math.tan(_halfHorFov)
    property real _aspect: playerSize.height / playerSize.width
    property real _fVertDisp: {
                let vertFov =  (_baseHorFov / 2) * _aspect;
                if (_settings.hudVertFov.value > 1){
                    vertFov = _settings.hudVertFov.value / 2
                }
                return (playerSize.height / 2) / Math.tan(vertFov * Math.PI / 180)
    }


    function _computeFpi() {
        // body -> NED rotation, aerospace order: yaw (Z/down), pitch (Y/east), roll (X/forward)
        const qBody = Quaternion.fromAxesAndAngles(
                Qt.vector3d(1, 0, 0), roll,
                Qt.vector3d(0, 1, 0), pitch,
                Qt.vector3d(0, 0, 1), heading)


        // NED -> body (x=forward, y=right, z=down)
        const b = qBody.inverted().times(velNED)
        const speed = velNED.length()

        // temporary test: velocity exactly along the heading, 25 m/s, level
        const d2r = Math.PI / 180
        const vTest = Qt.vector3d(25 * Math.cos(heading * d2r), 25 * Math.sin(heading * d2r), 0)
        const bt = qBody.inverted().times(vTest)
        const course = Math.atan2(velNED.y, velNED.x) * 180 / Math.PI

        if (speed < minFpiSpeed || b.x <= 0.01 * speed) {
            return Qt.point(0, 0)
        }

        const x = Math.max(-playerSize.width  / 2, Math.min(playerSize.width  / 2, _fHorDisp  * b.y / b.x))
        const y = Math.max(-playerSize.height / 2, Math.min(playerSize.height / 2, _fVertDisp * b.z / b.x))
        return Qt.point(x, y)
    }

    onPaint: {
        var ctx = begin()

        var fpiPos = _computeFpi()
        ctx.lineWidth = style.minorLineWidth
        ctx.translate((width / 2) + fpiPos.x, (height / 2) + fpiPos.y)
        ctx.beginPath()
        ctx.arc(0, 0, radius, 0, 2 * Math.PI)
        ctx.moveTo(-radius, 0)
        ctx.lineTo(-radius * 1.5, radius)
        ctx.lineTo(-radius * 3, radius)
        ctx.moveTo(radius, 0)
        ctx.lineTo(radius * 1.5, radius)
        ctx.lineTo(radius * 3, radius)
        ctx.stroke()
    }

    // _repaint
    // property real
}
