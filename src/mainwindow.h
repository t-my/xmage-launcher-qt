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
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QFile>
#include <QDir>
#include "aboutdialog.h"
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
    void load_browser(const QString &url);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_updateButton_clicked();
    void on_clientButton_clicked();
    void on_clientServerButton_clicked();
    void on_serverButton_clicked();
    void on_downloadButton_clicked();
    void on_javaButton_clicked();
    void on_actionSettings_triggered();
    void on_actionQuit_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionAbout_triggered();
    void on_actionForum_triggered();
    void on_actionGitHub_XMage_triggered();
    void on_actionGitHub_xmage_launcher_qt_triggered();
    void on_actionWebsite_triggered();
    void server_finished();

    // Java download slots
    void onJavaDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onJavaDownloadFinished(QNetworkReply *reply);
    void onJavaDownloadReadyRead();

    // Config fetch slot
    void onConfigFetched(QNetworkReply *reply);

private:
    Ui::MainWindow *ui;
    QLabel *background;
    Settings *settings;
    QPlainTextEdit *clientConsole;
    QPlainTextEdit *serverConsole;
    AboutDialog *aboutDialog;
    XMageProcess *serverProcess = nullptr;

    // Java download members
    QNetworkAccessManager *javaNetworkManager = nullptr;
    QNetworkReply *javaDownloadReply = nullptr;
    QSaveFile *javaSaveFile = nullptr;
    QString javaBaseUrl;
    QString javaVersion;
    bool javaDownloading = false;

    // Config fetch members
    QNetworkAccessManager *configNetworkManager = nullptr;
    bool configFetching = false;

    bool validateJavaSettings();
    bool findClientJar(QString *jar);
    bool findServerJar(QString *jar);
    void launchClient();
    void launchServer();
    void stopServer();

    // Java download methods
    void startJavaDownload();
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
