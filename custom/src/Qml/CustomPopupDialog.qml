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
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls



// Provides the standard dialog mechanism for QGC. Works 99% like Qml Dialog.
//
// Example usage:
//      Component {
//          id: dialogComponent
//
//          QGCPopupDialog {
//              ...
//          }
//      }
//
//      onFoo: dialogComponent.createObject(mainWindow).open()
//
// Notes:
//  * QGCPopupDialog should be created from a component to limit the memory usage of the dialog
//      to only when it is displayed.
//  * Parent for createObject should always be mainWindow.
// Differences from standard Qml Dialog:
//  * The QGCPopupDialog object will automatically be destroyed when it closed. You can override this
//      behaviour by setting destroyOnClose to false if it was not created dynamically.
//  * Dialog will automatically close after accepted/rejected signal processing. You can prevent this by setting
//      preventClose = true prior to returning from your signal handlers.
Popup {
    id:                 root
    width:              mainWindow.width * 0.4
    height:             ScreenTools.defaultFontPixelHeight * 3.5
    parent:             Overlay.overlay
    modal:              false
    focus:              true
    margins:            0

    x: (mainWindow.width - width) / 2.
    y:  mainWindow.height - height - ScreenTools.defaultFontPixelHeight * 2


    property string title
    property var    buttons:                Dialog.Ok
    property alias  acceptButtonEnabled:    acceptButton.enabled
    property alias  rejectButtonEnabled:    rejectButton.enabled
    property var    dialogProperties
    property bool   destroyOnClose:         true
    property bool   preventClose:           false

    signal accepted
    signal rejected

    property var    _qgcPal:            QGroundControl.globalPalette
    property real   _frameSize:         ScreenTools.defaultFontPixelWidth
    property real   _contentMargin:     ScreenTools.defaultFontPixelHeight / 2
    property bool   _acceptAllowed:     acceptButton.visible
    property bool   _rejectAllowed:     rejectButton.visible
    property int    _previousValidationErrorCount: 0
    property int    _closeInterval: 10000

    background: Item{}

    QGCMouseArea {
        width:  parent.width
        height: parent.height
        onEntered: {
            console.log("entered")
            closeTimer.stop()
        }
        onExited: {
            console.log("leaved")
            closeTimer.start()
        }
    }

    Timer{
        id: closeTimer
        interval: root._closeInterval
        running: false
        onTriggered: root.close()
    }

    enter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200 }
            NumberAnimation { property: "y"; from: root.y + 20; to: root.y; duration: 200 }
        }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 200 }
        NumberAnimation { property: "y"; from: root.y; to: root.y + 20; duration: 200 }
    }

    Component.onCompleted: {
        // The last child item will be the true dialog content.
        // Re-Parent it to the right place in the ui hierarchy.
        contentChildren[contentChildren.length - 1].parent = dialogContentParent
        closeTimer.start()

    }

    onAboutToShow: {
        _previousValidationErrorCount = globals.validationErrorCount
        setupDialogButtons(buttons)
    }

    onClosed: {
        globals.validationErrorCount = _previousValidationErrorCount
        Qt.inputMethod.hide()
        if (destroyOnClose) {
            root.destroy()
        }
    }

    function _accept() {
        if (_acceptAllowed && mainWindow.allowViewSwitch(_previousValidationErrorCount)) {
            accepted()
            if (preventClose) {
                preventClose = false
            } else {
                close()
            }
        }
    }

    function _reject() {
        // Dialogs with cancel button are allowed to close with validation errors
        if (_rejectAllowed && ((buttons & Dialog.Cancel) || mainWindow.allowViewSwitch(_previousValidationErrorCount))) {
            rejected()
            if (preventClose) {
                preventClose = false
            } else {
                close()
            }
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: parent.enabled }

    function setupDialogButtons(buttons) {
        acceptButton.visible = false
        rejectButton.visible = false
        // Accept role buttons
        if (buttons & Dialog.Ok) {
            acceptButton.text = qsTr("Ok")
            acceptButton.visible = true
        } else if (buttons & Dialog.Open) {
            acceptButton.text = qsTr("Open")
            acceptButton.visible = true
        } else if (buttons & Dialog.Save) {
            acceptButton.text = qsTr("Save")
            acceptButton.visible = true
        } else if (buttons & Dialog.Apply) {
            acceptButton.text = qsTr("Apply")
            acceptButton.visible = true
        } else if (buttons & Dialog.Open) {
            acceptButton.text = qsTr("Open")
            acceptButton.visible = true
        } else if (buttons & Dialog.SaveAll) {
            acceptButton.text = qsTr("Save All")
            acceptButton.visible = true
        } else if (buttons & Dialog.Yes) {
            acceptButton.text = qsTr("Yes")
            acceptButton.visible = true
        } else if (buttons & Dialog.YesToAll) {
            acceptButton.text = qsTr("Yes to All")
            acceptButton.visible = true
        } else if (buttons & Dialog.Retry) {
            acceptButton.text = qsTr("Retry")
            acceptButton.visible = true
        } else if (buttons & Dialog.Reset) {
            acceptButton.text = qsTr("Reset")
            acceptButton.visible = true
        } else if (buttons & Dialog.RestoreToDefaults) {
            acceptButton.text = qsTr("Restore to Defaults")
            acceptButton.visible = true
        } else if (buttons & Dialog.Ignore) {
            acceptButton.text = qsTr("Ignore")
            acceptButton.visible = true
        }

        // Reject role buttons
        if (buttons & Dialog.Cancel) {
            rejectButton.text = qsTr("Cancel")
            rejectButton.visible = true
        } else if (buttons & Dialog.Close) {
            rejectButton.text = qsTr("Close")
            rejectButton.visible = true
        } else if (buttons & Dialog.No) {
            rejectButton.text = qsTr("No")
            rejectButton.visible = true
        } else if (buttons & Dialog.NoToAll) {
            rejectButton.text = qsTr("No to All")
            rejectButton.visible = true
        } else if (buttons & Dialog.Abort) {
            rejectButton.text = qsTr("Abort")
            rejectButton.visible = true
        }

        closePolicy = Popup.NoAutoClose
        if (buttons & Dialog.Cancel) {
            closePolicy |= Popup.CloseOnEscape
        }
    }

    function disableAcceptButton() {
        acceptButton.enabled = false
    }

    Rectangle {
        x:              mainLayout.x - _contentMargin
        y:              mainLayout.y - _contentMargin
        width:          mainLayout.width + _contentMargin * 2
        height:         mainLayout.height + _contentMargin * 2
        color:          _qgcPal.windowShade
        radius:         root.padding / 2
        border.width:   1
        border.color:   _qgcPal.windowShadeLight
    }

    RowLayout {
        id:                     mainLayout
        anchors.fill:           parent
        spacing:                _contentMargin

        QGCLabel {
            id:                 titleLabel
            text:               root.title
            font.pointSize:     ScreenTools.mediumFontPointSize
            verticalAlignment:	Text.AlignVCenter
            visible: width < root.width / 3
        }

        Rectangle {
            id: body
            Layout.fillHeight:       true
            Layout.preferredWidth:  maxAvailableWidth
            Layout.fillWidth:        true
            color:                  _qgcPal.window

            property real totalContentWidth:    dialogContentParent.childrenRect.width + _contentMargin * 2
            property real totalContentHeight:   dialogContentParent.childrenRect.height + _contentMargin * 2
            property real maxAvailableWidth:    mainWindow.width / 4 - parent.width
            property real maxAvailableHeight:   ScreenTools.defaultFontPixelHeight * 3

            QGCFlickable {
                anchors.margins:    _contentMargin
                anchors.fill:       parent
                contentWidth:       dialogContentParent.childrenRect.width
                contentHeight:      dialogContentParent.childrenRect.height

                Item {
                    id:     dialogContentParent
                    focus:  true

                    //we close the dialog on each button, but have a different priority.
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Escape) {
                            console.log("rejected")
                            if( _rejectAllowed ) {
                                _reject()
                            } else {
                                _accept()
                            }
                            event.accepted = true
                        }
                        if(event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            if( _acceptAllowed ){
                                _accept()
                            } else {
                                _reject()
                            }
                            event.accepted = true
                        }
                    }
                }
            }
        }

        QGCButton {
            id:                     rejectButton
            onClicked:              _reject()
            Layout.minimumWidth:    height * 1.5
        }

        QGCButton {
            id:                     acceptButton
            primary:                true
            onClicked:              _accept()
            Layout.minimumWidth:    height * 1.5
        }
    }
}
