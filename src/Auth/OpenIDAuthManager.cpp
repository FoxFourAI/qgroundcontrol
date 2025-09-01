/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "OpenIDAuthManager.h"
#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>
#include <QtGui/QDesktopServices>
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>
#include <QGCLoggingCategory.h>


QGC_LOGGING_CATEGORY(OpenIDAuthLog, "qgc.auth.openid")

OpenIDAuthManager* OpenIDAuthManager::instance()
{
    static OpenIDAuthManager* _instance = nullptr;
    if (!_instance) {
        _instance = new OpenIDAuthManager(qApp);
    }
    return _instance;
}

OpenIDAuthManager::OpenIDAuthManager(QObject *parent)
    : QObject(parent)
    , _oauth2Flow(nullptr)
    , _replyHandler(nullptr)
    , _networkManager(new QNetworkAccessManager(this))
    , _authState(NotAuthenticated)
    , _statusText("Login")
    , _statusColor("white")
    ,_modelDecryptionManager(new UavModelDecryptionManager(this))
{
    connect(_modelDecryptionManager, &UavModelDecryptionManager::decryptionCompleted, this, &OpenIDAuthManager::onDecryptionFinished);
    connect(_modelDecryptionManager, &UavModelDecryptionManager::decryptionFailed, this, &OpenIDAuthManager::onDecryptionError);

    initializeOAuth();
}

OpenIDAuthManager::~OpenIDAuthManager()
{
    if (_replyHandler) {
        delete _replyHandler;
    }

    if (_networkManager) {
        delete _networkManager;
    }

    if (_modelDecryptionManager) {
        delete _modelDecryptionManager;
    }
}


void OpenIDAuthManager::initializeOAuth()
{
    _oauth2Flow = new QOAuth2AuthorizationCodeFlow(this);
    _replyHandler = new QOAuthUriSchemeReplyHandler(QUrl{"f4-qgroundcontrol://auth/callback"}, this);

    _oauth2Flow->setReplyHandler(_replyHandler);
    _oauth2Flow->setNetworkAccessManager(_networkManager);

    //FIXME: hard-code
    _oauth2Flow->setAuthorizationUrl(QUrl("https://stg.delta.mil.gov.ua/auth/oauth2/authorize"));
    _oauth2Flow->setAccessTokenUrl(QUrl("https://stg.delta.mil.gov.ua/auth/oauth2/token"));
    _oauth2Flow->setClientIdentifier("7ec11e7a-18a6-4f79-b665-4d900bbeca41");
    _oauth2Flow->setScope("openid profile");

    // Enable PKCE
    _oauth2Flow->setPkceMethod(QOAuth2AuthorizationCodeFlow::PkceMethod::S256);

    connect(_oauth2Flow, &QOAuth2AuthorizationCodeFlow::statusChanged,
            this, [this](QAbstractOAuth::Status status) {
        qCDebug(OpenIDAuthLog) << "OAuth status changed:" << static_cast<int>(status);
        switch (status) {
        case QAbstractOAuth::Status::NotAuthenticated:
            setAuthState(NotAuthenticated);
            break;
        case QAbstractOAuth::Status::TemporaryCredentialsReceived:
            setAuthState(Authenticating);
            break;
        case QAbstractOAuth::Status::Granted:
            onAuthenticationFinished();
            break;
        case QAbstractOAuth::Status::RefreshingToken:
            setAuthState(Authenticating);
            break;
        }
    });

    connect(_oauth2Flow, &QOAuth2AuthorizationCodeFlow::error,
            this, [this](const QString& error, const QString& errorDescription, const QUrl& uri) {
        Q_UNUSED(uri)
        qCWarning(OpenIDAuthLog) << "OAuth error:" << error << errorDescription;
        onAuthenticationError(error + ": " + errorDescription);
    });
}

void OpenIDAuthManager::login()
{
    qCWarning(OpenIDAuthLog) << "Starting login process";
    setAuthState(Authenticating);

    // Uncomment this for test login
    // setAuthState(Authenticated);
    // _accessToken = "123456";
    // emit authenticationCompleted(_accessToken);
    // return;

            // Connect to see the URL it wants to open in browser
    connect(_oauth2Flow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            this, [](const QUrl &url) {
                qWarning() << "Grant triggered, authorization URL:" << url;
                QDesktopServices::openUrl(url);   // <--- actually opens browser
            });

    qCWarning(OpenIDAuthLog) << "Calling _oauth2Flow->grant() now...";
    _oauth2Flow->grant();
    qCWarning(OpenIDAuthLog) << "_oauth2Flow->grant() call finished (async).";
}

