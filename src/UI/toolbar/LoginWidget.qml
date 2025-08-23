/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

Rectangle {
    id: root
    
    width: loginButton.width + ScreenTools.defaultFontPixelWidth
    height: ScreenTools.toolbarHeight
    color: "transparent"
    
    // Temporarily simplify to test basic component loading
    property string statusText: "Login"
    property string statusColor: "white"
    
    QGCPalette { id: qgcPal }
    
    QGCButton {
        id: loginButton
        anchors.centerIn: parent
        text: root.statusText
        
        property color baseColor: {
            if (root.statusColor === "#95F792") {
                return root.statusColor
            } else if (root.statusColor === "yellow") {
                return qgcPal.colorOrange
            } else if (root.statusColor === "red") {
                return qgcPal.colorRed
            } else {
                return qgcPal.button
            }
        }
        
        
        onClicked: {
            console.log("Login button clicked!")
            // Simple test - cycle through states
            if (root.statusText === "Login") {
                root.statusText = "Authorize"
            } else if (root.statusText === "Authorize") {
                root.statusText = "Authorized"
                root.statusColor = "#95F792"
            } else {
                root.statusText = "Login"
                root.statusColor = "white"
            }
        }
    }
}