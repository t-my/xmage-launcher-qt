#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "downloadmanager.h"
#include "zipextractthread.h"
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , background(new QLabel(this))
    , settings(new Settings)
    , clientConsole(new QPlainTextEdit)
    , serverConsole(new QPlainTextEdit)
{
    ui->setupUi(this);
    ui->progressBar->hide();
    ui->progressBar->setAlignment(Qt::AlignCenter);
    this->setFixedSize(this->size());
    QString imageName = QString::number(QRandomGenerator::global()->bounded(1, 17)) + ".jpg";
    QPixmap backgroundImage(":/backgrounds/" + imageName);
    background->setGeometry(0, ui->menubar->size().height(), this->size().width(), this->size().height() - ui->menubar->size().height());
    background->setPixmap(backgroundImage.scaled(background->size()));
    background->lower();
    clientConsole->setWindowTitle("XMage Client Console");
    clientConsole->setWindowIcon(this->windowIcon());
    clientConsole->setGeometry(0, 0, 860, 480);
    clientConsole->setStyleSheet("background-color:black; color:white;");
    clientConsole->setReadOnly(true);
    serverConsole->setWindowTitle("XMage Server Console");
    serverConsole->setWindowIcon(this->windowIcon());
    serverConsole->setGeometry(0, 0, 860, 480);
    serverConsole->setStyleSheet("background-color:black; color:white;");
    serverConsole->setReadOnly(true);
    aboutDialog = new AboutDialog(this);

    // Set Java download path to the same folder as the launcher
    javaDownloadPath = QCoreApplication::applicationDirPath();
#if defined(Q_OS_MACOS)
    // On macOS, go up from .app/Contents/MacOS to the folder containing the .app
    QDir appDir(javaDownloadPath);
    appDir.cdUp(); // Contents
    appDir.cdUp(); // .app
    appDir.cdUp(); // folder containing .app
    javaDownloadPath = appDir.absolutePath();
#elif defined(Q_OS_LINUX)
    // On Linux, check if running from an AppImage (which mounts to read-only /tmp)
    QString appImagePath = qEnvironmentVariable("APPIMAGE");
    if (!appImagePath.isEmpty())
    {
        QFileInfo appImageInfo(appImagePath);
        javaDownloadPath = appImageInfo.absolutePath();
    }
#endif
}

MainWindow::~MainWindow()
{
    if (javaSaveFile != nullptr)
    {
        delete javaSaveFile;
    }
    if (javaNetworkManager != nullptr)
    {
        delete javaNetworkManager;
    }
    delete aboutDialog;
    delete settings;
    delete background;
    delete ui;
}

void MainWindow::log(QString message)
{
    ui->log->appendPlainText(message);
}

void MainWindow::download_fail(QString errorMessage)
{
    log(errorMessage);
    ui->progressBar->hide();
    ui->progressBar->setValue(0);
    ui->downloadButton->setEnabled(true);
    ui->updateButton->setEnabled(true);
}

void MainWindow::download_success(QString installLocation, XMageVersion versionInfo)
{
    // Check if mage-client exists directly or in a subfolder
    QString xmagePath = installLocation;
    QDir dir(installLocation);

    if (!QDir(installLocation + "/mage-client").exists())
    {
        // Look for mage-client in subfolders (zip might have root folder)
        QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &entry : entries)
        {
            if (QDir(installLocation + "/" + entry + "/mage-client").exists())
            {
                xmagePath = installLocation + "/" + entry;
                break;
            }
        }
    }

    log("Setting XMage path to: " + xmagePath);
    settings->setXmageInstallLocation(xmagePath);
    settings->addXmageInstallation(xmagePath, versionInfo);
    ui->progressBar->hide();
    ui->progressBar->setValue(0);
    ui->downloadButton->setEnabled(true);
    ui->updateButton->setEnabled(true);
}

void MainWindow::on_updateButton_clicked()
{
    if (settings->xmageInstallLocation.isEmpty())
    {
        QMessageBox::warning(this, "XMage location not set", "Please set XMage location in settings.");
        SettingsDialog *settingsDialog = new SettingsDialog(settings, this);
        settingsDialog->showXmageSettings();
        settingsDialog->open();
    }
    else
    {
        XMageVersion versionInfo;
        versionInfo.version = "Unknown";
        versionInfo.branch = UNKNOWN;
        if (settings->xmageInstallations.contains(settings->xmageInstallLocation))
        {
            versionInfo = settings->xmageInstallations.value(settings->xmageInstallLocation);
        }
        if (versionInfo.branch == UNKNOWN)
        {
            QStringList branches;
            branches << "Stable" << "Beta";
            bool dialogAccepted;
            QString selectedBranch = QInputDialog::getItem(this, "Select Branch", "XMage Branch", branches, 0, false, &dialogAccepted);
            if (dialogAccepted)
            {
                if (selectedBranch == branches.at(1))
                {
                    versionInfo.branch = BETA;
                }
                else
                {
                    versionInfo.branch = STABLE;
                }
            }
        }
        if (versionInfo.branch != UNKNOWN)
        {
            ui->downloadButton->setEnabled(false);
            ui->updateButton->setEnabled(false);
            ui->progressBar->show();
            DownloadManager *downloadManager = new DownloadManager(settings->xmageInstallLocation, this);
            downloadManager->updateXmage(versionInfo);
        }
    }
}

