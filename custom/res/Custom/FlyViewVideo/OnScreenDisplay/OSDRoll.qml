import QtQuick
import FoxFour.Widgets 1.0

OSDCanvas {
    id: _root
    property var value
    onValueChanged: requestPaint()

    onPaint: {
        ctx = begin()

    }
}
