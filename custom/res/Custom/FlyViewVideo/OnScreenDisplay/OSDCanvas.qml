import QtQuick

Canvas {
    id: control

    property OSDStyle style: OSDStyle {}

    onVisibleChanged: requestPaint()
    onStyleChanged: {
        requestPaint()
    }

    Connections {
        target: style
        onColorChanged: requestPaint()
    }

    onWidthChanged:              requestPaint()
    onHeightChanged:             requestPaint()
    // onStyleChanged:              requestPaint()

    // Clears the canvas and sets the common style
    function begin() {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.clearRect(0, 0, width, height)
        ctx.lineCap     = "round"
        ctx.lineJoin    = "round"
        ctx.strokeStyle = style.color
        ctx.lineWidth = style.majorLineWidth
        _setFontSize(ctx, style.defaultFontSize)
        return ctx
    }

    function _normToCoord(point) {
        return Qt.vector2d(point.x * width, point.y * height)
    }
    function _coordToNorm(point) {
        return Qt.vector2d(point.x / width, point.y / height)
    }

    function _setFontSize(ctx, size) {
        ctx.font      = "bold " + size + "px sans-serif"
        ctx.fillStyle = style.color
    }
}
