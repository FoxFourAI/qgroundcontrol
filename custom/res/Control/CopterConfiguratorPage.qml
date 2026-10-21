import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import FoxFour.Widgets 1.0

ToolIndicatorPage{
    id: root
    showExpand: false
    property var configurator: globals.activeVehicle.autopilotPlugin.configurator
    property var currType: configurator?.currentType
    property var currMissn: currType?.currentMission

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: true
    }

    contentComponent: Component {
        SettingsGroupLayout{
            showDividers: false
            heading: qsTr("Mission configuration")

            SettingsGroupLayout{
                heading: qsTr("FRAME TYPE GUID_FRAME_TYPE")
                RowLayout{
                    spacing: ScreenTools.defaultFontPixelWidth
                    Layout.fillWidth:true
                    Repeater{
                        model: configurator.copterTypes
                        delegate: StatusButton{
                            text: model.name
                            statusIndex: model.status
                            implicitWidth: 100
                            implicitHeight: 50
                            textField.font.pointSize: ScreenTools.mediumFontPointSize
                            onClicked: modelData.setActive()
                        }
                    }
                }
            }

            SettingsGroupLayout{
                heading: qsTr("Mission type MISSN_GUID_TYPE")
                GridLayout{
                    Layout.fillWidth:true
                    Layout.fillHeight:true
                    id: grid
                    columns: 3
                    columnSpacing: ScreenTools.defaultFontPixelWidth
                    rowSpacing: ScreenTools.defaultFontPixelWidth
                    Repeater{
                        model: currType.missions
                        delegate: StatusButton{
                            text: model.name
                            Layout.fillWidth: true
                            statusIndex: model.status
                            implicitHeight: 40
                            implicitWidth: 0 //disabling implicit width, so buttons spread evenly
                            onClicked: modelData.setActive()
                        }
                    }
                }
            }

            SettingsGroupLayout{
                Layout.fillWidth:true
                height: ScreenTools.defaultFontPixelHeight
                id: tuningColumn
                heading: qsTr("Configuration")
                QGCLabel{
                    // Layout.fillWidth:true
                    Layout.alignment:Qt.AlignHCenter
                    text: qsTr("WARNING! Parameter list is not full!")
                    color: qgcPal.colorOrange
                    visible: !currMissn.parametersReady
                }

                QGCFlickable{
                    id: flick
                    Layout.fillWidth:       true
                    Layout.preferredHeight: Math.min(contentHeight,ScreenTools.defaultFontPixelHeight * 16)
                    contentHeight:          tunableList.height
                    contentWidth:           parent.width
                    clip:                   true
                    ColumnLayout{
                        id: tunableList
                        width:flick.width - ScreenTools.defaultFontPixelWidth
                        Repeater{
                            model:currMissn.tunableParameters
                            delegate:ColumnLayout{
                                Layout.fillWidth:true
                                RowLayout{
                                    Layout.fillWidth: true
                                    ColumnLayout{
                                        Layout.fillWidth: true

                                        HoverHandler{
                                            id: hoverHandeler
                                        }

                                        ToolTip.visible: hoverHandeler.hovered
                                        ToolTip.delay: 500
                                        ToolTip.text: modelData.shortDescription

                                        QGCLabel{
                                            Layout.fillWidth: true
                                            id: name
                                            text: modelData.name
                                            elide: Text.ElideRight
                                        }
                                        QGCLabel{
                                            id:description
                                            Layout.fillWidth: true
                                            text: modelData.shortDescription.split(/<br\s*\/?>|\n/i)[0].trim()
                                            font.pointSize: ScreenTools.smallFontPointSize
                                            elide: Text.ElideRight
                                        }
                                    }
                                    FactTextField{
                                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 18
                                        Layout.alignment: Qt.AlignRight
                                        fact: modelData
                                    }
                                }
                                Rectangle{
                                    Layout.fillWidth: true
                                    radius: 2
                                    height: 1
                                    color: qgcPal.groupBorder
                                }
                            }
                        }
                    }
                }
            }
            QGCButton{
                text:qsTr("Reboot Required")
                Layout.fillWidth:true
                visible: currType.status != 1 || !currMissn.parametersReady
                onClicked:{

                    QGroundControl.showMessageDialog(root,qsTr("Reboot VGM?"),
                                                     qsTr("Select Ok to reboot VGM."),
                                                     Dialog.Cancel | Dialog.Ok,
                                                     function() {globals.activeVehicle.autopilotPlugin.rebootOnboardComputers()
                                                         mainWindow.closeIndicatorDrawer()}
                                                     )

                }
            }
        }
    }
}

