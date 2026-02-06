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

ScrollView{
    anchors.fill: parent
    TextArea {
        id:                     messageText
        readOnly:               true
        textFormat:             TextEdit.RichText
        color:                  qgcPal.text
        placeholderText:        qsTr("No Messages")
        placeholderTextColor:   qgcPal.text
        padding:                0
        wrapMode:               TextEdit.Wrap

        property bool noMessages: messageText.length === 0

        property var _fact: null

        function formatMessage(message) {
            message = message.replace(new RegExp("<#E>", "g"), "color: " + qgcPal.warningText + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
            message = message.replace(new RegExp("<#I>", "g"), "color: " + qgcPal.warningText + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
            message = message.replace(new RegExp("<#N>", "g"), "color: " + qgcPal.text + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
            return message;
        }

        Component.onCompleted: {
            messageText.text = messageText.formatMessage(_activeVehicle.formattedMessages)
            if (_activeVehicle) {
                _activeVehicle.resetAllMessages()
            }
        }

        Connections {
            target: _activeVehicle
            function onNewFormattedMessage(formattedMessage) { messageText.insert(0, messageText.formatMessage(formattedMessage)) }
        }

        FactPanelController {
            id: controller
        }

        onLinkActivated: (link) => {
                             if (link.startsWith('param://')) {
                                 var paramName = link.substr(8);
                                 _fact = controller.getParameterFact(-1, paramName, true)
                                 if (_fact != null) {
                                     paramEditorDialogComponent.createObject(mainWindow).open()
                                 }
                             } else {
                                 Qt.openUrlExternally(link);
                             }
                         }

        Component {
            id: paramEditorDialogComponent

            ParameterEditorDialog {
                title:          qsTr("Edit Parameter")
                fact:           messageText._fact
                destroyOnClose: true
            }
        }
    }
}
