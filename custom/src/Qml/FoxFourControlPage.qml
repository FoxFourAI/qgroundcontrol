import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id:         control
    width: 70

    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property real   _spacing:           ScreenTools.defaultFontPixelWidth / 2
    property var    _settingsManager:           QGroundControl.settingsManager
    property var    _videoSettings:             _settingsManager.videoSettings
    property var    _flyViewSettings:           _settingsManager.flyViewSettings
    property var    _appSettings:               _settingsManager.appSettings



    Image {
        id:foxFourLogo
        anchors.fill: parent
        source:                 _outdoorPalette ? "/custom/img/FoxFourTextLogo_dark.svg" : "/custom/img/FoxFourTextLogo_light.svg"
        mipmap:                 true
        fillMode:               Image.PreserveAspectFit
        property bool   _outdoorPalette:        qgcPal.globalTheme === QGCPalette.Light
        QGCMouseArea{
            anchors.fill: parent
            onClicked: mainWindow.showIndicatorDrawer(foxFourControlPanel,foxFourLogo)
        }
    }

    Component{
        id: foxFourControlPanel
        ToolIndicatorPage{
            showExpand: true
            contentComponent:controlPanelComponent
            expandedComponent:extendedComponent
        }
    }

    Component{
        id: extendedComponent
        SettingsGroupLayout{
            spacing: 0
            id: shortcutGroup
            heading: qsTr("Shortcuts")
            Repeater{
                model: globalShortcuts
                delegate:LabelledLabel{
                    label:description
                    labelText: sequence
                }
            }
        }
    }

    Component{
        id: controlPanelComponent

        ColumnLayout{
            id:     mainLayout
            spacing: _spacing

            QGCLabel{
                font.pointSize: ScreenTools.largeFontPointSize
                text: qsTr("FoxFour Configuration")
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            SettingsGroupLayout{
                heading: qsTr("General")
                FactCheckBoxSlider{
                    Layout.fillWidth: true
                    text:               qsTr("Minimal mode")
                    fact:               _minimalMode
                    property Fact       _minimalMode: _flyViewSettings.minimalMode
                    ToolTip{
                        visible:parent.hovered
                        text: qsTr("Will pull ONLY mandatory parameters when connecting to the vehicle.")
                    }
                }

                LabelledFactTextField {
                    Layout.fillWidth:   true
                    label:               qsTr("TRACKING_STATUS rate")
                    fact:               _flyViewSettings.trackingRate
                    textField.numericValuesOnly: true
                    textFieldShowUnits:          true
                    textFieldUnitsLabel:         qsTr("Hz")
                    visible:            true
                    ToolTip{
                        visible:parent.hovered
                        text: qsTr("The interval of the TRACKING_STATUS message, when tracking an object.")
                    }
                }

                FactCheckBoxSlider {
                    text:       fact.shortDescription
                    Layout.fillWidth: true
                    fact:       _appSettings.cacheParameters
                    visible:    fact.visible
                    ToolTip{
                        visible: parent.hovered
                        text: qsTr("Cache vehicle parameters using UID of the FCU.")
                    }
                }
            }

            SettingsGroupLayout{
                heading: qsTr("Video")

                FactCheckBoxSlider{
                    Layout.fillWidth: true
                    id: autoConfigure
                    text: "Auto configurate stream"
                    // visible: _videoAutoStreamConfig
                    fact: _videoSettings.autoConfigure
                    ToolTip{
                        visible:parent.hovered
                        text: qsTr("QGC will detect the stream configurations and apply changes automatically (stream should support this feature)")
                    }
                }
            }

            SettingsGroupLayout{
                heading: qsTr("Map")

                FactCheckBoxSlider {
                    Layout.fillWidth:   true
                    text:               qsTr("Show GPS_RAW_INT trajectory")
                    fact:               _showGPSrawTrajectory
                    visible:            _showGPSrawTrajectory.visible

                    property Fact   _showGPSrawTrajectory: _flyViewSettings.showGPSrawTrajectory
                    ToolTip{
                        visible:parent.hovered
                        text: qsTr("Draw GPS path on the map.")
                    }
                }

                LabelledFactTextField{
                    Layout.fillWidth:  true
                    label:              qsTr("Map-matching dots amount")
                    fact:               _flyViewSettings.mapMatchingPointsCnt
                    textField.numericValuesOnly: true
                }
            }

            SettingsGroupLayout{
                heading: qsTr("VGM")

                FactCheckBoxSlider {
                    Layout.fillWidth:   true
                    text:               qsTr("Enable dialect")
                    fact:               _enableVGMDiaclect
                    visible:            _enableVGMDiaclect.visible

                    property Fact   _enableVGMDiaclect: _flyViewSettings.enableVGMDialect
                }
            }
        }
    }
}

