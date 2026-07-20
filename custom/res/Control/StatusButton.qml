import QtQuick

import QGroundControl
import QGroundControl.Controls

import Custom.Widgets 1.0

Button {
    id: statusButton

    property int statusIndex: 0
    property alias textField: field
    property alias backgroundRect: bg
    MouseArea{
        anchors.fill:parent
        acceptedButtons: Qt.NoButton
        cursorShape: statusButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
    contentItem: StatusLabel{
        id: field
        text: parent.text
        statusIndex: parent.checkable ? parent.checked ? parent.statusIndex : 0 : parent.statusIndex
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: StatusRect{
        id: bg
        statusIndex: parent.checkable ? parent.checked ? parent.statusIndex : 0 : parent.statusIndex
    }
}
