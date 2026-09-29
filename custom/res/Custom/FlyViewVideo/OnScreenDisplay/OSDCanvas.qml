import QtQuick

Canvas {
    id: control

    property OSDStyle style: OSDStyle {}

    onWidthChanged:              requestPaint()
    onHeightChanged:             requestPaint()
    onStyleChanged:              requestPaint()

    // Clears the canvas and sets the common style
    function begin() {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.clearRect(0, 0, width, height)
        ctx.lineCap     = "round"
        ctx.lineJoin    = "round"
        ctx.strokeStyle = style.color
        ctx.lineWidth = style.majorLineWidth
        setFont(ctx, false)
        return ctx
    }

    function setFont(ctx, large) {
        ctx.font      = "bold " + (large ? style.majorFontSize : style.minorFontSize) + "px sans-serif"
        ctx.fillStyle = style.color
    }
}
