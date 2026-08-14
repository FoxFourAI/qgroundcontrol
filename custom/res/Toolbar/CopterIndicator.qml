import QtQuick

import QGroundControl.Controls

import FoxFour.Widgets 1.0

Item {
    id: root
    width: statusButton.implicitWidth
    property var _vehicle: globals.activeVehicle
    property var configurator: _vehicle?.autopilotPlugin.configurator
    property var currType: configurator?.currentType
    property var currMissn: currType?.currentMission
    StatusButton{
        text: statusIndex != 0 ? root.currType.name + ' - ' + root.currMissn.name : "Unknown"
        statusIndex: (root.currType != undefined && root.currMissn != undefined) ? root.currType.status : 0
        id: statusButton
        onClicked:{
            if (statusIndex == 0) {
                root.configurator.refresh()
            } else  {
                root.currMissn.checkParameters()
                mainWindow.showIndicatorDrawer(copterConfigPage, anchor)
            }
        }
    }
    Item{
        id:anchor
        anchors.left: root.left
        width:0
    }

    Component {
        id: copterConfigPage
        CopterConfiguratorPage {}
    }
}
