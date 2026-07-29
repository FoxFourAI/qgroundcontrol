#include "FoxFourParameterMetaData.h"

#include "JsonParsing.h"

FoxFourParameterMetaData::FoxFourParameterMetaData(QObject* parent) : APMParameterMetaData(parent) {}

void FoxFourParameterMetaData::parseParameterJson(const QJsonObject& json)
{
    APMParameterMetaData::parseParameterJson(json);
    // Adding VGM parameters to the default one

    QJsonDocument doc;
    QString errorString;
    const QString vgmMetaDataFile = ":/json/parameters.metadata.json";
    if (!JsonParsing::isJsonFile(vgmMetaDataFile, doc, errorString)) {
        qWarning() << "Unable to open parameter meta data file:" << vgmMetaDataFile << errorString;
        return;
    }
    if (!doc.isObject()) {
        qWarning() << "JSON root is not an object:" << vgmMetaDataFile;
        return;
    }

    QJsonObject obj = doc.object();

    for (auto groupIt = obj.constBegin(); groupIt != obj.constEnd(); ++groupIt) {
        if (!groupIt->isObject()) {
            continue;
        }

        const QJsonObject params = groupIt->toObject();

        for (auto paramIt = params.constBegin(); paramIt != params.constEnd(); ++paramIt) {
            if (!paramIt->isObject()) {
                continue;
            }

            const QString name = paramIt.key();
            const QString group = _groupFromParameterName(name);

            if (_rawVGMParams.contains(name)) {
                qWarning() << "Duplicate parameter found:" << name;
            }

            _rawVGMParams[name] = RawParameterData{group, paramIt->toObject()};
        }
    }
}

FactMetaData* FoxFourParameterMetaData::_lookupMetaData(const QString& name, FactMetaData::ValueType_t type)
{
    // if default search find metadata, return it
    FactMetaData* metadata = _lookupVGMMetaData(name, type);
    if (metadata != nullptr) {
        return metadata;
    }
    return APMParameterMetaData::_lookupMetaData(name, type);
}

QString FoxFourParameterMetaData::_groupFromParameterName(const QString& name)
{
    static const QRegularExpression regex(QStringLiteral("[0-9]*$"));
    QString group = name.split('_').first();
    return group.remove(regex);
}

QList<ParameterMetaData::ValueDescPair> FoxFourParameterMetaData::_sortedNumericPairs(const QJsonObject& obj,
                                                                                      const QString& paramName)
{
    // APM format: {"0":"Disabled","1":"Enabled"} — sort by numeric value
    // but preserve original string keys to avoid float round-trip issues.
    struct Entry
    {
        double sortKey;
        QString key;
        QString desc;
    };

    QList<Entry> entries;
    entries.reserve(obj.size());
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        bool ok = false;
        const double sortKey = it.key().toDouble(&ok);
        if (!ok) {
            qWarning() << "Non-numeric key:" << it.key() << "for" << paramName;
            continue;
        }
        entries.append({sortKey, it.key(), it->toString()});
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.sortKey < b.sortKey; });

    QList<ValueDescPair> pairs;
    pairs.reserve(entries.size());
    for (const auto& e : std::as_const(entries)) {
        pairs.append({e.key, e.desc});
    }
    return pairs;
}

void FoxFourParameterMetaData::_applyEnumValues(FactMetaData* metaData, const QJsonObject& valuesObj)
{
    QList<ValueDescPair> pairs = _sortedNumericPairs(valuesObj, metaData->name());

    // ArduPilot JSON stores some int8 enum codes as unsigned values (e.g. 200 for
    // BTN#_FUNCTION actuator functions added in Sub-4.5+). Reinterpret codes in
    // [128..255] as their signed equivalent so convertAndValidateRaw passes and
    // the correct bit pattern is sent back over MAVLink as INT8.
    if (metaData->type() == FactMetaData::valueTypeInt8) {
        for (auto& pair : pairs) {
            bool ok = false;
            const int code = pair.value.toInt(&ok);
            if (ok && code >= 128 && code <= 255) {
                pair.value = QString::number(code - 256);
            }
        }
    }

    setEnumFromPairs(metaData, pairs);
}

void FoxFourParameterMetaData::_applyBitmask(FactMetaData* metaData, const QJsonObject& bitmaskObj)
{
    setBitmaskFromPairs(metaData, _sortedNumericPairs(bitmaskObj, metaData->name()));
}

FactMetaData* FoxFourParameterMetaData::_lookupVGMMetaData(const QString& name, FactMetaData::ValueType_t type)
{
    auto it = _rawVGMParams.constFind(name);
    if (it == _rawVGMParams.constEnd()) {
        return nullptr;
    }

    const RawParameterData& raw = *it;
    const QJsonObject& f = raw.fields;

    auto* metaData = new FactMetaData(type, this);
    metaData->setName(name);
    metaData->setGroup(raw.group);

    const QString displayName = f.value(u"DisplayName").toString();
    if (!displayName.isEmpty()) {
        metaData->setShortDescription(displayName);
    }

    const QString description = f.value(u"Description").toString();
    if (!description.isEmpty()) {
        metaData->setLongDescription(description);
    }

    const QString units = f.value(u"Units").toString();
    if (!units.isEmpty()) {
        metaData->setRawUnits(units);
    }

    const QString category = f.value(u"User").toString();
    if (!category.isEmpty()) {
        metaData->setCategory(category);
    }

    if (f.contains(u"ReadOnly")) {
        metaData->setReadOnly(jsonToBool(f.value(u"ReadOnly")));
    }
    if (f.contains(u"RebootRequired")) {
        metaData->setVehicleRebootRequired(jsonToBool(f.value(u"RebootRequired")));
    }

    const QString increment = f.value(u"Increment").toString();
    if (!increment.isEmpty()) {
        bool ok = false;
        const double val = increment.toDouble(&ok);
        if (ok) {
            metaData->setRawIncrement(val);
        }
    }

    const QJsonObject range = f.value(u"Range").toObject();
    if (!range.isEmpty()) {
        const QString lowStr = range.value(u"low").toString();
        const QString highStr = range.value(u"high").toString();
        if (!lowStr.isEmpty()) {
            setRawConvertedValue(metaData, lowStr, &FactMetaData::setRawMin);
            setRawConvertedValue(metaData, lowStr, &FactMetaData::setRawUserMin);
        }
        if (!highStr.isEmpty()) {
            setRawConvertedValue(metaData, highStr, &FactMetaData::setRawMax);
            setRawConvertedValue(metaData, highStr, &FactMetaData::setRawUserMax);
        }
    }

    const QJsonObject valuesObj = f.value(u"Values").toObject();
    if (!valuesObj.isEmpty()) {
        _applyEnumValues(metaData, valuesObj);
    }

    const QJsonObject bitmaskObj = f.value(u"Bitmask").toObject();
    if (!bitmaskObj.isEmpty()) {
        _applyBitmask(metaData, bitmaskObj);
    }
    return metaData;
}
