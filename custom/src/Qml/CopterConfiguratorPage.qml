import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import Custom.Widgets 1.0

ToolIndicatorPage{
    id: root
    showExpand: false

    ListModel{
        id: availableTypes

        ListElement{
            name: "Kamikaze"
            enable: true
            status: 0
        }
        ListElement{
            name: "Plane"
            enable: false
            status: 3
        }
        ListElement{
            name: "Bomber"
            enable: true
            status: 1
        }
        ListElement{
            name: "Photolit"
            enable: true
            status: 2
        }
    }

    contentComponent: Component {
        SettingsGroupLayout{
            heading: qsTr("Mission configuration")
            QGCLabel{
                text: qsTr("FRAME TYPE GUID_FRAME_TYPE")
                font.pointSize: ScreenTools.mediumFontPixelSize
                // color: QGCPalette.
            }

            RowLayout{
                spacing: ScreenTools.defaultFontPixelWidth
                Layout.fillWidth:true
                Repeater{
                    model: availableTypes
                    delegate: StatusButton{
                        text: model.name
                        enabled: model.enable
                        statusIndex: model.status
                        implicitWidth: 100
                        implicitHeight: 50
                        textField.font.pointSize: ScreenTools.mediumFontPointSize
                    }
                }
            }
            RowLayout{
                spacing: ScreenTools.defaultFontPixelWidth
                Repeater{
                    model: reqParams
                    delegate: StatusButton{
                        textField.font.pointSize: ScreenTools.mediumFontPixelWidth
                        implicitHeight: ScreenTools.mediumFontPixelHeight + 2
                        text: model.name
                        statusIndex: model.status
                        backgroundRect.border.width:0
                    }
                }
            }
        }
    }
}

