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
import QGroundControl.Palette
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Controllers

Rectangle {
    id:     _root
    width:  parent.width
    height: ScreenTools.toolbarHeight
    color:  qgcPal.toolbarBackground

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property color  _mainStatusBGColor: qgcPal.brandingPurple

    function dropMainStatusIndicatorTool() {
        mainStatusIndicator.dropMainStatusIndicator();
    }

    QGCPalette { id: qgcPal }

    /// Bottom single pixel divider
    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        height:         1
        color:          "black"
        visible:        qgcPal.globalTheme === QGCPalette.Light
    }

    Rectangle {
        anchors.fill: viewButtonRow
        
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0;                                     color: _mainStatusBGColor }
            GradientStop { position: currentButton.x + currentButton.width; color: _mainStatusBGColor }
            GradientStop { position: 1;                                     color: _root.color }
        }
    }

    RowLayout {
        id:                     viewButtonRow
        anchors.bottomMargin:   1
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        spacing:                ScreenTools.defaultFontPixelWidth / 2

        QGCToolBarButton {
            id:                     currentButton
            Layout.preferredHeight: viewButtonRow.height
            icon.source:            "/res/QGCLogoFull.svg"
            logo:                   true
            onClicked:              mainWindow.showToolSelectDialog()
        }

        Rectangle {
            id: loginButton
            Layout.preferredHeight: viewButtonRow.height
            width: 120  // Reduced width since authorizing shows spinner instead of text
            radius: 6
            
            property var authManager: QGroundControl.openIDAuthManager
            
            // Enhanced styling based on auth state
            property color baseColor: {
                if (authManager.statusColor === "#95F792") {
                    return "#95F792"  // Success green
                } else if (authManager.statusColor === "yellow") {
                    return "#3254AD"  // Delta blue for processing
                } else if (authManager.statusColor === "red") {
                    return "#DC3545"  // Error red
                } else {
                    return "#3254AD"  // Delta blue default
                }
            }
            
            color: baseColor
            border.width: loginMouseArea.pressed ? 2 : 1
            border.color: Qt.darker(baseColor, 1.2)
            
            // Subtle shadow effect
            Rectangle {
                anchors.fill: parent
                anchors.topMargin: 1
                radius: parent.radius
                color: Qt.darker(parent.color, 1.1)
                opacity: 0.3
                z: -1
            }
            
            // Logo positioned on the left
            Image {
                id: deltaLogo
                source: "/res/DeltaLog.svg"
                width: 24
                height: 28
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                smooth: true
                mipmap: true
                fillMode: Image.PreserveAspectFit
            }
            
            // Text centered in the remaining space (hidden during authorizing)
            Text {
                id: loginText
                text: loginButton.authManager.statusText
                font.pixelSize: ScreenTools.defaultFontPixelSize * 0.9
                font.weight: Font.Medium
                color: loginButton.authManager.statusColor === "#95F792" ? "black" : "white"
                anchors.left: deltaLogo.right
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                maximumLineCount: 1
                visible: loginButton.authManager.authState !== loginButton.authManager.Authorizing
            }
            
            // Rotating wheel for authorizing state
            Rectangle {
                id: spinningWheel
                width: 20
                height: 20
                radius: 10
                color: "transparent"
                border.width: 2
                border.color: "white"
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: 12  // Offset to center in text area (adjusted for 120px width)
                visible: loginButton.authManager.authState === loginButton.authManager.Authorizing
                
                // Create the spinning effect with a partial circle
                Rectangle {
                    width: 4
                    height: 4
                    radius: 2
                    color: "white"
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                RotationAnimation on rotation {
                    running: spinningWheel.visible
                    from: 0
                    to: 360
                    duration: 1000
                    loops: Animation.Infinite
                }
            }
            
            // Simple loading indicator with opacity change
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: Qt.rgba(1, 1, 1, 0.1)  // Subtle white overlay
                opacity: (loginButton.authManager.authState === loginButton.authManager.Authenticating || 
                         loginButton.authManager.authState === loginButton.authManager.Authorizing) ? 1.0 : 0.0
                visible: opacity > 0
                
                Behavior on opacity {
                    NumberAnimation { duration: 200 }
                }
            }
            
            MouseArea {
                id: loginMouseArea
                anchors.fill: parent
                enabled: loginButton.authManager.authState !== loginButton.authManager.Authenticating && 
                        loginButton.authManager.authState !== loginButton.authManager.Authorizing
                
                onClicked: {
                    console.log("Login button clicked, current state:", loginButton.authManager.authState)
                    
                    switch (loginButton.authManager.authState) {
                    case loginButton.authManager.NotAuthenticated:
                    case loginButton.authManager.Error:
                        console.log("Starting login flow...")
                        loginButton.authManager.login()
                        break
                    case loginButton.authManager.Authenticated:
                        console.log("Starting authorization...")
                        loginButton.authManager.authorize()
                        break
                    case loginButton.authManager.Authorized:
                        console.log("Already authorized - could show menu or logout")
                        // Could add logout or show user info here
                        break
                    default:
                        console.log("Unexpected auth state:", loginButton.authManager.authState)
                        break
                    }
                }
            }
        }

        MainStatusIndicator {
            id: mainStatusIndicator
            Layout.preferredHeight: viewButtonRow.height
        }

        QGCButton {
            id:                 disconnectButton
            text:               qsTr("Disconnect")
            onClicked:          _activeVehicle.closeVehicle()
            visible:            _activeVehicle && _communicationLost
        }
    }

    QGCFlickable {
        id:                     toolsFlickable
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio * 1.5
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth / 2
        anchors.left:           viewButtonRow.right
        anchors.bottomMargin:   1
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.right:          parent.right
        contentWidth:           toolIndicators.width
        flickableDirection:     Flickable.HorizontalFlick

        FlyViewToolBarIndicators { id: toolIndicators }
    }

    //-------------------------------------------------------------------------
    //-- Branding Logo
    Image {
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.margins:        ScreenTools.defaultFontPixelHeight * 0.66
        visible:                _activeVehicle && !_communicationLost && x > (toolsFlickable.x + toolsFlickable.contentWidth + ScreenTools.defaultFontPixelWidth)
        fillMode:               Image.PreserveAspectFit
        source:                 _outdoorPalette ? _brandImageOutdoor : _brandImageIndoor
        mipmap:                 true

        property bool   _outdoorPalette:        qgcPal.globalTheme === QGCPalette.Light
        property bool   _corePluginBranding:    QGroundControl.corePlugin.brandImageIndoor.length != 0
        property string _userBrandImageIndoor:  QGroundControl.settingsManager.brandImageSettings.userBrandImageIndoor.value
        property string _userBrandImageOutdoor: QGroundControl.settingsManager.brandImageSettings.userBrandImageOutdoor.value
        property bool   _userBrandingIndoor:    QGroundControl.settingsManager.brandImageSettings.visible && _userBrandImageIndoor.length != 0
        property bool   _userBrandingOutdoor:   QGroundControl.settingsManager.brandImageSettings.visible && _userBrandImageOutdoor.length != 0
        property string _brandImageIndoor:      brandImageIndoor()
        property string _brandImageOutdoor:     brandImageOutdoor()

        function brandImageIndoor() {
            if (_userBrandingIndoor) {
                return _userBrandImageIndoor
            } else {
                if (_userBrandingOutdoor) {
                    return _userBrandImageOutdoor
                } else {
                    if (_corePluginBranding) {
                        return QGroundControl.corePlugin.brandImageIndoor
                    } else {
                        return _activeVehicle ? _activeVehicle.brandImageIndoor : ""
                    }
                }
            }
        }

        function brandImageOutdoor() {
            if (_userBrandingOutdoor) {
                return _userBrandImageOutdoor
            } else {
                if (_userBrandingIndoor) {
                    return _userBrandImageIndoor
                } else {
                    if (_corePluginBranding) {
                        return QGroundControl.corePlugin.brandImageOutdoor
                    } else {
                        return _activeVehicle ? _activeVehicle.brandImageOutdoor : ""
                    }
                }
            }
        }
    }

    // Small parameter download progress bar
    Rectangle {
        anchors.bottom: parent.bottom
        height:         _root.height * 0.05
        width:          _activeVehicle ? _activeVehicle.loadProgress * parent.width : 0
        color:          qgcPal.colorGreen
        visible:        !largeProgressBar.visible
    }

    // Large parameter download progress bar
    Rectangle {
        id:             largeProgressBar
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         parent.height
        color:          qgcPal.window
        visible:        _showLargeProgress

        property bool _initialDownloadComplete: _activeVehicle ? _activeVehicle.initialConnectComplete : true
        property bool _userHide:                false
        property bool _showLargeProgress:       !_initialDownloadComplete && !_userHide && qgcPal.globalTheme === QGCPalette.Light

        Connections {
            target:                 QGroundControl.multiVehicleManager
            function onActiveVehicleChanged(activeVehicle) { largeProgressBar._userHide = false }
        }

        Rectangle {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          _activeVehicle ? _activeVehicle.loadProgress * parent.width : 0
            color:          qgcPal.colorGreen
        }

        QGCLabel {
            anchors.centerIn:   parent
            text:               qsTr("Downloading")
            font.pointSize:     ScreenTools.largeFontPointSize
        }

        QGCLabel {
            anchors.margins:    _margin
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            text:               qsTr("Click anywhere to hide")

            property real _margin: ScreenTools.defaultFontPixelWidth / 2
        }

        MouseArea {
            anchors.fill:   parent
            onClicked:      largeProgressBar._userHide = true
        }
    }
}
