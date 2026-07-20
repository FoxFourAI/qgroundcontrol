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

    ListModel{
        id: tunableParams

        ListElement{
            name: "Disable"
            enable: true
            status: 1
        }
        ListElement{
            name: "Hover"
            enable: false
            status: 1
        }
        ListElement{
            name: "Terminal Attack"
            enable: true
            status: 1
        }
        ListElement{
            name: "Tuning"
            enable: true
            status: 1
        }
        ListElement{
            name: "Cruise"
            enable: true
            status: 1
        }
    }

    ListModel{
        id: tunableParameters
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
        }
        ListElement{
            name: "Element"
            status: 1
            Description: "blah blah blah"
            min: -1
            max: 100
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
            GridLayout{
                Layout.fillWidth:true
                Layout.fillHeight:true
                id: grid
                columns: 3
                columnSpacing: ScreenTools.defaultFontPixelWidth
                rowSpacing: ScreenTools.defaultFontPixelWidth
                Repeater{
                    model: tunableParams
                    delegate: StatusButton{
                        required property var model
                        text: model.name
                        Layout.fillWidth: true
                        statusIndex: model.status
                        implicitHeight: 40
                        implicitWidth: 0 //disabling implicit width, so buttons spread evenly
                        checkable: true
                    }
                }
            }
            SettingsGroupLayout{
                Layout.fillWidth: true
                heading: qsTr("Tuning")
                ColumnLayout{
                    Layout.fillWidth: true
                    Repeater{
                        model:tunableParameters
                        delegate: StatusRect{
                            required property var model
                            statusIndex: model.status
                            implicitHeight: 20
                            Layout.fillWidth:true
                            border.width: 0
                        }
                    }
                }
            }
        }
    }
}

