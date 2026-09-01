#pragma once

#include <QObject>

class Vehicle;
class FoxFourAutoPilotPlugin;
class ParameterManager;
class OnboardComputersManager;
class Fact;

class AutoPilotWidget : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool parametersReady READ parametersReady NOTIFY parametersReadyChanged)
public:
    AutoPilotWidget(Vehicle* vehicle, QObject* parent = nullptr);

    struct ParameterInfo
    {
        QString name;
        bool fromVGM = false;
        Fact*& fact;

        ParameterInfo(const QString& _name, Fact*& _fact, bool _fromVGM = false)
            : name(_name), fromVGM(_fromVGM), fact(_fact)
        {
            //by default all facts should be nullptr so we handle them correctly.
            fact = nullptr;
        }
    };

    Q_INVOKABLE void refreshParameters();

    bool parametersReady() { return _parametersReady; }

signals:
    void parametersReadyChanged();

    // for internal usage. Only emits when parameter from _requiredParameters is added
    void _requiredFactChanged(Fact* fact);

protected:
    Vehicle* _vehicle = nullptr;
    FoxFourAutoPilotPlugin* _apm = nullptr;
    ParameterManager* _pm = nullptr;
    OnboardComputersManager* _ocm = nullptr;
    bool _notifyOnRefreshFail = true;

    void _setRequiredParameters(QVector<ParameterInfo> parameters);
    Fact* _getParameter(bool fromVGM, const QString& parameterName);
private slots:
    void _handleParameter(int componentId, Fact* parameter);

    void _handleVGMIdChanged(uint8_t newCompId);
    void _checkParametersReady();
    void _setParametersReady(bool ready);

private:
    QVector<ParameterInfo> _requiredParametrs;
    bool _parametersReady = false;
};
