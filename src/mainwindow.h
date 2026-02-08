#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QRandomGenerator>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QFile>
#include <QDir>
#include "settingsdialog.h"
#include "settings.h"
#include "xmageprocess.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void update_progress_bar(qint64 bytesReceived, qint64 bytesTotal);
    void log(QString message);
    void download_fail(QString errorMessage);
    void download_success(QString installLocation);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_updateButton_clicked();
    void on_clientButton_clicked();
    void on_clientServerButton_clicked();
    void on_serverButton_clicked();
    void on_downloadButton_clicked();
    void on_actionSettings_triggered();
    void on_decksButton_clicked();
    void server_finished();

    // Java download slots
    void onJavaDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onJavaDownloadFinished(QNetworkReply *reply);
    void onJavaDownloadReadyRead();

    // Config fetch slot
    void onConfigFetched(QNetworkReply *reply);

    // Decks download slots
    void onDecksDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDecksDownloadFinished(QNetworkReply *reply);
    void onDecksDownloadReadyRead();

private:
    Ui::MainWindow *ui;
    QLabel *background;
    Settings *settings;
    QPlainTextEdit *clientConsole;
    QPlainTextEdit *serverConsole;
    XMageProcess *serverProcess = nullptr;

    // Java download members
    QNetworkAccessManager *javaNetworkManager = nullptr;
    QNetworkReply *javaDownloadReply = nullptr;
    QSaveFile *javaSaveFile = nullptr;
    QString javaBaseUrl;
    QString javaVersion;
    bool javaDownloading = false;
    bool downloadXmageAfterJava = false;  // Flag to continue with XMage download after Java

    // Config fetch members
    QNetworkAccessManager *configNetworkManager = nullptr;
    bool configFetching = false;

    // Decks download members
    QNetworkAccessManager *decksNetworkManager = nullptr;
    QNetworkReply *decksDownloadReply = nullptr;
    QSaveFile *decksSaveFile = nullptr;
    bool decksDownloading = false;

    bool validateJavaSettings();
    bool findClientJar(QString *jar);
    bool findServerJar(QString *jar);
    void launchClient();
    void launchServer();
    void stopServer();

    // Java download methods
    void startJavaDownload();
    void startXmageDownload(const QJsonObject &config);
    QString getJavaPlatformSuffix();
    void extractJava(const QString &filePath);
    void javaDownloadComplete();
    void javaDownloadFailed(const QString &error);

    // Config methods
    void fetchConfig();
    void updateLaunchReadiness();
    bool loadCachedConfig(QJsonObject *config);
};
#endif // MAINWINDOW_H
