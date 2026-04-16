#pragma once
#include <QtCore/QObject>
#include <QtCore/QSettings>

class Vehicle;

class ButtonInfo : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(int defaultValue READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged FINAL)
    Q_PROPERTY(int servoIndex READ servoIndex WRITE setServoIndex NOTIFY servoIndexChanged FINAL)
    Q_PROPERTY(int activeValue READ activeValue WRITE setActiveValue NOTIFY activeValueChanged FINAL)
public:
    ButtonInfo(QObject* parent = nullptr): QObject(parent){};
    QString name() const;
    void setName(const QString &newName);

    int defaultValue() const;
    void setDefaultValue(int newDefaultValue);

    int activeValue() const;
    void setActiveValue(int newActiveValue);

    int servoIndex() const;
    void setServoIndex(int newServoIndex);

    static QString nameKey;
    static QString servoIndexKey;
    static QString defaultValueKey;
    static QString activeValueKey;

signals:
    void nameChanged();
    void defaultValueChanged();
    void activeValueChanged();
    void servoIndexChanged();

private:
    QString _name = "Button";
    int _defaultValue = 1000; //default "closed" value for servo
    int _activeValue = 2000; //default "open" value for servo
    int _servoIndex = 1;
};

class ButtonList : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<QObject*> buttons READ buttons NOTIFY listChanged)
public:
    ButtonList(Vehicle* vehicle, QObject* parent);
    ~ButtonList();
    QList<QObject*> buttons();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void removeAt(int indx);
    Q_INVOKABLE void remove(QObject* entry);
    Q_INVOKABLE void append();
signals:
    void listChanged();
private:
    void _loadList();
    void _saveList();
    Vehicle* _vehicle = nullptr;
    QList<QObject*> _buttons;
    static QString listKey;
};
