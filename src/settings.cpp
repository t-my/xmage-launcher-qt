#include "settings.h"

Settings::Settings()
{
    defaultClientOptions << "-Xms256m" << "-Xmx512m" << "-Dfile.encoding=UTF-8";
    defaultServerOptions << "-Xms256m" << "-Xmx1g" << "-Dfile.encoding=UTF-8";
    if (diskSettings.contains("clientOptions"))
    {
        currentClientOptions = diskSettings.value("clientOptions").toStringList();
    }
    else
    {
        currentClientOptions = defaultClientOptions;
    }
    if (diskSettings.contains("serverOptions"))
    {
        currentServerOptions = diskSettings.value("serverOptions").toStringList();
    }
    else
    {
        currentServerOptions = defaultServerOptions;
    }

    computeBasePath();
    loadBuilds();
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

void Settings::loadBuilds()
{
    // Load current build name
    currentBuildName = diskSettings.value("currentBuildName", "official").toString();

    // Load custom builds from settings
    int buildCount = diskSettings.beginReadArray("builds");
    for (int i = 0; i < buildCount; ++i)
    {
        diskSettings.setArrayIndex(i);
        Build build;
        build.name = diskSettings.value("name").toString();
        build.url = diskSettings.value("url").toString();
        if (!build.name.isEmpty() && !build.url.isEmpty())
        {
            builds.append(build);
        }
    }
    diskSettings.endArray();

    // Ensure default builds are always present
    bool hasOfficial = false;
    bool hasXdhs = false;
    for (const Build &build : builds)
    {
        if (build.name == "official") hasOfficial = true;
        if (build.name == "xdhs") hasXdhs = true;
    }

    if (!hasOfficial)
    {
        builds.prepend({"official", "https://xmage.today/config.json"});
    }
    if (!hasXdhs)
    {
        // Insert xdhs as second item if official is first
        int insertPos = (builds.isEmpty() || builds.first().name != "official") ? 0 : 1;
        builds.insert(insertPos, {"xdhs", "https://xdhs.net/xmage/config.json"});
    }

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

void Settings::saveBuilds()
{
    diskSettings.beginWriteArray("builds");
    for (int i = 0; i < builds.size(); ++i)
    {
        diskSettings.setArrayIndex(i);
        diskSettings.setValue("name", builds[i].name);
        diskSettings.setValue("url", builds[i].url);
    }
    diskSettings.endArray();
}

void Settings::setJavaInstallLocation(QString location)
{
    javaInstallLocation = location;
    diskSettings.setValue("javaInstallLocation", location);
}

void Settings::setClientOptions(QString options)
{
    currentClientOptions = stringToList(options);
    diskSettings.setValue("clientOptions", currentClientOptions);
}

void Settings::setServerOptions(QString options)
{
    currentServerOptions = stringToList(options);
    diskSettings.setValue("serverOptions", currentServerOptions);
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

void Settings::addBuild(const QString &name, const QString &url)
{
    // Check if build with this name already exists
    for (const Build &build : builds)
    {
        if (build.name == name)
        {
            return;
        }
    }
    builds.append({name, url});
    saveBuilds();
}

void Settings::removeBuild(const QString &name)
{
    // Don't allow removing default builds
    if (isDefaultBuild(name))
    {
        return;
    }

    for (int i = 0; i < builds.size(); ++i)
    {
        if (builds[i].name == name)
        {
            builds.removeAt(i);
            saveBuilds();

            // If we removed the current build, switch to first available
            if (currentBuildName == name && !builds.isEmpty())
            {
                setCurrentBuild(builds.first().name);
            }
            return;
        }
    }
}

bool Settings::isDefaultBuild(const QString &name) const
{
    return name == "official" || name == "xdhs";
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
