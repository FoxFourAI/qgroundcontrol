import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import Custom.Widgets

Item {
    required property var guidedValueSlider

    id: control
    width: parent.width
    height: ScreenTools.toolbarHeight

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property color _mainStatusBGColor: qgcPal.brandingPurple
    property real _leftRightMargin: ScreenTools.defaultFontPixelWidth * 0.75
    property var _guidedController: globals.guidedControllerFlyView

    function dropMainStatusIndicatorTool() {
        mainStatusIndicator.dropMainStatusIndicator()
    }

    QGCPalette {
        id: qgcPal
    }

    QGCFlickable {
        anchors.fill: parent

        contentWidth: toolBarLayout.width
        flickableDirection: Flickable.HorizontalFlick

        Row {
            id: toolBarLayout
            height: parent.height
            spacing: 0

            Item {
                id: leftPanel
                width: leftPanelLayout.implicitWidth
                height: parent.height

                // Gradient background behind Q button and main status indicator
                Rectangle {
                    id: gradientBackground
                    height: parent.height
                    width: mainStatusLayout.width

                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop {
                            position: 0
                            color: _mainStatusBGColor
                        }
                        //GradientStop { position: qgcButton.x + qgcButton.width; color: _mainStatusBGColor }
                        GradientStop {
                            position: 1
                            color: qgcPal.windowTransparent
                        }
                    }
                }

                // Standard toolbar background to the right of the gradient
                Rectangle {
                    anchors.left: gradientBackground.right
                    anchors.right: parent.right
                    height: parent.height
                    color: qgcPal.windowTransparent
                }

                RowLayout {
                    id: leftPanelLayout
                    height: parent.height
                    spacing: ScreenTools.defaultFontPixelWidth * 2

                    RowLayout {
                        id: mainStatusLayout
                        height: parent.height
                        spacing: 0

                        QGCToolBarButton {
                            id: qgcButton2
                            objectName: "toolbar_qgcLogo"
                            height: parent.height
                            icon.source: "/res/QGCLogoFull.svg"
                            logo: true
                            onClicked: mainWindow.showToolSelectDialog()
                        }

                        MainStatusIndicator {
                            id: mainStatusIndicator
                            objectName: "toolbar_mainStatusIndicator"
                            Layout.fillHeight: true
                        }
                    }

                    QGCButton {
                        id: disconnectButton
                        text: qsTr("Disconnect")
                        onClicked: _activeVehicle.closeVehicle()
                        visible: _activeVehicle && _communicationLost
                    }

                    FlightModeIndicator {
                        objectName: "toolbar_flightModeIndicator"
                        Layout.fillHeight: true
                        visible: _activeVehicle
                    }
                }
            }

            Item {
                id: centerPanel
                width: Math.max(
                           flyViewIndicators.width,
                           control.width - (leftPanel.width) - (rightPanel.width))
                height: parent.height

                Rectangle {
                    anchors.fill: parent
                    color: qgcPal.windowTransparent
                }

                FlyViewToolBarIndicators {
                    id: flyViewIndicators
                    height: parent.height

                }
            }



            Item {
                id: rightPanel
                // center panel takes up all remaining space in toolbar between left and right panels
                width: f4ControlPage.width + ScreenTools.defaultFontPixelWidth * 2
                height: parent.height

                Rectangle {
                    anchors.fill: parent
                    color: qgcPal.windowTransparent
                }

                FoxFourControlPage {
                    id: f4ControlPage
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: ScreenTools.defaultFontPixelHeight / 2
                }
            }


            // }
        }
    }

    Rectangle{
    width: guidedActionConfirm.width + _margins * 3
    height: guidedActionConfirm.height + _margins * 2
    radius: ScreenTools.defaultBorderRadius
    anchors.centerIn:guidedActionConfirm
    color: qgcPal.windowTransparent
    visible: guidedActionConfirm.visible
    }
    GuidedActionConfirm {
        id: guidedActionConfirm
        height: parent.height
        anchors.top: parent.bottom
        anchors.topMargin: _margins * 2
        anchors.horizontalCenter: parent.horizontalCenter
        guidedController: control._guidedController
        guidedValueSlider: control.guidedValueSlider
        messageDisplay: guidedActionMessageDisplay
    }

    // The guided action message display is outside of the GuidedActionConfirm control so that it doesn't end up as
    // part of the Flickable
    Rectangle {
        id: guidedActionMessageDisplay
        anchors.top: guidedActionConfirm.bottom
        anchors.topMargin: _margins
        anchors.horizontalCenter: parent.horizontalCenter
        width: messageLabel.contentWidth + (_margins * 2)
        height: messageLabel.contentHeight + (_margins * 2)
        color: qgcPal.windowTransparent
        radius: ScreenTools.defaultBorderRadius
        visible: guidedActionConfirm.visible

        QGCLabel {
            id: messageLabel
            x: _margins
            y: _margins
            width: ScreenTools.defaultFontPixelWidth * 30
            wrapMode: Text.WordWrap
            text: guidedActionConfirm.message
        }

        PropertyAnimation {
            id: messageOpacityAnimation
            target: guidedActionMessageDisplay
            property: "opacity"
            from: 1
            to: 0
            duration: 500
        }

        Timer {
            id: messageFadeTimer
            interval: 4000
            onTriggered: messageOpacityAnimation.start()
        }
    }

    ParameterDownloadProgress {
        anchors.fill: parent
    }
}