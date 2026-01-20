#ifndef SETTINGS_H
#define SETTINGS_H

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QSettings>

struct XMageVersion
{
    QString version;
};

Q_DECLARE_METATYPE(XMageVersion);

class Settings
{
public:
    Settings();
    QSettings diskSettings { "xmage", "xmage-launcher-qt" };
    QString xmageInstallLocation { diskSettings.value("xmageInstallLocation").toString() };
    QString javaInstallLocation { diskSettings.value("javaInstallLocation").toString() };
    QString configUrl { diskSettings.value("configUrl", "http://xmage.today/config.json").toString() };
    QStringList configUrls;
    QMap<QString, XMageVersion> xmageInstallations;
    QStringList defaultClientOptions;
    QStringList defaultServerOptions;
    QStringList currentClientOptions;
    QStringList currentServerOptions;

    void addXmageInstallation(QString location, XMageVersion &version);
    void setXmageInstallLocation(QString location);
    void setJavaInstallLocation(QString location);
    void setConfigUrl(QString url);
    void addConfigUrl(QString url);
    void removeConfigUrl(QString url);
    void setClientOptions(QString options);
    void setServerOptions(QString options);

private:
    void saveXmageInstallation();
    void saveConfigUrls();
    QStringList stringToList(QString str);
};

#endif // SETTINGS_H
