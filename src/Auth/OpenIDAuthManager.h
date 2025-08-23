/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetworkAuth/QOAuth2AuthorizationCodeFlow>
#include <QtNetworkAuth/QOAuthUriSchemeReplyHandler>
#include <QtNetwork/QNetworkAccessManager>

class OpenIDAuthManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AuthState authState READ authState NOTIFY authStateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString statusColor READ statusColor NOTIFY statusColorChanged)
    Q_PROPERTY(QString accessToken READ accessToken NOTIFY authenticationCompleted)
    Q_PROPERTY(QString userName READ userName NOTIFY authenticationCompleted)
    
    // Enum constants for QML access
    Q_PROPERTY(AuthState NotAuthenticated READ getNotAuthenticated CONSTANT)
    Q_PROPERTY(AuthState Authenticating READ getAuthenticating CONSTANT)
    Q_PROPERTY(AuthState Authenticated READ getAuthenticated CONSTANT)
    Q_PROPERTY(AuthState Authorizing READ getAuthorizing CONSTANT)
    Q_PROPERTY(AuthState Authorized READ getAuthorized CONSTANT)
    Q_PROPERTY(AuthState Error READ getError CONSTANT)

public:
    enum AuthState {
        NotAuthenticated,
        Authenticating,
        Authenticated,
        Authorizing,
        Authorized,
        Error
    };
    Q_ENUM(AuthState)

    static OpenIDAuthManager* instance();

    explicit OpenIDAuthManager(QObject *parent = nullptr);
    ~OpenIDAuthManager() override;

    AuthState authState() const { return _authState; }
    QString statusText() const { return _statusText; }
    QString statusColor() const { return _statusColor; }
    QString accessToken() const { return _accessToken; }
    QString userName() const { return _userName; }

    Q_INVOKABLE void login();
    Q_INVOKABLE void authorize();
    Q_INVOKABLE void logout();
    
    // Enum constant getters for QML
    AuthState getNotAuthenticated() const { return NotAuthenticated; }
    AuthState getAuthenticating() const { return Authenticating; }
    AuthState getAuthenticated() const { return Authenticated; }
    AuthState getAuthorizing() const { return Authorizing; }
    AuthState getAuthorized() const { return Authorized; }
    AuthState getError() const { return Error; }

signals:
    void authStateChanged();
    void statusTextChanged();
    void statusColorChanged();
    void authenticationCompleted(const QString& token);
    void authorizationCompleted();

private slots:
    void onAuthenticationFinished();
    void onAuthenticationError(const QString& error);

private:
    void setAuthState(AuthState state);
    void updateStatusDisplay();
    void initializeOAuth();
    void requestArmingServiceUnlock();

    QOAuth2AuthorizationCodeFlow* _oauth2Flow;
    QOAuthUriSchemeReplyHandler* _replyHandler;
    QNetworkAccessManager* _networkManager;
    
    AuthState _authState;
    QString _statusText;
    QString _statusColor;
    QString _accessToken;
    QString _userName;

    // OAuth2 Configuration - Avengers-Edge-FoxFour
    static constexpr const char* CLIENT_ID = "7ec11e7a-18a6-4f79-b665-4d900bbeca41";
    static constexpr const char* CLIENT_SECRET = "";
    static constexpr const char* AUTHORIZATION_URL = "https://stg.delta.mil.gov.ua/auth/oauth2/authorize";
    static constexpr const char* ACCESS_TOKEN_URL = "https://stg.delta.mil.gov.ua/auth/oauth2/token";
    static constexpr const char* USERINFO_URL = "https://stg.delta.mil.gov.ua/auth/userinfo";
    static constexpr const char* ARMING_SERVICE_URL = "https://stg.delta.mil.gov.ua/arming-service/v1/unlock";
    static constexpr const char* WELL_KNOWN_URL = "https://stg.delta.mil.gov.ua/auth/.well-known/openid-configuration";
    static constexpr const char* SCOPE = "openid profile";
    static constexpr const char* REDIRECT_URI = "f4-qgroundcontrol://auth/callback";
    static constexpr const char* LOGOUT_REDIRECT_URI = "f4-qgroundcontrol://auth/logout";
    static constexpr int CALLBACK_PORT = 8080;
};
