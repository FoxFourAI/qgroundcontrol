import QtQuick

import QGroundControl.Controls

import Custom.Widgets 1.0

Item {
    id: root
    width: statusButton.implicitWidth
    StatusButton{
        text: "Kamikaze - Terminal Attack"
        id: statusButton
        onClicked:{
            mainWindow.showIndicatorDrawer(copterConfigPage, root)
        }
    }
    Component {
        id: copterConfigPage
        CopterConfiguratorPage {}
    }
}
