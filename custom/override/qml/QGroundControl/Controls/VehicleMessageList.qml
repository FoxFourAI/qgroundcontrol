/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout{
    anchors.fill: parent
    ListView{
        Layout.fillWidth: true
        Layout.fillHeight: true
        id: listView
        clip: true

        property bool _autoScroll: true  // track if we should autoscroll

        ScrollBar.vertical: ScrollBar {
            id: scrollBar
            policy: ScrollBar.AlwaysOff
            onActiveChanged: listView._autoScroll = false
        }

        // Fire AFTER contentHeight settles, not before
        onContentHeightChanged: {
            if (_autoScroll) positionViewAtEnd()
        }

        ListModel {
            id: messages
        }

        model: messages

        delegate: TextEdit {
            required property string message
            text:       message
            enabled:    false
            textFormat: TextEdit.RichText
            width:      listView.width - scrollBar.width
            wrapMode:   TextEdit.WordWrap
            color:      qgcPal.text
        }

        Component.onCompleted: {
            let rawMsges = _activeVehicle.formattedMessages.split('<br/>')
            for (let message of rawMsges) {
                if (message === "") continue
                messages.append({ "message": formatMessage(message) })
            }
        }

        function formatMessage(message) {
            message = message.replace(new RegExp("<#E>", "g"), "color: " + qgcPal.warningText + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
            message = message.replace(new RegExp("<#I>", "g"), "color: " + qgcPal.warningText + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
            message = message.replace(new RegExp("<#N>", "g"), "color: " + qgcPal.text + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
            message = message.replace(new RegExp("<br\\s*/?>", "gi"), "")
            return message.trim()
        }

        Connections {
            target: _activeVehicle
            function onNewFormattedMessage(formattedMessage) {
                messages.append({ "message": listView.formatMessage(formattedMessage) })
            }
        }

        // FactPanelController {
        //     id: controller
        // }

        // onLinkActivated: (link) => {
        //                      if (link.startsWith('param://')) {
        //                          var paramName = link.substr(8);
        //                          _fact = controller.getParameterFact(-1, paramName, true)
        //                          if (_fact != null) {
        //                              paramEditorDialogComponent.createObject(mainWindow).open()
        //                          }
        //                      } else {
        //                          Qt.openUrlExternally(link);
        //                      }
        //                  }

        // Component {
        //     id: paramEditorDialogComponent

        //     ParameterEditorDialog {
        //         title:          qsTr("Edit Parameter")
        //         fact:           messageText._fact
        //         destroyOnClose: true
        //     }
        // }
    }
    QGCButton{
        text: "Auto Scroll"
        checkable: true
        checked: listView._autoScroll
        onCheckedChanged: {
            if(checked){
                listView.positionViewAtEnd()
            }

            listView._autoScroll = checked
        }
        Layout.fillWidth: true
    }
}


