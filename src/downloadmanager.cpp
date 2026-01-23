#include "downloadmanager.h"
#include "unzipthread.h"

DownloadManager::DownloadManager(QString downloadLocation, MainWindow *mainWindow)
    : QObject(mainWindow)
    , networkManager(new QNetworkAccessManager(this))
{
    this->downloadLocation = downloadLocation;
    this->mainWindow = mainWindow;
}

DownloadManager::~DownloadManager()
{
    if (saveFile != nullptr)
    {
        delete saveFile;
    }
    delete networkManager;
}

void DownloadManager::downloadXmage(QString configUrl)
{
    connect(networkManager, &QNetworkAccessManager::finished, this, &DownloadManager::poll_config);
    mainWindow->log("Fetching XMage version info from " + configUrl + "...");
    QUrl url(configUrl);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    networkManager->get(request);
}

void DownloadManager::downloadXmageFromUrl(const QString &url, const QString &version)
{
    xmageVersion = version.isEmpty() ? "xmage" : version;
    mainWindow->log("Downloading XMage " + xmageVersion + " from " + url);
    startDownload(QUrl(url), nullptr);
}

void DownloadManager::poll_config(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        pollFailed(reply, "Network error: " + reply->errorString());
    }
    else
    {
        QJsonObject xmageInfo = QJsonDocument::fromJson(reply->readAll()).object().value("XMage").toObject();
        QUrl url(xmageInfo.value("full").toString());
        if (url.isValid())
        {
            xmageVersion = xmageInfo.value("version").toString();
            if (xmageVersion.isEmpty())
            {
                xmageVersion = "xmage";
            }
            startDownload(url, reply);
        }
        else
        {
            pollFailed(reply, "Error parsing JSON");
        }
    }
}

void DownloadManager::pollFailed(QNetworkReply *reply, QString errorMessage)
{
    mainWindow->download_fail(errorMessage);
    if (reply)
    {
        reply->deleteLater();
    }
    this->deleteLater();
}

void DownloadManager::startDownload(QUrl url, QNetworkReply *reply)
{
    mainWindow->log("Found XMage version: " + xmageVersion);

    QString fileName;
    if (!downloadLocation.isEmpty())
    {
        QDir().mkpath(downloadLocation);
        fileName.append(downloadLocation + '/');
    }
    fileName.append("xmage.zip");

    saveFile = new QSaveFile(fileName);
    if (!saveFile->open(QIODevice::WriteOnly))
    {
        pollFailed(reply, "Failed to create file " + saveFile->fileName());
        delete saveFile;
        saveFile = nullptr;
    }
    else
    {
        mainWindow->log("Downloading XMage from " + url.toString());
        networkManager->disconnect();
        connect(networkManager, &QNetworkAccessManager::finished, this, &DownloadManager::download_complete);
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        downloadReply = networkManager->get(request);
        connect(downloadReply, &QNetworkReply::downloadProgress, mainWindow, &MainWindow::update_progress_bar);
        connect(downloadReply, &QNetworkReply::readyRead, this, &DownloadManager::save_data);
        if (reply)
        {
            reply->deleteLater();
        }
    }
}

void DownloadManager::save_data()
{
    if (!saveFile->write(downloadReply->readAll()))
    {
        mainWindow->download_fail("Error writing to file " + saveFile->fileName());
        delete saveFile;
        saveFile = nullptr;
        downloadReply->deleteLater();
        this->deleteLater();
    }
}

void DownloadManager::download_complete(QNetworkReply *reply)
{
    QString errorMessage;
    QString fileName(saveFile->fileName());
    if (reply->error() != QNetworkReply::NoError)
    {
        errorMessage = "Network error: " + reply->errorString();
    }
    else
    {
        saveFile->write(reply->readAll());
        if (saveFile->commit())
        {
            mainWindow->log("Download complete");
        }
        else
        {
            errorMessage = "Error writing to file " + fileName;
        }
    }
    delete saveFile;
    saveFile = nullptr;
    reply->deleteLater();
    this->deleteLater();
    if (errorMessage.isEmpty())
    {
        UnzipThread *unzip = new UnzipThread(fileName, downloadLocation);
        connect(unzip, &UnzipThread::log, mainWindow, &MainWindow::log);
        connect(unzip, &UnzipThread::progress, mainWindow, &MainWindow::update_progress_bar);
        connect(unzip, &UnzipThread::unzip_fail, mainWindow, &MainWindow::download_fail);
        connect(unzip, &UnzipThread::unzip_complete, mainWindow, &MainWindow::download_success);
        connect(unzip, &UnzipThread::finished, unzip, &QObject::deleteLater);
        unzip->start();
    }
    else
    {
        mainWindow->download_fail(errorMessage);
    }
}