void MainWindow::on_clientButton_clicked()
{
    launchClient();
}

void MainWindow::on_serverButton_clicked()
{
    if (serverProcess == nullptr)
    {
        launchServer();
    }
    else
    {
        stopServer();
    }
}

void MainWindow::on_clientServerButton_clicked()
{
    launchServer();
    launchClient();
}

void MainWindow::on_downloadButton_clicked()
{
    QStringList branches;
    branches << "Stable" << "Beta";
    bool dialogAccepted;
    QString selectedBranch = QInputDialog::getItem(this, "Select Branch", "XMage Branch", branches, 0, false, &dialogAccepted);
    if (dialogAccepted)
    {
        // Download to current folder/xmage
        QString downloadLocation = javaDownloadPath + "/xmage";
        QDir().mkpath(downloadLocation);

        ui->downloadButton->setEnabled(false);
        ui->updateButton->setEnabled(false);
        ui->progressBar->show();
        log("Downloading XMage to: " + downloadLocation);
        DownloadManager *downloadManager = new DownloadManager(downloadLocation, this);
        if (selectedBranch == branches.at(1))
        {
            downloadManager->downloadBeta();
        }
        else
        {
            downloadManager->downloadStable();
        }
    }
}

void MainWindow::on_javaButton_clicked()
{
    if (javaDownloading)
    {
        log("Java download already in progress...");
        return;
    }
    fetchJavaConfig();
}

void MainWindow::fetchJavaConfig()
{
    javaDownloading = true;
    ui->javaButton->setEnabled(false);
    log("Fetching Java download info...");

    if (javaNetworkManager == nullptr)
    {
        javaNetworkManager = new QNetworkAccessManager(this);
    }
    else
    {
        javaNetworkManager->disconnect();
    }

    connect(javaNetworkManager, &QNetworkAccessManager::finished, this, &MainWindow::onJavaConfigFetched);
    QNetworkRequest request(QUrl("http://xmage.today/config.json"));
    javaNetworkManager->get(request);
}

void MainWindow::onJavaConfigFetched(QNetworkReply *reply)
{
    javaNetworkManager->disconnect();

    if (reply->error() != QNetworkReply::NoError)
    {
        javaDownloadFailed("Failed to fetch config: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject root = doc.object();
    QJsonObject javaObj = root.value("java").toObject();

    javaVersion = javaObj.value("version").toString();
    javaBaseUrl = javaObj.value("location").toString();

    reply->deleteLater();

    if (javaBaseUrl.isEmpty())
    {
        javaDownloadFailed("No Java download URL in config");
        return;
    }

    startJavaDownload();
}

QString MainWindow::getJavaPlatformSuffix()
{
#if defined(Q_OS_WIN)
    return "windows-x64.zip";
#elif defined(Q_OS_MACOS)
    return "macosx-x64.tar.gz";
#elif defined(Q_OS_LINUX)
    return "linux-x64.tar.gz";
#else
    return "linux-x64.tar.gz";
#endif
}

void MainWindow::startJavaDownload()
{
    QString platform = getJavaPlatformSuffix();
    QString fullUrl = javaBaseUrl + platform;
    QString fileName = javaDownloadPath + "/java-" + javaVersion + "-" + platform;

    log("Downloading Java " + javaVersion + " from " + fullUrl);

    javaSaveFile = new QSaveFile(fileName);
    if (!javaSaveFile->open(QIODevice::WriteOnly))
    {
        javaDownloadFailed("Failed to create file: " + fileName);
        delete javaSaveFile;
        javaSaveFile = nullptr;
        return;
    }

    ui->progressBar->setValue(0);
    ui->progressBar->show();

    QUrl url(fullUrl);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    connect(javaNetworkManager, &QNetworkAccessManager::finished, this, &MainWindow::onJavaDownloadFinished);
    javaDownloadReply = javaNetworkManager->get(request);
    connect(javaDownloadReply, &QNetworkReply::downloadProgress, this, &MainWindow::onJavaDownloadProgress);
    connect(javaDownloadReply, &QNetworkReply::readyRead, this, &MainWindow::onJavaDownloadReadyRead);
}

void MainWindow::onJavaDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0)
    {
        int percent = static_cast<int>((bytesReceived * 100) / bytesTotal);
        ui->progressBar->setValue(percent);
        ui->progressBar->setFormat(QString("Downloading Java... %1 MB / %2 MB")
                                       .arg(bytesReceived / 1048576.0, 0, 'f', 1)
                                       .arg(bytesTotal / 1048576.0, 0, 'f', 1));
    }
}