void OpenIDAuthManager::authorize()
{
    if (_authState != Authenticated) {
        qCWarning(OpenIDAuthLog) << "Cannot authorize: not authenticated";
        return;
    }

    if (_accessToken.isEmpty()) {
        qCWarning(OpenIDAuthLog) << "Cannot authorize: no access token";
        setAuthState(Error);
        return;
    }

    qCDebug(OpenIDAuthLog) << "Starting authorization process with arming service";
    setAuthState(Authorizing);

    // Uncomment this for test authorization
    // _modelDecryptionToken.resize(128, 'c');
    // setAuthState(Authorized);
    // emit authorizationCompleted();
    // return;

    // Make real POST request to arming service
    QUrl armingServiceUrl(ARMING_SERVICE_URL);
    QNetworkRequest request(armingServiceUrl);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(_accessToken).toUtf8());
    request.setRawHeader("Content-Type", "application/json");

    // Prepare JSON body with enrollment_uid
    QJsonObject jsonBody;
    // FIXME: this is currently hard-coded. Should be ready
    jsonBody["enrollment_uid"] = "_uM4KvLITvL6YkYC";

    QJsonDocument jsonDoc(jsonBody);
    QByteArray postData = jsonDoc.toJson();

    QNetworkReply* reply = _networkManager->post(request, postData);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            // TODO: obtain the 128-byte model decryption token from the response. For now, just filling in with some string
            _modelDecryptionToken.resize(128, 's');
            qWarning() << "Arming service authorization successful:" << responseData;

            setAuthState(Authorized);
            emit authorizationCompleted();
        } else {
            qCWarning(OpenIDAuthLog) << "Failed to authorize with arming service:" << reply->errorString();
            qCWarning(OpenIDAuthLog) << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray errorResponse = reply->readAll();
            if (!errorResponse.isEmpty()) {
                qCWarning(OpenIDAuthLog) << "Error response body:" << errorResponse;
            }
            setAuthState(Error);
        }
    });
}

void OpenIDAuthManager::decrypt_model()
{
    if (_authState != Authorized) {
        qCWarning(OpenIDAuthLog) << "Cannot decrypt model: not authorized";
        return;
    }

    if (_modelDecryptionToken.isEmpty()) {
        qCWarning(OpenIDAuthLog) << "Cannot decrypt model: no token";
        setAuthState(Error);
        return;
    }
    setAuthState(ModelDecrypting);

    _modelDecryptionManager->startModelDecryption(_modelDecryptionToken);
}


void OpenIDAuthManager::logout()
{
    qCDebug(OpenIDAuthLog) << "Logging out";
    _oauth2Flow->setToken("");
    _accessToken.clear();
    setAuthState(NotAuthenticated);
}

void OpenIDAuthManager::onAuthenticationFinished()
{
    _accessToken = _oauth2Flow->token();
    qCDebug(OpenIDAuthLog) << "Authentication completed, token received";

    if (!_accessToken.isEmpty()) {
        // Request arming service unlock with the access token
        requestArmingServiceUnlock();
    } else {
        qCWarning(OpenIDAuthLog) << "Received empty token";
        setAuthState(Error);
    }
}

void OpenIDAuthManager::onAuthenticationError(const QString& error)
{
    qCWarning(OpenIDAuthLog) << "Authentication error:" << error;
    setAuthState(Error);
}

void OpenIDAuthManager::onDecryptionFinished() {
    qCDebug(OpenIDAuthLog) << "Model decryption finished";
    setAuthState(ModelDecrypted);
}
void OpenIDAuthManager::onDecryptionError(const QString& error) {
    qCWarning(OpenIDAuthLog) << "Model decription error:" << error;
    setAuthState(Error);
}


void OpenIDAuthManager::setAuthState(AuthState state)
{
    if (_authState != state) {
        _authState = state;
        updateStatusDisplay();
        emit authStateChanged();
    }
}

void OpenIDAuthManager::updateStatusDisplay()
{
    QString newText;
    QString newColor;

    switch (_authState) {
    case NotAuthenticated:
        newText = "Login";
        newColor = "white";
        break;
    case Authenticating:
        newText = "Logging in...";
        newColor = "yellow";
        break;
    case Authenticated:
        newText = "Authorize";
        newColor = "white";
        break;
    case Authorizing:
        newText = "Authorizing...";
        newColor = "yellow";
        break;
    case Authorized:
        newText = "Decrypt";
        newColor = "#95F792";
        break;
    case ModelDecrypting:
        newText = "Decrypting...";
        newColor = "yellow";
        break;
    case ModelDecrypted:
        newText = "Decrypted!";
        newColor = "#95F792" ;
        break;
    case Error:
        newText = "Error";
        newColor = "red";
        break;
    }

    if (_statusText != newText) {
        _statusText = newText;
        emit statusTextChanged();
    }

    if (_statusColor != newColor) {
        _statusColor = newColor;
        emit statusColorChanged();
    }
}

void OpenIDAuthManager::requestArmingServiceUnlock()
{
    if (_accessToken.isEmpty()) {
        qCWarning(OpenIDAuthLog) << "Cannot request arming service unlock: no access token";
        setAuthState(Error);
        return;
    }

    QUrl armingServiceUrl(ARMING_SERVICE_URL);
    QNetworkRequest request(armingServiceUrl);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(_accessToken).toUtf8());
    request.setRawHeader("Content-Type", "application/json");

    // Prepare JSON body with enrollment_uid
    QJsonObject jsonBody;
    jsonBody["enrollment_uid"] = "_uM4KvLITvL6YkYC";

    QJsonDocument jsonDoc(jsonBody);
    QByteArray postData = jsonDoc.toJson();

    QNetworkReply* reply = _networkManager->post(request, postData);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();

            // For now, assume successful response means we're authenticated
            // Later we can parse the response for more specific handling
            _userName = "Authorized User"; // Set a default username

            qCInfo(OpenIDAuthLog) << "Arming service unlock successful";
            setAuthState(Authenticated);
            emit authenticationCompleted(_accessToken);
        } else {
            qCWarning(OpenIDAuthLog) << "Failed to unlock arming service:" << reply->errorString();
            qCWarning(OpenIDAuthLog) << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray errorResponse = reply->readAll();
            if (!errorResponse.isEmpty()) {
                qCWarning(OpenIDAuthLog) << "Error response body:" << errorResponse;
            }
            setAuthState(Error);
        }
    });
}
