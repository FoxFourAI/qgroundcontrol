#include "ButtonList.h"
#include "Vehicle.h"

//====================ButtonInfo part========================
QString ButtonInfo::nameKey = "ButtonName";
QString ButtonInfo::defaultValueKey = "minValue";
QString ButtonInfo::activeValueKey = "maxValue";
QString ButtonInfo::servoIndexKey = "servoIndex";

QString ButtonInfo::name() const { return _name; }

void ButtonInfo::setName(const QString& newName) {
    if (_name == newName) return;
    _name = newName;
    emit nameChanged();
}

int ButtonInfo::defaultValue() const { return _defaultValue; }

void ButtonInfo::setDefaultValue(int newDefaultValue) {
    if (_defaultValue == newDefaultValue) return;
    _defaultValue = newDefaultValue;
    emit defaultValueChanged();
}

int ButtonInfo::activeValue() const { return _activeValue; }

void ButtonInfo::setActiveValue(int newActiveValue) {
    if (_activeValue == newActiveValue) return;
    _activeValue = newActiveValue;
    emit activeValueChanged();
}

int ButtonInfo::servoIndex() const
{
    return _servoIndex;
}

void ButtonInfo::setServoIndex(int newServoIndex)
{
    if (_servoIndex == newServoIndex)
        return;
    _servoIndex = newServoIndex;
    emit servoIndexChanged();
}

//====================ButtonList part========================
QString ButtonList::listKey = "ServoButtons";

ButtonList::ButtonList(Vehicle* vehicle, QObject* parent) : QObject(parent), _vehicle(vehicle) {
    _loadList();
}

ButtonList::~ButtonList()
{
    _saveList();
    clear();
}

QList<QObject*> ButtonList::buttons() { return _buttons; }

void ButtonList::clear() {
    for (auto entry : _buttons) {
        entry->deleteLater();
    }
    _buttons.clear();
    emit listChanged();
}

void ButtonList::removeAt(int indx) {
    if (indx < 0 || indx >= _buttons.size()) {
        return;
    }
    delete _buttons.takeAt(indx);
    emit listChanged();
}

void ButtonList::remove(QObject *entry)
{
    if(_buttons.contains(entry)) {
        _buttons.takeAt(_buttons.indexOf(entry))->deleteLater();
    }
}

void ButtonList::append()
{
    _buttons.append(new ButtonInfo(this));
    emit listChanged();
}

void ButtonList::_loadList() {
    QSettings settings;
    int size = settings.beginReadArray(listKey);
    if (size == 0) {
        return;
    }
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        ButtonInfo* info = new ButtonInfo(this);
        info->setName(settings.value(ButtonInfo::nameKey).toString());
        info->setServoIndex(settings.value(ButtonInfo::servoIndexKey).toInt());
        info->setDefaultValue(settings.value(ButtonInfo::defaultValueKey).toInt());
        info->setActiveValue(settings.value(ButtonInfo::activeValueKey).toInt());
        _buttons.append(info);
    }
    settings.endArray();
    emit listChanged();
    \
}

void ButtonList::_saveList() {
    QSettings settings;
    settings.beginWriteArray(listKey);
    int indx = 0;
    for (auto entry : _buttons) {
        settings.setArrayIndex(indx);
        auto cast = reinterpret_cast<ButtonInfo*>(entry);
        settings.setValue(ButtonInfo::nameKey, cast->name());
        settings.setValue(ButtonInfo::servoIndexKey, cast->servoIndex());
        settings.setValue(ButtonInfo::defaultValueKey, cast->defaultValue());
        settings.setValue(ButtonInfo::activeValueKey, cast->activeValue());
        indx++;
    }
    settings.endArray();
}