void MainWindow::onJavaDownloadReadyRead()
{
    if (javaSaveFile && javaDownloadReply)
    {
        javaSaveFile->write(javaDownloadReply->readAll());
    }
}

void MainWindow::onJavaDownloadFinished(QNetworkReply *reply)
{
    javaNetworkManager->disconnect();
    javaDownloadReply = nullptr;

    if (reply->error() != QNetworkReply::NoError)
    {
        if (javaSaveFile)
        {
            javaSaveFile->cancelWriting();
            delete javaSaveFile;
            javaSaveFile = nullptr;
        }
        javaDownloadFailed("Download failed: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    // Write any remaining data
    if (javaSaveFile)
    {
        javaSaveFile->write(reply->readAll());
        QString fileName = javaSaveFile->fileName();

        if (javaSaveFile->commit())
        {
            log("Download complete. Extracting...");
            ui->progressBar->setValue(100);
            ui->progressBar->setFormat("Extracting...");
            extractJava(fileName);
        }
        else
        {
            javaDownloadFailed("Failed to save file");
        }
        delete javaSaveFile;
        javaSaveFile = nullptr;
    }

    reply->deleteLater();
}

void MainWindow::extractJava(const QString &filePath)
{
    QString extractPath = javaDownloadPath + "/java";
    QDir().mkpath(extractPath);

#if defined(Q_OS_WIN)
    // On Windows, use libzip via ZipExtractThread
    ZipExtractThread *extractThread = new ZipExtractThread(filePath, extractPath);
    connect(extractThread, &ZipExtractThread::log, this, &MainWindow::log);
    connect(extractThread, &ZipExtractThread::progress, this, &MainWindow::update_progress_bar);
    connect(extractThread, &ZipExtractThread::extractComplete,
            this, [this, filePath, extractPath](QString) {
                // Find java.exe in extracted folder
                QDir dir(extractPath);
                QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                QString javaExe = extractPath;
                if (!entries.isEmpty())
                {
                    javaExe = extractPath + "/" + entries.first() + "/bin/java.exe";
                }
                log("Java extracted to: " + extractPath);
                log("Setting Java path to: " + javaExe);
                settings->setJavaInstallLocation(javaExe);
                javaDownloadComplete();
                QFile::remove(filePath);
            });
    connect(extractThread, &ZipExtractThread::extractFailed,
            this, [this, filePath](QString error) {
                log("Extraction failed: " + error);
                javaDownloadComplete();
                QFile::remove(filePath);
            });
    connect(extractThread, &ZipExtractThread::finished, extractThread, &QObject::deleteLater);
    extractThread->start();
#else
    // On macOS/Linux, use tar
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, filePath, extractPath](int exitCode, QProcess::ExitStatus) {
                if (exitCode == 0)
                {
                    // Find the extracted JRE directory
                    QDir dir(extractPath);
                    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                    QString jrePath = extractPath;
                    if (!entries.isEmpty())
                    {
                        jrePath = extractPath + "/" + entries.first();
#if defined(Q_OS_MACOS)
                        // macOS JRE has Contents/Home structure
                        if (QDir(jrePath + "/Contents/Home").exists())
                        {
                            jrePath = jrePath + "/Contents/Home";
                        }
#endif
                    }
                    QString javaExe = jrePath + "/bin/java";
                    log("Java extracted to: " + jrePath);
                    log("Setting Java path to: " + javaExe);
                    settings->setJavaInstallLocation(javaExe);
                }
                else
                {
                    log("Extraction failed");
                }
                javaDownloadComplete();
                QFile::remove(filePath);
                process->deleteLater();
            });

    process->start("tar", QStringList() << "-xzf" << filePath << "-C" << extractPath);
#endif
}

void MainWindow::javaDownloadComplete()
{
    javaDownloading = false;
    ui->javaButton->setEnabled(true);
    ui->progressBar->hide();
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat("%p%");
}

void MainWindow::javaDownloadFailed(const QString &error)
{
    log("Java download error: " + error);
    javaDownloading = false;
    ui->javaButton->setEnabled(true);
    ui->progressBar->hide();
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat("%p%");
}

