#include "settings.h"

Settings::Settings()
{
    qRegisterMetaType<XMageVersion>();
    QJsonArray jsonArray = diskSettings.value("xmageInstallations").toJsonArray();
    for (int i = 0; i < jsonArray.size(); i++)
    {
        QJsonObject jsonObject = jsonArray.at(i).toObject();
        QString location = jsonObject.value("installLocation").toString();
        XMageVersion versionInfo;
        QString savedVersion = jsonObject.value("version").toString();
        if (savedVersion.isEmpty())
        {
            versionInfo.version = "Unknown";
        }
        else
        {
            versionInfo.version = savedVersion;
        }
        xmageInstallations.insert(location, versionInfo);
    }

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

    // Load config URLs, ensure defaults are always present
    configUrls = diskSettings.value("configUrls").toStringList();
    if (!configUrls.contains("http://xmage.today/config.json"))
    {
        configUrls.prepend("http://xmage.today/config.json");
    }
    if (!configUrls.contains("http://xdhs.net/xmage/config.json"))
    {
        configUrls.insert(1, "http://xdhs.net/xmage/config.json");
    }
    // Ensure selected URL is in the list
    if (!configUrls.contains(configUrl))
    {
        configUrls.append(configUrl);
    }
}

void Settings::addXmageInstallation(QString location, XMageVersion &versionInfo)
{
    xmageInstallations.insert(location, versionInfo);
    saveXmageInstallation();
}

void Settings::setXmageInstallLocation(QString location)
{
    xmageInstallLocation = location;
    diskSettings.setValue("xmageInstallLocation", location);
}

void Settings::setJavaInstallLocation(QString location)
{
    javaInstallLocation = location;
    diskSettings.setValue("javaInstallLocation", location);
}

void Settings::saveXmageInstallation()
{
    QJsonArray jsonArray;
    QMapIterator<QString, XMageVersion> i(xmageInstallations);
    while (i.hasNext())
    {
        i.next();
        const XMageVersion &curValue = i.value();
        QJsonObject jsonObject;
        jsonObject.insert("installLocation", i.key());
        jsonObject.insert("version", curValue.version);
        jsonArray.append(jsonObject);
    }
    diskSettings.setValue("xmageInstallations", jsonArray);
}

void Settings::setConfigUrl(QString url)
{
    configUrl = url;
    diskSettings.setValue("configUrl", url);
}

void Settings::addConfigUrl(QString url)
{
    if (!configUrls.contains(url))
    {
        configUrls.append(url);
        saveConfigUrls();
    }
}

void Settings::removeConfigUrl(QString url)
{
    // Don't allow removing the default URL
    if (url != "http://xmage.today/config.json" && configUrls.removeAll(url) > 0)
    {
        saveConfigUrls();
    }
}

void Settings::saveConfigUrls()
{
    diskSettings.setValue("configUrls", configUrls);
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
