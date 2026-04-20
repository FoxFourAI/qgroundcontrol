#include "FoxFourPlugin.h"

#include <QtCore/QApplicationStatic>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlFile>

#include "BrandImageSettings.h"
#include "QGCLoggingCategory.h"
#include "QGCPalette.h"

QGC_LOGGING_CATEGORY(CustomLog, "FoxFour.Plugin")

Q_APPLICATION_STATIC(FoxFourPlugin, _customPluginInstance);

FoxFourPlugin::FoxFourPlugin(QObject* parent)
    : QGCCorePlugin(parent), _options(new QGCOptions(this)), _parameterSetter(new ParameterSetter(this)) {
    _version = QString(QGC_CUSTOM_VERSION);
    _showAdvancedUI = true;
    (void)connect(this, &FoxFourPlugin::showAdvancedUIChanged, this, &FoxFourPlugin::_advancedChanged);
}

QGCCorePlugin* FoxFourPlugin::instance() { return _customPluginInstance(); }

void FoxFourPlugin::cleanup() {
    if (_qmlEngine) {
        _qmlEngine->removeUrlInterceptor(_selector);
    }

    delete _selector;
}

void FoxFourPlugin::_advancedChanged(bool changed) {
    // Firmware Upgrade page is only show in Advanced mode
    emit _options->showFirmwareUpgradeChanged(changed);
}

bool FoxFourPlugin::overrideSettingsGroupVisibility(const QString& name) {
    // We have set up our own specific brand imaging.
    // Hide the brand image settings such that the end user can't change it.
    if (name == BrandImageSettings::name) {
        return false;
    }

    return true;
}

VideoReceiver* FoxFourPlugin::createVideoReceiver(QObject* parent) {
#ifdef QGC_GST_STREAMING
    return new FoxFourGstVideoReceiver(parent);
#elif defined(QGC_QT_STREAMING)
    return QtMultimediaReceiver::createVideoReceiver(parent);
#else
    return nullptr;
#endif
}

ParameterSetter* FoxFourPlugin::parameterSetter() { return _parameterSetter; }

QQmlApplicationEngine* FoxFourPlugin::createQmlApplicationEngine(QObject* parent) {
    _qmlEngine = QGCCorePlugin::createQmlApplicationEngine(parent);
    // TODO: Investigate _qmlEngine->setExtraSelectors({"custom"})
    _selector = new CustomOverrideInterceptor();
    _qmlEngine->addUrlInterceptor(_selector);

    return _qmlEngine;
}

