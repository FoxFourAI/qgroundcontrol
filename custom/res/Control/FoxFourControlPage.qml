import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id: control
    width: 70

    property real _margins: ScreenTools.defaultFontPixelWidth
    property real _spacing: ScreenTools.defaultFontPixelWidth / 2
    property var _settingsManager: QGroundControl.settingsManager
    property var _settings: _settingsManager.foxFourSettings
    property var _autoPilot: globals.activeVehicle.autopilotPlugin

    Image {
        id: foxFourLogo
        anchors.fill: parent
        source: _outdoorPalette ? "/custom/img/FoxFourTextLogo_dark.svg" : "/custom/img/FoxFourTextLogo_light.svg"
        mipmap: true
        fillMode: Image.PreserveAspectFit
        property bool _outdoorPalette: qgcPal.globalTheme === QGCPalette.Light
        QGCMouseArea {
            anchors.fill: parent
            onClicked: mainWindow.showIndicatorDrawer(foxFourControlPanel,
                                                      foxFourLogo)
        }
    }

    Component {
        id: foxFourControlPanel
        ToolIndicatorPage {
            showExpand: true
            contentComponent: controlPanelComponent
            expandedComponent: extendedComponent
        }
    }

    Component {
        id: extendedComponent
        SettingsGroupLayout {
            spacing: 0
            id: shortcutGroup
            heading: qsTr("Shortcuts")
            Repeater {
                model: globalShortcuts
                delegate: LabelledLabel {
                    label: description
                    labelText: sequence
                }
            }
        }
    }

    Component {
        id: controlPanelComponent

        ColumnLayout {
            id: mainLayout
            spacing: _spacing

            QGCLabel {
                font.pointSize: ScreenTools.largeFontPointSize
                text: qsTr("FoxFour Configuration")
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            SettingsGroupLayout {
                heading: qsTr("General")
                FactCheckBoxSlider {
                    Layout.fillWidth: true
                    fact: control._settings.minimalMode
                    text: fact.label

                    ToolTip {
                        visible: parent.hovered
                        text: parent.fact.shortDescription
                    }
                }
                FactCheckBoxSlider {
                    Layout.fillWidth: true
                    fact: control._settings.directVGM
                    text: fact.label
                    ToolTip {
                        visible: parent.hovered
                        text: parent.fact.shortDescription
                    }
                }

                LabelledFactTextField {
                    Layout.fillWidth: true
                    label: fact.label
                    fact: control._settings.trackingRate
                    textField.numericValuesOnly: true
                    textFieldShowUnits: true
                    textFieldUnitsLabel: qsTr("Hz")
                    visible: true
                }
                // FactCheckBoxSlider {
                //     text: fact.label
                //     Layout.fillWidth: true
                //     fact: control._settings.cacheVehicleParameters

                //     ToolTip {
                //         visible: parent.hovered
                //         text: parent.fact.shortDescription
                //     }
                // }
            }

            SettingsGroupLayout {
                heading: qsTr("Video")

                FactCheckBoxSlider {
                    Layout.fillWidth: true
                    text: fact.label
                    fact: control._settings.autoConfigureStream

                    ToolTip {
                        visible: parent.hovered
                        text: parent.fact.shortDescription
                    }
                }

                SettingsGroupLayout {
                    id: hudGroup
                    property bool showFull: control._settings.hudVisible.value
                    heading: qsTr("HUD")
                    FactCheckBoxSlider{
                        Layout.fillWidth: true
                        text: fact.label
                        fact: control._settings.hudVisible
                    }
                    LabelledFactTextField {
                        visible: hudGroup.showFull
                        Layout.fillWidth: true
                        label: fact.label
                        fact: control._settings.hudColor
                    }
                    RowLayout{
                        visible: hudGroup.showFull
                        Layout.fillWidth: true
                        QGCLabel{
                            Layout.fillWidth: true
                            text: "Opacity"
                        }
                        QGCSlider{
                            Layout.fillWidth: true
                            from:0
                            to:1
                            stepSize: 0.1
                            Component.onCompleted: value = _settings.hudOpacity.value
                            onValueChanged: _settings.hudOpacity.value = value
                        }
                    }

                    FactCheckBoxSlider {
                        visible: hudGroup.showFull
                        Layout.fillWidth: true
                        text: fact.label
                        fact: control._settings.hudShadow
                    }
                    FactCheckBoxSlider {
                        visible: hudGroup.showFull
                        Layout.fillWidth: true
                        text: fact.label
                        fact: control._settings.hudCompass
                    }
                    FactCheckBoxSlider {
                        visible: hudGroup.showFull
                        Layout.fillWidth: true
                        text: fact.label
                        fact: control._settings.hudRoll
                    }
                    FactCheckBoxSlider {
                        visible: hudGroup.showFull
                        Layout.fillWidth: true
                        text: fact.label
                        fact: control._settings.hudSpeed
                    }
                    FactCheckBoxSlider {
                        visible: hudGroup.showFull
                        Layout.fillWidth: true
                        text: fact.label
                        fact: control._settings.hudAltitude
                    }
                    FactCheckBoxSlider {
                        visible: hudGroup.showFull
                        Layout.fillWidth: true
                        text: fact.label
                        fact: control._settings.hudHorizon
                    }
                }
            }

            SettingsGroupLayout {
                heading: qsTr("Map")

                FactCheckBoxSlider {
                    Layout.fillWidth: true
                    text: fact.label
                    fact: control._settings.showGPSTrajectory
                }

                FactCheckBoxSlider {
                    Layout.fillWidth: true
                    text: fact.label
                    fact: control._settings.disableVehicleTracking
                }

                LabelledFactTextField {
                    Layout.fillWidth: true
                    label: fact.label
                    fact: control._settings.mapMatchingPointsCnt
                    textField.numericValuesOnly: true
                }


                SettingsGroupLayout {
                    heading: qsTr("Detections")
                    FactCheckBoxSlider {
                        Layout.fillWidth: true
                        text: fact.label
                        fact: control._settings.showDetections
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("Clear")
                        onClicked: _autoPilot.dialectHandler.clearDetections()
                    }
                }

            }

            SettingsGroupLayout {
                heading: qsTr("VGM")

                FactCheckBoxSlider {
                    Layout.fillWidth: true
                    text: fact.label
                    fact: control._settings.enableVGMDialect

                    ToolTip {
                        visible: parent.hovered
                        text: parent.fact.shortDescription
                    }
                }
            }
        }
    }
}
