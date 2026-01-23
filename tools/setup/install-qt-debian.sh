#!/usr/bin/env bash

set -e

QT_VERSION="${QT_VERSION:-6.10.0}"
QT_PATH="${QT_PATH:-/opt/Qt}"
QT_HOST="${QT_HOST:-linux}"
QT_TARGET="${QT_TARGET:-desktop}"
QT_ARCH="${QT_ARCH:-linux_gcc_64}"
QT_ARCH_DIR="${QT_ARCH_DIR:-gcc_64}"
QT_ROOT_DIR="${QT_ROOT_DIR:-${QT_PATH}/${QT_VERSION}/${QT_ARCH_DIR}}"
QT_MODULES="${QT_MODULES:-qtcharts qtlocation qtpositioning qtspeech qt5compat qtmultimedia qtserialport qtimageformats qtshadertools qtconnectivity qtquick3d qtsensors qtscxml}"

echo "QT_VERSION $QT_VERSION"
echo "QT_PATH $QT_PATH"
echo "QT_HOST $QT_HOST"
echo "QT_TARGET $QT_TARGET"
echo "QT_ARCH $QT_ARCH"
echo "QT_ARCH_DIR $QT_ARCH_DIR"
echo "QT_ROOT_DIR $QT_ROOT_DIR"
echo "QT_MODULES $QT_MODULES"

apt-get update -y --quiet
apt-get install python3 python3-pip pipx -y
pipx install aqtinstall
pipx install cmake
pipx install ninja
pipx ensurepath
export PATH="$(python3 -m site --user-base)/bin:$PATH"
aqt install-qt ${QT_HOST} ${QT_TARGET} ${QT_VERSION} ${QT_ARCH} -O ${QT_PATH} -m ${QT_MODULES}

export QT_ROOT_DIR=$(readlink -e ${QT_ROOT_DIR})
export PATH=$(readlink -e ${QT_ROOT_DIR}/bin/):$PATH
export PKG_CONFIG_PATH=$(readlink -e ${QT_ROOT_DIR}/lib/pkgconfig):$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=$(readlink -e ${QT_ROOT_DIR}/lib):$LD_LIBRARY_PATH
export QT_PLUGIN_PATH=$(readlink -e ${QT_ROOT_DIR}/plugins)
export QML2_IMPORT_PATH=$(readlink -e ${QT_ROOT_DIR}/qml)

echo "QT_ROOT_DIR $QT_ROOT_DIR"
echo "PATH $PATH"
echo "PKG_CONFIG_PATH $PKG_CONFIG_PATH"
echo "LD_LIBRARY_PATH $LD_LIBRARY_PATH"
echo "QT_PLUGIN_PATH $QT_PLUGIN_PATH"
echo "QML2_IMPORT_PATH $QML2_IMPORT_PATH"

#Check Qt dependencies:
apt-get install -y -qq --no-install-recommends \
    libqt6core6t64 \
    libqt6dbus6t64 \
    libqt6gui6t64 \
    libqt6network6t64 \
    libqt6opengl6t64 \
    libqt6printsupport6t64 \
    libqt6qml6 \
    libqt6qmlmodels6 \
    libqt6quick6 \
    libqt6svg6 \
    libqt6waylandclient6 \
    libqt6waylandcompositor6 \
    libqt6waylandeglclienthwintegration6 \
    libqt6waylandeglcompositorhwintegration6 \
    libqt6widgets6t64 \
    libqt6wlshellintegration6 \
    qt6-translations-l10n \