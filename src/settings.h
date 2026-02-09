#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QList>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFileInfo>

struct Build {
    QString name;
    QString url;
};

class Settings
{
public:
    Settings();
    QSettings diskSettings { "xmage", "xmage-launcher-qt" };
    QString javaInstallLocation { diskSettings.value("javaInstallLocation").toString() };
    QList<Build> builds;
    QString currentBuildName;
    QStringList currentClientOptions;
    QStringList currentServerOptions;
    QString basePath;  // Base path for all installations (java/ and xmage-*/ folders)

    void setJavaInstallLocation(QString location);

    // Build management
    QString getCurrentBuildUrl() const;
    QString getBuildInstallPath(const QString &buildName) const;
    QString getCurrentBuildInstallPath() const;
    void setCurrentBuild(const QString &buildName);

private:
    void loadSettingsJson();
    void computeBasePath();
    QStringList stringToList(QString str);
};

#endif // SETTINGS_H
