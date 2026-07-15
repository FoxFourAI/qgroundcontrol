import QtQuick

Item {
    id: rootItem

    required property var  camera
    required property real videoWidth
    required property real videoHeight

    // Drag origin in parent coordinates
    property bool _dragging: false
    property real _dragStartX: 0
    property real _dragStartY: 0
    property real _currentX: 0
    property real _currentY: 0
    property int  _threshold: 10
    property bool _zooming: false
    property int  _trackSize: 15

    readonly property bool _trackingEnabled: camera && camera.trackingEnabled
    readonly property bool _canTrackPoint: _trackingEnabled && camera.supportsTrackingPoint
    readonly property bool _canTrackRect: _trackingEnabled && camera.supportsTrackingRect

    // function mouseClicked(mouseX, mouseY) {

    //     if (!_canTrackPoint) {
    //         return
    //     }
    //     if (videoWidth <= 0 || videoHeight <= 0) {
    //         return
    //     }
    //     // Click = point tracking
    //     var marginH = (width - videoWidth) / 2
    //     var marginV = (height - videoHeight) / 2
    //     var nx = Math.max(Math.min((mouseX - marginH) / videoWidth, 1.0), 0.0)
    //     var ny = Math.max(Math.min((mouseY - marginV) / videoHeight, 1.0), 0.0)
    //     var pointRadius = 20
    //     camera.startTrackingPoint(Qt.point(nx, ny), Math.min(pointRadius / videoWidth, 1.0))
    // }


    function mousePressed(mouseX, mouseY) {
        _dragStartX = mouseX
        _dragStartY = mouseY
        _dragging = true
    }

    function mouseMoving(mouseX, mouseY){
        _currentX = mouseX
        _currentY = mouseY

        if (_dragging) {
            let absX = Math.abs(_currentX - _dragStartX)
            let absY = Math.abs(_currentY - _dragStartY)
            if(!_zooming &&
                    (absX > _threshold ||
                     absY > _threshold)
                    ) {
                _zooming = true
            }
        }
    }

    function zoom(mouseX, mouseY){
        if (videoWidth <= 0 || videoHeight <= 0) {
            return
        }

        if (videoWidth <= 0 || videoHeight <= 0) {
            return
        }

        // Order coordinates: top-left and bottom-right
        var x0 = zoomRect.x
        var x1 = zoomRect.x + zoomRect.width
        var y0 = zoomRect.y
        var y1 = zoomRect.y + zoomRect.height

        // Letterbox margins (black bars when aspect ratio doesn't match)
        var marginH = (width - videoWidth) / 2
        var marginV = (height - videoHeight) / 2

        // Convert from view coordinates to video-relative coordinates
        x0 = x0 - marginH
        x1 = x1 - marginH
        y0 = y0 - marginV
        y1 = y1 - marginV

        // Normalize to 0..1
        x0 = Math.max(Math.min(x0 / videoWidth, 1.0), 0.0)
        x1 = Math.max(Math.min(x1 / videoWidth, 1.0), 0.0)
        y0 = Math.max(Math.min(y0 / videoHeight, 1.0), 0.0)
        y1 = Math.max(Math.min(y1 / videoHeight, 1.0), 0.0)

        var w = x1 - x0
        var h = y1 - y0

        // Ignore degenerate rectangles (e.g. mostly-horizontal/vertical drags)
        if (w < 0.01 || h < 0.01) {
            return
        }

        // Drag = rectangle tracking
        camera.startTracking(Qt.rect(x0, y0, w, h), true)
    }


    function mouseReleased(mouseX, mouseY){
        if(_zooming){
            zoom(mouseX, mouseY)
        } else {
            track(mouseX, mouseY)
        }
        _dragging = false
        _zooming = false
    }

    function wheelUsed(wheel){
        _trackSize = Math.max(10, Math.min(1000, _trackSize + wheel.angleDelta.y * 0.1))
    }

    function track(mouseX, mouseY) {
        if (!_canTrackRect) {
            return
        }
        if (videoWidth <= 0 || videoHeight <= 0) {
            return
        }

        // Order coordinates: top-left and bottom-right
        var x0 = trackRect.x
        var x1 = trackRect.x + trackRect.width
        var y0 = trackRect.y
        var y1 = trackRect.y + trackRect.height

        // Letterbox margins (black bars when aspect ratio doesn't match)
        var marginH = (width - videoWidth) / 2
        var marginV = (height - videoHeight) / 2

        // Convert from view coordinates to video-relative coordinates
        x0 = x0 - marginH
        x1 = x1 - marginH
        y0 = y0 - marginV
        y1 = y1 - marginV

        // Normalize to 0..1
        x0 = Math.max(Math.min(x0 / videoWidth, 1.0), 0.0)
        x1 = Math.max(Math.min(x1 / videoWidth, 1.0), 0.0)
        y0 = Math.max(Math.min(y0 / videoHeight, 1.0), 0.0)
        y1 = Math.max(Math.min(y1 / videoHeight, 1.0), 0.0)

        var w = x1 - x0
        var h = y1 - y0

        // Ignore degenerate rectangles (e.g. mostly-horizontal/vertical drags)
        if (w < 0.01 || h < 0.01) {
            return
        }

        // Drag = rectangle tracking
        camera.startTracking(Qt.rect(x0, y0, w, h), false)
    }

    // --- ROI selection overlay (green rectangle while dragging) ---
    Rectangle {
        id: zoomRect
        visible: _zooming
        color: Qt.rgba(0.1, 0.85, 0.1, 0.15)
        border.color: "green"
        border.width: 1
        width: Math.abs(_currentX - _dragStartX)
        x: Math.min(_dragStartX, _currentX)
        y: Math.min(_dragStartY, _currentY)
        height: Math.abs(_currentY - _dragStartY)
    }

    // --- Tracking selection (draw at the center of the cursor,scrolling change the tracking size)
    Rectangle{
        id: trackRect
        visible: _trackingEnabled && !_zooming
        color: "transparent"
        border.color: "green"
        border.width: 2
        width:_trackSize
        height: _trackSize
        x: _currentX - width / 2
        y: _currentY - height / 2
    }

    // --- Tracking status overlay (red rectangle/circle from camera feedback) ---
    // Coordinates are normalized to the video frame (0.0–1.0). Convert to screen
    // pixels by scaling by the displayed video size and offsetting by letterbox margins.
    readonly property bool _trackingActive: _trackingEnabled && camera.trackingImageIsActive
    readonly property bool _isPoint: _trackingActive && camera.trackingImageIsPoint
    readonly property real _marginH: (rootItem.width - videoWidth) / 2
    readonly property real _marginV: (rootItem.height - videoHeight) / 2

    // Rectangle {
    //     id: trackingStatusOverlay
    //     color: "transparent"
    //     border.color: "red"
    //     border.width: 5
    //     radius: rootItem._isPoint ? width / 2 : 5
    //     visible: rootItem._trackingActive

    //     x: rootItem._trackingActive
    //        ? (rootItem._isPoint ? rootItem._marginH + videoWidth  * (camera.trackingImagePoint.x - camera.trackingImageRadius)
    //                             : rootItem._marginH + videoWidth  * camera.trackingImageRect.x)
    //        : 0
    //     y: rootItem._trackingActive
    //        ? (rootItem._isPoint ? rootItem._marginV + videoHeight * (camera.trackingImagePoint.y - camera.trackingImageRadius)
    //                             : rootItem._marginV + videoHeight * camera.trackingImageRect.y)
    //        : 0
    //     width: rootItem._trackingActive
    //            ? (rootItem._isPoint ? videoWidth * camera.trackingImageRadius * 2
    //                                 : videoWidth * camera.trackingImageRect.width)
    //            : 0
    //     height: rootItem._trackingActive
    //             ? (rootItem._isPoint ? width
    //                                  : videoHeight * camera.trackingImageRect.height)
    //             : 0
    // }
}
