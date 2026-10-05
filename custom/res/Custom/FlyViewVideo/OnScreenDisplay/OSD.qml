pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects

import QGroundControl

Item {
    id: _root

    // style
    OSDStyle {
        id: _style
    }

    // settings
    property var _settingsManager:  QGroundControl.settingsManager
    property var _settings:         _settingsManager.foxFourSettings

    // status providers
    property var _activeVehicle:    globals.activeVehicle
    property var _videoManager:     QGroundControl.videoManager
    property var activeGroup:       _activeVehicle ? _activeVehicle.vehicle : null
    // status grid for telemetry
    property var statusGrid:        null

    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled:          true
        shadowColor:            _style.shadowColor
        shadowHorizontalOffset: _style.shadowOffset
        shadowVerticalOffset:   _style.shadowOffset
        shadowBlur:             _style.shadowBlur
    }

    // OSDLadder {
    //     anchors.fill: parent
    //     orientation: Qt.Horizontal
    //     tickSpacing: width / 80
    //     minorTickLength: 10
    //     majorTickLength: 15
    //     // height: 80
    //     mirrored: true
    //     value: 10
    //     header: "AVS"
    //     footer: "M/S"
    //     namedTicks: [{name:"ABC",value:20},{name:"DEF",value:10}]
    //     sectors: [{from:10,to:15,color:"red"}, {from:16,to:18,color:"green"}]
    //     targetValue: 0
    // }

    Grid {
        id: rec
        anchors.centerIn: parent
        columns: 2
        rows: 2
        spacing: 4
        property real dummy: 6
        Timer {
            repeat: true
            running: true
            interval: 100
            // onTriggered: rec.dummy += 0.1
        }

        Rectangle{
            width: 400
            height: 400
            color: Qt.rgba(0,0,0,0.0)
            OSDLadder {
                anchors.fill: parent
                orientation: Qt.Horizontal
                mirrored: true
                value: rec.dummy
                header: "AVS"
                footer: "M/S"
                namedTicks: [{name:"ABC",value:10},{name:"DEF",value:11}]
                sectors: [{name:"LOW",from:10,to:15,color:"red"}, {name:"HI",from:16,to:18,color:"green"}]
                targetValue: 9
            }
            Label {
                anchors.centerIn: parent
                text: qsTr("Hor+M")
                color: "white"
            }
        }

        Rectangle{
            width: 400
            height: 400
            color: Qt.rgba(0,0,0,0.5)
            OSDLadder {
                anchors.fill: parent
                orientation: Qt.Vertical
                mirrored: true
                value: rec.dummy
                header: "AVS"
                footer: "M/S"
                namedTicks: [{name:"ABC",value:10},{name:"DEF",value:11}]
                sectors: [{name:"LOW",from:10,to:15,color:"red"}, {name:"HI",from:16,to:18,color:"green"}]
                targetValue: 9

            }
            Label {
                anchors.centerIn: parent
                text: qsTr("Vert+M")
                color: "white"
            }
        }
        Rectangle{
            width: 400
            height: 400
            color: Qt.rgba(0,0,0,0.5)
            OSDLadder {
                anchors.fill: parent
                orientation: Qt.Horizontal
                // mirrored: true
                value: rec.dummy
                header: "AVS"
                footer: "M/S"
                namedTicks: [{name:"ABC",value:10},{name:"DEF",value:11}]
                sectors: [{name:"LOW",from:10,to:15,color:"red"}, {name:"HI",from:16,to:18,color:"green"}]
                targetValue: 9
            }
            Label {
                anchors.centerIn: parent
                text: qsTr("Hor")
                color: "white"
            }
        }
        Rectangle{
            width: 400
            height: 400
            color: Qt.rgba(0,0,0,0.5)
            OSDLadder {
                anchors.fill: parent
                orientation: Qt.Vertical
                // mirrored: true
                value: rec.dummy
                header: "AVS"
                footer: "M/S"
                namedTicks: [{name:"ABC",value:10},{name:"DEF",value:11}]
                sectors: [{name:"LOW",from:10,to:15,color:"red"}, {name:"HI",from:16,to:18,color:"green"}]
                targetValue: 9
            }
            Label {
                anchors.centerIn: parent
                text: qsTr("Vert")
                color: "white"
            }
        }
    }
}
