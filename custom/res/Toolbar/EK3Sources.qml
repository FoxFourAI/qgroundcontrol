/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import Custom.Widgets 1.0
Item{
	id: root
	//to prevent empty spaces, resize widget when its needed
	// property var parameterSetter: QGroundControl.corePlugin.parameterSetter
	property var ekSrc : globals.activeVehicle.autopilotPlugin.ekSources
	width: mainRow.implicitWidth
	anchors.top: parent.top
	visible: ekSrc.visible
	anchors.bottom: parent.bottom
	QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

	RowLayout{
		id: mainRow
		anchors.top:    parent.top
		anchors.bottom: parent.bottom
		spacing:        ScreenTools.defaultFontPixelWidth / 2
		QGCLabel{
			Layout.alignment: Qt.AlignVCenter
			text: qsTr("Position Src")
		}
		QGCComboBox{
			Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 13
			model: ekSrc.sources 
			onCurrentIndexChanged: {ekSrc.setSource(currentIndex + 1)}
			currentIndex: ekSrc.currentSource
		}
	}
}