void MainWindow::update_progress_bar(qint64 bytesReceived, qint64 bytesTotal)
{
    float percentage = (float)bytesReceived / (float)bytesTotal * 100.0f + 0.5f;
    ui->progressBar->setValue((int)percentage);
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog *settingsDialog = new SettingsDialog(settings, this);
    settingsDialog->open();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (serverProcess != nullptr && QMessageBox::question(this, "XMage Server Running", "XMage server is still running. Would you like to close it? If not, it will need to be closed manually.") == QMessageBox::Yes)
    {
        stopServer();
    }
    delete clientConsole;
    delete serverConsole;
    event->accept();
}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

void MainWindow::on_actionAbout_triggered()
{
    aboutDialog->open();
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::on_actionForum_triggered()
{
    load_browser("https://www.slightlymagic.net/forum/viewforum.php?f=70");
}

void MainWindow::on_actionWebsite_triggered()
{
    load_browser("http://xmage.de/");
}

void MainWindow::on_actionGitHub_XMage_triggered()
{
    load_browser("https://github.com/magefree/mage");
}

void MainWindow::on_actionGitHub_xmage_launcher_qt_triggered()
{
    load_browser("https://github.com/weirddan455/xmage-launcher-qt");
}

void MainWindow::load_browser(const QString &url)
{
    if (QDesktopServices::openUrl(QUrl(url)))
    {
        log("Loading " + url + " in default web browser");
    }
    else
    {
        log("Failed to open web browser for " + url);
    }
}

void MainWindow::server_finished()
{
    serverProcess = nullptr;
    ui->serverButton->setText("Launch Server");
    ui->clientServerButton->setEnabled(true);
}

void MainWindow::launchClient()
{
    QString clientJar;
    if (validateJavaSettings() && findClientJar(&clientJar))
    {
        clientConsole->show();
        XMageProcess *process = new XMageProcess(clientConsole);
        process->setWorkingDirectory(settings->xmageInstallLocation + "/mage-client");
        QStringList arguments = settings->currentClientOptions;
        arguments << "-jar" << clientJar;
        process->start(settings->javaInstallLocation, arguments);
    }
}

void MainWindow::launchServer()
{
    QString serverJar;
    if (serverProcess == nullptr && validateJavaSettings() && findServerJar(&serverJar))
    {
        ui->serverButton->setText("Stop Server");
        ui->clientServerButton->setEnabled(false);
        serverConsole->show();
        serverProcess = new XMageProcess(serverConsole);
        connect(serverProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::server_finished);
        serverProcess->setWorkingDirectory(settings->xmageInstallLocation + "/mage-server");
        QStringList arguments = settings->currentServerOptions;
        arguments << "-jar" << serverJar;
        serverProcess->start(settings->javaInstallLocation, arguments);
    }
}

void MainWindow::stopServer()
{
    /* Qt Documentation: https://doc.qt.io/qt-5/qprocess.html#terminate
     * On Windows, terminate works by sending a WM_CLOSE message which does not work for console applications.
     * On Unix based systems (including Mac OSX), it sends a SIGTERM which should work fine.
     */
#ifdef Q_OS_WINDOWS
    serverProcess->kill();
#else
    serverProcess->terminate();
#endif
}

bool MainWindow::validateJavaSettings()
{
    QFileInfo java(settings->javaInstallLocation);
    if (java.isExecutable())
    {
        return true;
    }
    QMessageBox::warning(this, "Invalid Java Configuration", "Invalid Java configuration. Please set Java location in settings.");
    SettingsDialog *settingsDialog = new SettingsDialog(settings, this);
    settingsDialog->showJavaSettings();
    settingsDialog->open();
    return false;
}

bool MainWindow::findClientJar(QString *jar)
{
    QDir clientDir(settings->xmageInstallLocation + "/mage-client/lib");
    QStringList filter;
    filter << "mage-client*.jar";
    QFileInfoList infoList = clientDir.entryInfoList(filter, QDir::Files);
    if (infoList.isEmpty())
    {
        QMessageBox::warning(this, "Invalid XMage Configuration", "Unable to find XMage client jar file. Please set XMage location in settings.");
        SettingsDialog *settingsDialog = new SettingsDialog(settings, this);
        settingsDialog->showXmageSettings();
        settingsDialog->open();
        return false;
    }
    *jar = infoList.at(0).absoluteFilePath();
    return true;
}

bool MainWindow::findServerJar(QString *jar)
{
    QDir clientDir(settings->xmageInstallLocation + "/mage-server/lib");
    QStringList filter;
    filter << "mage-server*.jar";
    QFileInfoList infoList = clientDir.entryInfoList(filter, QDir::Files);
    if (infoList.isEmpty())
    {
        QMessageBox::warning(this, "Invalid XMage Configuration", "Unable to find XMage server jar file. Please set XMage location in settings.");
        SettingsDialog *settingsDialog = new SettingsDialog(settings, this);
        settingsDialog->showXmageSettings();
        settingsDialog->open();
        return false;
    }
    *jar = infoList.at(0).absoluteFilePath();
    return true;
}
