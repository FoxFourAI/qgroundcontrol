import QtQuick

import QGroundControl

OSDCanvas {
    id: _root
    property real tickStep: 1
    property real majorEvery: 5
    property real tickSpacing: style.majorFontSize
    property real from:     -Infinity
    property real to:       Infinity
    property real wrap:     0 //if set , it will wrap value instead
    property int valueDecimals: 1
    property bool mirrored: false

    property int orientation: Qt.Vertical

    //drawers
    property var drawer: standardDrawer
    property var ticksDrawer: standardTickDrawer
    property var valueDrawer: standardValueDrawer

    //up to 2 sources are supported
    property var sources:    []
    property var headers:    []
    property var footers:    []

    // named ticks and sectors are only available in single soruce ladders
    property var namedTicks: []
    property var sectors:    []

    property var _repaintKeys: [width, height, sources, headers, footers, namedTicks, sectors, orientation, valueDecimals, wrap, from, to, tickStep, drawer, ticksDrawer]
    on_RepaintKeysChanged: requestPaint()

    function _wrapValue(v) {
        return wrap > 0 ? ((v % wrap) + wrap) % wrap : v
    }

    //standard tick drawer draw ticks from the middle to the edges
    function standardTickDrawer(ctx, ladderStart, ladderEnd, value, leftSided) {
        if (isFinite(value) || tickStep <=0 || tickSpacing <=0) {
            return
        }

        let center = ladderEnd - ladderStart
        let roundedCenter = center / value * Math.round(value) / tickStep
    }

    function standardDrawer(ctx) {
        ctx.strokeRect(0,0,width,height)
        let ladderStart = Qt.point(1 - style.majorLineWidth / width, 1 - style.majorLineWidth / height)
        var ladderEnd = Qt.point(0,0)
        if (orientation === Qt.Horizontal) {
            ladderStart.x = 0
            ladderEnd.x = 1
            if (mirrored) {
                ladderStart.y = 1 - ladderStart.y
            }
            ladderEnd.y = ladderStart.y
        } else {
            ladderStart.y = 0
            ladderEnd.y = 1
            if (mirrored) {
                ladderStart.x = 1 - ladderStart.x
            }
            ladderEnd.x = ladderStart.x
        }
        ctx.beginPath()
        ctx.moveTo(width * ladderStart.x, height * ladderStart.y)
        ctx.lineTo(width * ladderEnd.x, height * ladderEnd.y)
        ctx.stroke()

        for(let index = 0 ; index < sources.length; ++ index) {
            ticksDrawer(ctx, ladderStart, ladderEnd, sources[index], index % 2)
        }
    }

    function standardValueDrawer(ctx) {

    }

    onPaint: {
        let ctx = begin()
        drawer(ctx)
    }

}
