#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QDir>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include "mainwindow.h"

class DownloadManager : public QObject
{
    Q_OBJECT

public:
    DownloadManager(QString downloadLocation, MainWindow *mainWindow);
    ~DownloadManager();
    void downloadXmage(QString configUrl);
    void downloadXmageFromUrl(const QString &url, const QString &version);

private:
    QString downloadLocation;
    QString xmageVersion;
    MainWindow *mainWindow;
    QNetworkAccessManager *networkManager;
    QNetworkReply *downloadReply;
    QSaveFile *saveFile = nullptr;

    void pollFailed(QNetworkReply *reply, QString errorMessage);
    void startDownload(QUrl url, QNetworkReply *reply);

private slots:
    void poll_config(QNetworkReply *reply);
    void download_complete(QNetworkReply *reply);
    void save_data();
};

#endif // DOWNLOADMANAGER_H