void FoxFourPlugin::paletteOverride(const QString &colorName, QGCPalette::PaletteColorInfo_t &colorInfo)
{
    if (colorName == QStringLiteral("window")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#FFFFFF");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#FFFFFF");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#090A0B");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#090A0B");
    } else if (colorName == QStringLiteral("windowTransparent")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ccffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ccffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#cc222222");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#cc222222");
    } else if (colorName == QStringLiteral("windowShadeLight")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#8E97A1");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#76808A");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#515A64");
    } else if (colorName == QStringLiteral("windowShade")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#EFF1F3");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#EFF1F3");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#212529");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#212529");
    } else if (colorName == QStringLiteral("windowShadeDark")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#D5DBE0");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#D5DBE0");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#131517");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#131517");
    } else if (colorName == QStringLiteral("text")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#AAB3BB");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#FFFFFF");
    } else if (colorName == QStringLiteral("windowTransparentText")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#AAB3BB");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#FFFFFF");
    } else if (colorName == QStringLiteral("warningText")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#DC2F02");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#DC2F02");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#E04403");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#E04403");
    } else if (colorName == QStringLiteral("button")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#FFFFFF");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#FFFFFF");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#626270");
    } else if (colorName == QStringLiteral("buttonBorder")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#FFFFFF");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#EFF1F3");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#AAB3BB");
    } else if (colorName == QStringLiteral("buttonText")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#AAB3BB");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#B8C0C7");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#FFFFFF");
    } else if (colorName == QStringLiteral("buttonHighlight")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#F4F6F8");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#F07005");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#2E343A");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#FFD23A");
    } else if (colorName == QStringLiteral("buttonHighlightText")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#17191C");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#FFFFFF");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#17191C");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#000000");
    } else if (colorName == QStringLiteral("primaryButton")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#444C55");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#7FDBFF");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#444C55");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#7FDBFF");
    } else if (colorName == QStringLiteral("primaryButtonText")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#17191C");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#17191C");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#000000");
    } else if (colorName == QStringLiteral("textField")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#FFFFFF");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#FFFFFF");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#FFFFFF");
    } else if (colorName == QStringLiteral("textFieldText")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#6A737D");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#000000");
    } else if (colorName == QStringLiteral("mapButton")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#444C55");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#444C55");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#000000");
    } else if (colorName == QStringLiteral("mapButtonHighlight")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#444C55");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#F48C06");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#444C55");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#F48C06");
    } else if (colorName == QStringLiteral("mapIndicator")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#444C55");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#F48C06");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#444C55");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#F48C06");
    } else if (colorName == QStringLiteral("mapIndicatorChild")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#444C55");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#444C55");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#5B6671");
    } else if (colorName == QStringLiteral("colorGreen")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#255C2C");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#255C2C");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#69CB06");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#69CB06");
    } else if (colorName == QStringLiteral("colorYellow")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#9EF01A");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#9EF01A");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#CCFF33");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#CCFF33");
    } else if (colorName == QStringLiteral("colorYellowGreen")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#319900");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#319900");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#69CB06");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#69CB06");
    } else if (colorName == QStringLiteral("colorOrange")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#F99B06");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#F99B06");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#FAA307");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#FAA307");
    } else if (colorName == QStringLiteral("colorRed")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#DC2F02");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#DC2F02");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#E85D04");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#E85D04");
    } else if (colorName == QStringLiteral("colorGrey")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#6A737D");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#6A737D");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#DEE2E6");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#DEE2E6");
    } else if (colorName == QStringLiteral("colorBlue")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#1F66E5");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#1F66E5");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#4D8DFF");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#4D8DFF");
    } else if (colorName == QStringLiteral("alertBackground")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#FFBA08");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#FFBA08");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#FFBA08");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#FFBA08");
    } else if (colorName == QStringLiteral("alertBorder")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#6A737D");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#6A737D");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#6A737D");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#6A737D");
    } else if (colorName == QStringLiteral("alertText")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#000000");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#000000");
    } else if (colorName == QStringLiteral("missionItemEditor")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#444C55");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#F8F9FA");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#444C55");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#5B6671");
    } else if (colorName == QStringLiteral("toolStripHoverColor")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#444C55");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#AAB3BB");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#444C55");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#5B6671");
    } else if (colorName == QStringLiteral("statusFailedText")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#AAB3BB");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#FFFFFF");
    } else if (colorName == QStringLiteral("statusPassedText")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#AAB3BB");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#FFFFFF");
    } else if (colorName == QStringLiteral("statusPendingText")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#AAB3BB");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#FFFFFF");
    } else if (colorName == QStringLiteral("toolbarBackground")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#00ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#00ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#00222222");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#00222222");
    } else if (colorName == QStringLiteral("toolbarDivider")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#00000000");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#00000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#00000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#00000000");
    } else if (colorName == QStringLiteral("groupBorder")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#C9D0D6");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#C9D0D6");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#5B6671");
    } else if (colorName == QStringLiteral("brandingPurple")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#133E7C");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#133E7C");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#133E7C");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#133E7C");
    } else if (colorName == QStringLiteral("brandingBlue")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#7FDBFF");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#7FDBFF");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#1F66E5");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#1F66E5");
    } else if (colorName == QStringLiteral("toolStripFGColor")) {
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#5B6671");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#5B6671");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#FFFFFF");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#FFFFFF");
    }
}

/*===========================================================================*/

CustomOverrideInterceptor::CustomOverrideInterceptor() : QQmlAbstractUrlInterceptor() {}

QUrl CustomOverrideInterceptor::intercept(const QUrl& url, QQmlAbstractUrlInterceptor::DataType type) {
    switch (type) {
    using DataType = QQmlAbstractUrlInterceptor::DataType;
    case DataType::QmlFile:
    case DataType::UrlString:
        if (url.scheme() == QStringLiteral("qrc")) {
            const QString origPath = url.path();
            const QString overrideRes = QStringLiteral(":/Custom%1").arg(origPath);

            if (QFile::exists(overrideRes)) {
                // if (overrideRes.endsWith("qml")) {
                    qCDebug(CustomLog) << "Overiding: " << origPath << " with " << overrideRes;
                    // }
                    const QString relPath = overrideRes.mid(2);
                    QUrl result;
                    result.setScheme(QStringLiteral("qrc"));
                    result.setPath('/' + relPath);
                    return result;
            }
        }
        break;
    default:
        break;
    }

    return url;
}
