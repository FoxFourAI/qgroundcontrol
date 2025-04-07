/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Controllers
import QGroundControl.ScreenTools

Item {
    id: _root
    visible: QGroundControl.videoManager.hasVideo

    property int _track_rec_x: 0
    property int _track_rec_y: 0
    property Item pipState: videoPipState
    property real last_zoom_factor: 1
    property real last_zoom_center_x: 0
    property real last_zoom_center_y: 0
    property var lastZoomRectangle: null  // Store the last used rectangle for zooming
    QGCPipState {
        id: videoPipState
        pipOverlay: _pipOverlay
        isDark: true

        onWindowAboutToOpen: {
            QGroundControl.videoManager.stopVideo();
            videoStartDelay.start();
        }

        onWindowAboutToClose: {
            QGroundControl.videoManager.stopVideo();
            videoStartDelay.start();
        }

        onStateChanged: {
            if (pipState.state !== pipState.fullState) {
                QGroundControl.videoManager.fullScreen = false;
            }
        }
    }

    Timer {
        id: videoStartDelay
        interval: 2000
        running: false
        repeat: false
        onTriggered: QGroundControl.videoManager.startVideo()
    }

    //-- Video Streaming
    FlightDisplayViewVideo {
        id: videoStreaming
        anchors.fill: parent
        useSmallFont: _root.pipState.state !== _root.pipState.fullState
        visible: QGroundControl.videoManager.isGStreamer
    }
    //-- UVC Video (USB Camera or Video Device)
    Loader {
        id: cameraLoader
        anchors.fill: parent
        visible: !QGroundControl.videoManager.isGStreamer
        source: QGroundControl.videoManager.uvcEnabled ? "qrc:/qml/FlightDisplayViewUVC.qml" : "qrc:/qml/FlightDisplayViewDummy.qml"
    }

    QGCLabel {
        text: qsTr("Double-click to exit full screen")
        font.pointSize: ScreenTools.largeFontPointSize
        visible: QGroundControl.videoManager.fullScreen && flyViewVideoMouseArea.containsMouse
        anchors.centerIn: parent

        onVisibleChanged: {
            if (visible) {
                labelAnimation.start();
            }
        }

        PropertyAnimation on opacity {
            id: labelAnimation
            duration: 10000
            from: 1.0
            to: 0.0
            easing.type: Easing.InExpo
        }
    }

    // Define the selectable rectangle (initially hidden)
    Rectangle {
        id: selectionRect
        color: "transparent"
        border.color: "blue"
        border.width: 3
        visible: false // Initially hidden

        // Make sure selection rectangle doesn't go negative in size
        x: Math.min(startX, mouseArea.mouseX)
        y: Math.min(startY, mouseArea.mouseY)
        width: Math.abs(mouseArea.mouseX - startX)
        height: Math.abs(mouseArea.mouseY - startY)
    }

    MouseArea {
        id: flyViewVideoMouseArea
        anchors.fill: parent
        enabled: pipState.state === pipState.fullState
        hoverEnabled: true
        scrollGestureEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        property double x0: 0
        property double x1: 0
        property double y0: 0
        property double y1: 0
        property double offset_x: 0
        property double offset_y: 0
        property double radius: 20
        property real startX: 0
        property real startY: 0
        property var trackingStatus: trackingStatusComponent.createObject(flyViewVideoMouseArea, {})

        // Define constants representing rectangle scaling modes
        readonly property int scaleModeHorizontal: 0
        readonly property int scaleModeVertical: 1
        readonly property int scaleModeBoth: 2
        readonly property int numScaleModes: 3

        property int currentScaleMode: scaleModeBoth

        onPressed: {
            // Set starting point and make the selection rectangle visible
            startX = mouse.x;
            startY = mouse.y;
            selectionRect.visible = true;
            selectionRect.x = Math.min(mouse.x, startX);
            selectionRect.y = Math.min(mouse.y, startY);
            selectionRect.width = Math.abs(mouse.x - startX);
            selectionRect.height = Math.abs(mouse.y - startY);
        }

        onReleased: mouse => {
            // TODO: zooming inside the zoom window
            // TODO: replace hardcoding
            // let MAX_ZOOM_FACTOR = 12;
            // let VID_ZOOM_SIZE_X = 0.45;
            // let VID_ZOOM_SIZE_Y = 0.3;
            // let CENTER_X_OVERLAY = 0.5;
            // let CENTER_Y_OVERLAY = 0.16;
            let MAX_ZOOM_FACTOR = videoStreaming._camera ? videoStreaming._camera.maxZoomLevel : 12;
            let VID_ZOOM_SIZE_X = QGroundControl.settingsManager.videoSettings.pipZoomSizeX.rawValue;
            let VID_ZOOM_SIZE_Y = QGroundControl.settingsManager.videoSettings.pipZoomSizeY.rawValue;
            let CENTER_X_OVERLAY = QGroundControl.settingsManager.videoSettings.pipCenterX.rawValue;
            let CENTER_Y_OVERLAY = QGroundControl.settingsManager.videoSettings.pipCenterY.rawValue;
            console.log("MAX_ZOOM_FACTOR: ", MAX_ZOOM_FACTOR);
            console.log("VID_ZOOM_SIZE_X: ", VID_ZOOM_SIZE_X);
            console.log("VID_ZOOM_SIZE_Y: ", VID_ZOOM_SIZE_Y);
            console.log("CENTER_X_OVERLAY: ", CENTER_X_OVERLAY);
            console.log("CENTER_Y_OVERLAY: ", CENTER_Y_OVERLAY);
            selectionRect.visible = false;

            let x0 = Math.floor(Math.max(0, selectionRect.x));
            let y0 = Math.floor(Math.max(0, selectionRect.y));
            let x1 = Math.floor(Math.min(width, selectionRect.x + selectionRect.width));
            let y1 = Math.floor(Math.min(height, selectionRect.y + selectionRect.height));

            if (selectionRect.width < 15 || selectionRect.height < 15) {
                onClicked(mouse);
                return;
            }

            //calculate offset between video stream rect and background (black stripes)
            let offset_x = (parent.width - videoStreaming.getWidth()) / 2;
            let offset_y = (parent.height - videoStreaming.getHeight()) / 2;
            //calculate offset between video stream rect and background (black stripes)
            x0 -= offset_x;
            x1 -= offset_x;
            y0 -= offset_y;
            y1 -= offset_y;
            //convert absolute to relative coordinates and limit range to 0...1
            x0 = Math.max(Math.min(x0 / videoStreaming.getWidth(), 1.0), 0.0);
            x1 = Math.max(Math.min(x1 / videoStreaming.getWidth(), 1.0), 0.0);
            y0 = Math.max(Math.min(y0 / videoStreaming.getHeight(), 1.0), 0.0);
            y1 = Math.max(Math.min(y1 / videoStreaming.getHeight(), 1.0), 0.0);
            
            let factor = Math.min(1 / (x1 - x0) * VID_ZOOM_SIZE_X, 1 / (y1 - y0) * VID_ZOOM_SIZE_Y);

            let rec = Qt.rect(x0, y0, x1 - x0, y1 - y0);
            if (rec.width === 0 || rec.height === 0) {
                return;
            }


            let new_center_x = (x0 + x1) / 2;
            let new_center_y = (y0 + y1) / 2;

            // pc - picture in picture center
            // tl - top left corner of picture in picture region
            let pc = {
                x: CENTER_X_OVERLAY  // TODO clamp
                ,
                y: CENTER_Y_OVERLAY
            };
            let tl = {
                x: CENTER_X_OVERLAY - VID_ZOOM_SIZE_X / 2    // TODO clamp
                ,
                y: CENTER_Y_OVERLAY - VID_ZOOM_SIZE_Y / 2
            };

            let pip_overlap_zone = Qt.rect(tl.x, tl.y, VID_ZOOM_SIZE_X, VID_ZOOM_SIZE_Y);
            if (last_zoom_factor > 1) {
                // check if the selection was done inside the region or outside it.
                if (new_center_x >= tl.x && new_center_x <= tl.x + VID_ZOOM_SIZE_X && new_center_y >= tl.y && new_center_y <= tl.y + VID_ZOOM_SIZE_Y) {
                    console.log("zoom center is inside PIP");
                    console.log("old factor: ", factor);
                    console.log("old rec:", x0, x1, y0, y1);
                    new_center_x = last_zoom_center_x + (new_center_x - pc.x) / last_zoom_factor;
                    new_center_y = last_zoom_center_y + (new_center_y - pc.y) / last_zoom_factor;

                    let new_top_left_x = last_zoom_center_x + (x0 - pc.x) / last_zoom_factor;
                    let new_top_left_y = last_zoom_center_y + (y0 - pc.y) / last_zoom_factor;
                    let new_bottom_right_x = last_zoom_center_x + (x1 - pc.x) / last_zoom_factor;
                    let new_bottom_right_y = last_zoom_center_y + (y1 - pc.y) / last_zoom_factor;

                    factor = Math.min(1.0 / ((new_bottom_right_x - new_top_left_x) / VID_ZOOM_SIZE_X), 1.0 / ((new_bottom_right_y - new_top_left_y) / VID_ZOOM_SIZE_Y));
                    factor = Math.min(MAX_ZOOM_FACTOR, factor);
                    console.log("new factor: ", factor);
                    console.log("new rec:", new_top_left_x, new_bottom_right_x, new_top_left_y, new_bottom_right_y);
                    rec = Qt.rect(new_top_left_x, new_top_left_y, new_bottom_right_x - new_top_left_x, new_bottom_right_y - new_top_left_y);
                } else {
                    console.log("zoom center is outside PIP");
                }
            }
            last_zoom_center_x = rec.x + rec.width / 2
            last_zoom_center_y = rec.y + rec.height / 2
            last_zoom_factor = factor
            
            let latestFrameTimestamp = QGroundControl.videoManager.lastKlvTimestamp;
            //videoStreaming._camera.startTracking(rec, latestFrameTimestamp, true)
            videoStreaming._camera.setZoomParams(factor.toFixed(1), rec, latestFrameTimestamp);
            videoStreaming.updateZoom(factor.toFixed(2));
            videoStreaming._camera.zoomEnabled = true;
            lastZoomRectangle = rec;  // Update the last used rectangle
        }

        onDoubleClicked: QGroundControl.videoManager.fullScreen = !QGroundControl.videoManager.fullScreen

        function calculateTrackingCoordinates(targetCanvas, videoStreaming) {
            let x0 = Math.floor(Math.max(0, targetCanvas.rectCenterX - targetCanvas.rectWidth / 2));
            let y0 = Math.floor(Math.max(0, targetCanvas.rectCenterY - targetCanvas.rectHeight / 2));
            let x1 = Math.floor(Math.min(width, targetCanvas.rectCenterX + targetCanvas.rectWidth / 2));
            let y1 = Math.floor(Math.min(height, targetCanvas.rectCenterY + targetCanvas.rectHeight / 2));

            //calculate offset between video stream rect and background (black stripes)
            let offset_x = (parent.width - videoStreaming.getWidth()) / 2;
            let offset_y = (parent.height - videoStreaming.getHeight()) / 2;

            //calculate offset between video stream rect and background (black stripes)
            x0 -= offset_x;
            x1 -= offset_x;
            y0 -= offset_y;
            y1 -= offset_y;

            //convert absolute to relative coordinates and limit range to 0...1
            x0 = Math.max(Math.min(x0 / videoStreaming.getWidth(), 1.0), 0.0);
            x1 = Math.max(Math.min(x1 / videoStreaming.getWidth(), 1.0), 0.0);
            y0 = Math.max(Math.min(y0 / videoStreaming.getHeight(), 1.0), 0.0);
            y1 = Math.max(Math.min(y1 / videoStreaming.getHeight(), 1.0), 0.0);

            return {
                x0: x0,
                y0: y0,
                x1: x1,
                y1: y1
            };
        }

        onClicked: mouse => {
            // If right button is clicked, change the selection mode
            if (mouse.button == Qt.RightButton) {
                // If mode reached the highest value, put it back to zero
                // currentScaleMode = (currentScaleMode + 1) % numScaleModes;
                return;
            }

            if (!videoStreaming._camera || !videoStreaming._camera.trackingEnabled) {
                return;
            }

            let coords = calculateTrackingCoordinates(targetCanvas, videoStreaming);

            //use point message if rectangle is very small
            if (targetCanvas.rectWidth < 1 && targetCanvas.rectHeight < 1) {
                let pt = Qt.point(targetCanvas.rectCenterX, targetCanvas.rectCenterY);
                videoStreaming._camera.startTracking(pt, targetCanvas.rectWidth / 2);
            } else {
                let latestFrameTimestamp = QGroundControl.videoManager.lastKlvTimestamp;
                let rec = Qt.rect(coords.x0, coords.y0, coords.x1 - coords.x0, coords.y1 - coords.y0);
                videoStreaming._camera.startTracking(rec, latestFrameTimestamp);
            }
            // videoStreaming._camera._requestTrackingStatus()
        }

        onWheel: wheel => {
            const WHEEL_DIVIDER = 19;

            console.log(wheel);
            console.log(wheel.angleDelta);
            if (currentScaleMode == scaleModeHorizontal || currentScaleMode == scaleModeBoth) {
                targetCanvas.rectWidth = Math.max(0, targetCanvas.rectWidth + Math.floor(Number(wheel.angleDelta.y) / WHEEL_DIVIDER));
            }
            if (currentScaleMode == scaleModeVertical || currentScaleMode == scaleModeBoth) {
                targetCanvas.rectHeight = Math.max(0, targetCanvas.rectHeight + Math.floor(Number(wheel.angleDelta.y) / WHEEL_DIVIDER));
            }

            targetCanvas.requestPaint();
        }

        onPositionChanged: mouse => {
            targetCanvas.rectCenterX = mouse.x;
            targetCanvas.rectCenterY = mouse.y;
            targetCanvas.requestPaint();
            // Redraw selection rectangle as mouse moves
            if (flyViewVideoMouseArea.pressed) {
                selectionRect.x = Math.min(mouse.x, startX);
                selectionRect.y = Math.min(mouse.y, startY);
                selectionRect.width = Math.abs(mouse.x - startX);
                selectionRect.height = Math.abs(mouse.y - startY);
            }
        }

        Canvas {
            id: targetCanvas
            property int rectCenterX
            property int rectCenterY

            property int rectWidth: 100
            property int rectHeight: 100

            anchors.fill: parent
            onPaint: {
                const BORDER_WIDTH = 3;
                const BORDER_COLOR = '#11BB11';

                var ctx = getContext("2d");
                ctx.reset();

                if (!videoStreaming._camera || !videoStreaming._camera.trackingEnabled) {
                    return;
                }

                let rectStartX = Math.floor(Math.max(0, rectCenterX - rectWidth / 2));
                let rectStartY = Math.floor(Math.max(0, rectCenterY - rectHeight / 2));
                let rectEndX = Math.floor(Math.min(width, rectCenterX + rectWidth / 2));
                let rectEndY = Math.floor(Math.min(height, rectCenterY + rectHeight / 2));

                ctx.strokeStyle = BORDER_COLOR;
                ctx.lineWidth = BORDER_WIDTH;
                ctx.beginPath();
                ctx.moveTo(rectStartX, rectStartY);
                ctx.lineTo(rectStartX, rectEndY);
                ctx.lineTo(rectEndX, rectEndY);
                ctx.lineTo(rectEndX, rectStartY);
                ctx.lineTo(rectStartX, rectStartY);
                ctx.stroke();
            }
        }

        Component {
            id: trackingStatusComponent

            Rectangle {
                color: "transparent"
                border.color: "red"
                border.width: 5
                radius: 5
            }
        }

        Timer {
            id: trackingStatusTimer
            interval: 50
            repeat: true
            running: true
            onTriggered: {
                if (videoStreaming._camera) {
                    if (videoStreaming._camera.trackingEnabled && videoStreaming._camera.trackingImageStatus) {
                        var margin_hor = (parent.parent.width - videoStreaming.getWidth()) / 2;
                        var margin_ver = (parent.parent.height - videoStreaming.getHeight()) / 2;
                        var left = margin_hor + videoStreaming.getWidth() * videoStreaming._camera.trackingImageRect.left;
                        var top = margin_ver + videoStreaming.getHeight() * videoStreaming._camera.trackingImageRect.top;
                        var right = margin_hor + videoStreaming.getWidth() * videoStreaming._camera.trackingImageRect.right;
                        var bottom = margin_ver + !isNaN(videoStreaming._camera.trackingImageRect.bottom) ? videoStreaming.getHeight() * videoStreaming._camera.trackingImageRect.bottom : top + (right - left);
                        var width = right - left;
                        var height = bottom - top;

                        flyViewVideoMouseArea.trackingStatus.x = left;
                        flyViewVideoMouseArea.trackingStatus.y = top;
                        flyViewVideoMouseArea.trackingStatus.width = width;
                        flyViewVideoMouseArea.trackingStatus.height = height;
                    } else {
                        flyViewVideoMouseArea.trackingStatus.x = 0;
                        flyViewVideoMouseArea.trackingStatus.y = 0;
                        flyViewVideoMouseArea.trackingStatus.width = 0;
                        flyViewVideoMouseArea.trackingStatus.height = 0;
                    }
                }
            }
        }
    }

    ProximityRadarVideoView {
        anchors.fill: parent
        vehicle: QGroundControl.multiVehicleManager.activeVehicle
    }

    ObstacleDistanceOverlayVideo {
        id: obstacleDistance
        showText: pipState.state === pipState.fullState
    }
}
