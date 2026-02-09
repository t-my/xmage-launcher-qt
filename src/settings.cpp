#include "settings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

Settings::Settings()
{
    computeBasePath();
    currentBuildName = diskSettings.value("currentBuildName", "official").toString();
    loadSettingsJson();
}

void Settings::computeBasePath()
{
    basePath = QCoreApplication::applicationDirPath();
#if defined(Q_OS_MACOS)
    // On macOS, go up from .app/Contents/MacOS to the folder containing the .app
    QDir appDir(basePath);
    appDir.cdUp(); // Contents
    appDir.cdUp(); // .app
    appDir.cdUp(); // folder containing .app
    basePath = appDir.absolutePath();

    // Check if path is writable (App Translocation makes it read-only)
    QFileInfo pathInfo(basePath);
    if (!pathInfo.isWritable())
    {
        // Fall back to Application Support directory
        basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(basePath);
    }
#elif defined(Q_OS_LINUX)
    // On Linux, check if running from an AppImage (which mounts to read-only /tmp)
    QString appImagePath = qEnvironmentVariable("APPIMAGE");
    if (!appImagePath.isEmpty())
    {
        QFileInfo appImageInfo(appImagePath);
        basePath = appImageInfo.absolutePath();
    }
#endif
}

void Settings::loadSettingsJson()
{
    // Determine path to settings.json (next to the executable / .app)
    QString jsonPath;
#if defined(Q_OS_MACOS)
    QDir appDir(QCoreApplication::applicationDirPath());
    appDir.cdUp(); // Contents
    appDir.cdUp(); // .app
    appDir.cdUp(); // folder containing .app
    jsonPath = appDir.absolutePath() + "/settings.json";
#else
    jsonPath = QCoreApplication::applicationDirPath() + "/settings.json";
#endif

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        // Fallback defaults if settings.json not found
        currentClientOptions << "-Xms256m" << "-Xmx512m" << "-Dfile.encoding=UTF-8";
        currentServerOptions << "-Xms256m" << "-Xmx1g" << "-Dfile.encoding=UTF-8";
        builds.append(Build{"official", "https://xmage.today/config.json"});
        builds.append(Build{"xdhs", "https://xdhs.net/xmage/config.json"});
        builds.append(Build{"t-my", "https://t-my.github.io/mage/config.json"});
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject())
    {
        // Fallback defaults if JSON is invalid
        currentClientOptions << "-Xms256m" << "-Xmx512m" << "-Dfile.encoding=UTF-8";
        currentServerOptions << "-Xms256m" << "-Xmx1g" << "-Dfile.encoding=UTF-8";
        builds.append(Build{"official", "https://xmage.today/config.json"});
        return;
    }

    QJsonObject root = doc.object();

    // Load builds
    QJsonArray buildsArray = root.value("builds").toArray();
    for (const QJsonValue &val : buildsArray)
    {
        QJsonObject obj = val.toObject();
        QString name = obj.value("name").toString();
        QString url = obj.value("url").toString();
        if (!name.isEmpty() && !url.isEmpty())
        {
            builds.append(Build{name, url});
        }
    }

    // Load JVM options
    QString clientOpts = root.value("clientOptions").toString();
    QString serverOpts = root.value("serverOptions").toString();
    currentClientOptions = clientOpts.isEmpty()
        ? QStringList{"-Xms256m", "-Xmx512m", "-Dfile.encoding=UTF-8"}
        : stringToList(clientOpts);
    currentServerOptions = serverOpts.isEmpty()
        ? QStringList{"-Xms256m", "-Xmx1g", "-Dfile.encoding=UTF-8"}
        : stringToList(serverOpts);

    // Ensure current build exists in the list
    bool currentExists = false;
    for (const Build &build : builds)
    {
        if (build.name == currentBuildName)
        {
            currentExists = true;
            break;
        }
    }
    if (!currentExists && !builds.isEmpty())
    {
        currentBuildName = builds.first().name;
    }
}

void Settings::setJavaInstallLocation(QString location)
{
    javaInstallLocation = location;
    diskSettings.setValue("javaInstallLocation", location);
}

QString Settings::getCurrentBuildUrl() const
{
    for (const Build &build : builds)
    {
        if (build.name == currentBuildName)
        {
            return build.url;
        }
    }
    return QString();
}

QString Settings::getBuildInstallPath(const QString &buildName) const
{
    return basePath + "/builds/" + buildName;
}

QString Settings::getCurrentBuildInstallPath() const
{
    return getBuildInstallPath(currentBuildName);
}

void Settings::setCurrentBuild(const QString &buildName)
{
    for (const Build &build : builds)
    {
        if (build.name == buildName)
        {
            currentBuildName = buildName;
            diskSettings.setValue("currentBuildName", buildName);
            return;
        }
    }
}

QStringList Settings::stringToList(QString str)
{
    QStringList list;
    const QChar *data = str.constData();
    for (int i = 0; i < str.size(); i++)
    {
        QString item;
        while (i < str.size() && data[i] != ' ')
        {
            item.append(data[i]);
            i++;
        }
        if (!item.isEmpty())
        {
            list.append(item);
        }
    }
    return list;
}
