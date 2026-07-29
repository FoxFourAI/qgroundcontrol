#pragma once

#include "APMParameterMetaData.h"

class QJsonObject;

class FoxFourParameterMetaData: public APMParameterMetaData {
    Q_OBJECT
public:
    explicit FoxFourParameterMetaData(QObject *parent = nullptr);

protected:
    void parseParameterJson(const QJsonObject& json);
    FactMetaData* _lookupMetaData(const QString& name, FactMetaData::ValueType_t type);
    FactMetaData* _lookupVGMMetaData(const QString& name, FactMetaData::ValueType_t type);
    QString _groupFromParameterName(const QString &name);
    static QList<ValueDescPair> _sortedNumericPairs(const QJsonObject &obj, const QString &paramName);
    static void _applyEnumValues(FactMetaData *metaData, const QJsonObject &valuesObj);
    static void _applyBitmask(FactMetaData *metaData, const QJsonObject &bitmaskObj);
private:
    struct RawParameterData{
        QString group;
        QJsonObject fields;
    };

    QHash<QString, RawParameterData> _rawVGMParams;

};
