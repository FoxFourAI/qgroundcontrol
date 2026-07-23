import QtQuick

import QGroundControl.Controls

import Custom.Widgets 1.0

Item {
    id: root
    width: statusButton.implicitWidth
    property var _vehicle: globals.activeVehicle
    property var configurator: _vehicle?.autopilotPlugin.configurator
    property var currType: configurator?.currentType
    property var currMissn: currType?.currentMission
    StatusButton{
        text: currType != undefined ? currType.name + ' - ' + currMissn.name : "Unknown"
        statusIndex: currType != undefined ? currType.status : 0
        id: statusButton
        onClicked:{
            if (statusIndex == 0) {
                _vehicle.parameterManager.refreshAllParameters()
            } else  {
                currMissn.checkParameters()
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
