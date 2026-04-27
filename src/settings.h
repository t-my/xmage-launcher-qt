#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QStringList>
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
    QString javaInstallLocation;
    QList<Build> builds;
    QString currentBuildName;
    QStringList currentClientOptions;
    QStringList currentServerOptions;
    QString basePath;  // Base path for all installations (java/ and xmage-*/ folders)
    QString loadError;  // Non-empty if settings.json failed to load

    void setJavaInstallLocation(QString location);

    // Build management
    QString getCurrentBuildUrl() const;
    QString getBuildInstallPath(const QString &buildName) const;
    QString getCurrentBuildInstallPath() const;
    void setCurrentBuild(const QString &buildName);

private:
    void loadSettingsJson();
    void loadUserSettings();
    void saveUserSettings();
    void computeBasePath();
    QStringList stringToList(QString str);
};

#endif // SETTINGS_H
