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
    background->setGeometry(0, 0, this->size().width(), this->size().height());
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

    // Log startup info and check launch readiness
    updateLaunchReadiness();
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
    if (configNetworkManager != nullptr)
    {
        delete configNetworkManager;
    }
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

void MainWindow::download_success(QString installLocation)
{
    log("XMage installed to: " + installLocation);
    ui->progressBar->hide();
    ui->progressBar->setValue(0);
    ui->downloadButton->setEnabled(true);
    ui->updateButton->setEnabled(true);
    updateLaunchReadiness();
}

void MainWindow::on_updateButton_clicked()
{
    fetchConfig();
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
    on_actionSettings_triggered();
}

void MainWindow::on_downloadButton_clicked()
{
    if (javaDownloading)
    {
        log("Download already in progress...");
        return;
    }

    QJsonObject config;
    if (!loadCachedConfig(&config))
    {
        QMessageBox::warning(this, "No Config", "No cached config. Click 'Check Updates' first.");
        return;
    }

    ui->downloadButton->setEnabled(false);
    ui->updateButton->setEnabled(false);
    ui->progressBar->show();

    // Check if Java is installed
    QFileInfo javaInfo(settings->javaInstallLocation);
    if (!javaInfo.isExecutable())
    {
        // Need to download Java first
        QJsonObject javaObj = config.value("java").toObject();
        javaVersion = javaObj.value("version").toString();
        javaBaseUrl = javaObj.value("location").toString();

        if (javaBaseUrl.isEmpty())
        {
            log("Error: No Java download URL in config");
            ui->downloadButton->setEnabled(true);
            ui->updateButton->setEnabled(true);
            ui->progressBar->hide();
            return;
        }

        log("Java not found, downloading Java first...");
        javaDownloading = true;
        downloadXmageAfterJava = true;  // Continue with XMage after Java completes
        startJavaDownload();
        return;
    }

    // Java is installed, proceed with XMage download
    startXmageDownload(config);
}

void MainWindow::startXmageDownload(const QJsonObject &config)
{
    QJsonObject xmageObj = config.value("XMage").toObject();
    QString downloadUrl = xmageObj.value("full").toString();
    QString version = xmageObj.value("version").toString();

    if (downloadUrl.isEmpty())
    {
        log("Error: No XMage download URL in config");
        ui->downloadButton->setEnabled(true);
        ui->updateButton->setEnabled(true);
        ui->progressBar->hide();
        return;
    }

    QString downloadLocation = settings->getCurrentBuildInstallPath();
    QDir().mkpath(downloadLocation);

    log("Downloading XMage " + version + " to: " + downloadLocation);
    DownloadManager *downloadManager = new DownloadManager(downloadLocation, this);
    downloadManager->downloadXmageFromUrl(downloadUrl, version);
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
    QString fileName = settings->basePath + "/java-" + javaVersion + "-" + platform;

    log("Downloading Java " + javaVersion + " from " + fullUrl);

    if (javaNetworkManager == nullptr)
    {
        javaNetworkManager = new QNetworkAccessManager(this);
    }
    else
    {
        javaNetworkManager->disconnect();
    }

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
    QString extractPath = settings->basePath + "/java";
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
    ui->progressBar->setFormat("%p%");

    // If we were downloading Java as part of XMage download, continue with XMage
    if (downloadXmageAfterJava)
    {
        downloadXmageAfterJava = false;
        QJsonObject config;
        if (loadCachedConfig(&config))
        {
            log("Java installed, now downloading XMage...");
            startXmageDownload(config);
            return;
        }
        else
        {
            log("Error: Could not load cached config for XMage download");
        }
    }

    ui->progressBar->hide();
    ui->progressBar->setValue(0);
    ui->downloadButton->setEnabled(true);
    ui->updateButton->setEnabled(true);
}

void MainWindow::javaDownloadFailed(const QString &error)
{
    log("Java download error: " + error);
    javaDownloading = false;
    downloadXmageAfterJava = false;
    ui->downloadButton->setEnabled(true);
    ui->updateButton->setEnabled(true);
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
    connect(settingsDialog, &QDialog::accepted, this, [this]() {
        log("Build changed to: " + settings->currentBuildName);
        updateLaunchReadiness();
    });
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

void MainWindow::server_finished()
{
    serverProcess = nullptr;
    ui->serverButton->setText("Launch Server");
}

void MainWindow::launchClient()
{
    QString clientJar;
    if (validateJavaSettings() && findClientJar(&clientJar))
    {
        // Derive working directory from jar path (jar is in mage-client/lib/)
        QFileInfo jarInfo(clientJar);
        QString clientDir = jarInfo.dir().absolutePath();  // .../mage-client/lib
        clientDir = QFileInfo(clientDir).dir().absolutePath();  // .../mage-client

        log("Launching XMage client...");
        log("  Java: " + settings->javaInstallLocation);
        log("  Build: " + settings->currentBuildName);
        log("  Client dir: " + clientDir);
        clientConsole->show();
        XMageProcess *process = new XMageProcess(clientConsole);
        process->setWorkingDirectory(clientDir);
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
        // Derive working directory from jar path (jar is in mage-server/lib/)
        QFileInfo jarInfo(serverJar);
        QString serverDir = jarInfo.dir().absolutePath();  // .../mage-server/lib
        serverDir = QFileInfo(serverDir).dir().absolutePath();  // .../mage-server

        log("Launching XMage server...");
        log("  Java: " + settings->javaInstallLocation);
        log("  Build: " + settings->currentBuildName);
        log("  Server dir: " + serverDir);
        ui->serverButton->setText("Stop Server");
        serverConsole->show();
        serverProcess = new XMageProcess(serverConsole);
        connect(serverProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::server_finished);
        serverProcess->setWorkingDirectory(serverDir);
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
    QMessageBox::warning(this, "Invalid Java Configuration", "Invalid Java configuration. Please download Java first.");
    return false;
}

bool MainWindow::findClientJar(QString *jar)
{
    QString buildPath = settings->getCurrentBuildInstallPath();
    QStringList filter;
    filter << "mage-client*.jar";

    // Try direct path first, then xmage subfolder
    QStringList searchPaths;
    searchPaths << buildPath + "/mage-client/lib";
    searchPaths << buildPath + "/xmage/mage-client/lib";

    for (const QString &clientLibPath : searchPaths)
    {
        QDir clientDir(clientLibPath);
        QFileInfoList infoList = clientDir.entryInfoList(filter, QDir::Files);
        if (!infoList.isEmpty())
        {
            *jar = infoList.at(0).absoluteFilePath();
            return true;
        }
    }

    log("ERROR: No client jar found in: " + searchPaths.join(" or "));
    QMessageBox::warning(this, "Invalid XMage Configuration", "Unable to find XMage client jar file.\n\nPlease download XMage first.");
    return false;
}

bool MainWindow::findServerJar(QString *jar)
{
    QString buildPath = settings->getCurrentBuildInstallPath();
    QStringList filter;
    filter << "mage-server*.jar";

    // Try direct path first, then xmage subfolder
    QStringList searchPaths;
    searchPaths << buildPath + "/mage-server/lib";
    searchPaths << buildPath + "/xmage/mage-server/lib";

    for (const QString &serverLibPath : searchPaths)
    {
        QDir serverDir(serverLibPath);
        QFileInfoList infoList = serverDir.entryInfoList(filter, QDir::Files);
        if (!infoList.isEmpty())
        {
            *jar = infoList.at(0).absoluteFilePath();
            return true;
        }
    }

    log("ERROR: No server jar found in: " + searchPaths.join(" or "));
    QMessageBox::warning(this, "Invalid XMage Configuration", "Unable to find XMage server jar file.\n\nPlease download XMage first.");
    return false;
}

void MainWindow::fetchConfig()
{
    if (configFetching)
    {
        log("Config fetch already in progress...");
        return;
    }

    configFetching = true;
    ui->updateButton->setEnabled(false);
    log("Fetching config for build: " + settings->currentBuildName);
    log("  URL: " + settings->getCurrentBuildUrl());

    if (configNetworkManager == nullptr)
    {
        configNetworkManager = new QNetworkAccessManager(this);
    }
    else
    {
        configNetworkManager->disconnect();
    }

    connect(configNetworkManager, &QNetworkAccessManager::finished, this, &MainWindow::onConfigFetched);
    QNetworkRequest request(QUrl(settings->getCurrentBuildUrl()));
    configNetworkManager->get(request);
}

void MainWindow::onConfigFetched(QNetworkReply *reply)
{
    configNetworkManager->disconnect();
    configFetching = false;
    ui->updateButton->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError)
    {
        log("Failed to fetch config: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    // Validate JSON before saving
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject())
    {
        log("Error: Invalid JSON in config response");
        return;
    }

    // Save to build folder
    QString buildPath = settings->getCurrentBuildInstallPath();
    QDir().mkpath(buildPath);
    QString configPath = buildPath + "/config.json";

    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        log("Error: Could not write config to " + configPath);
        return;
    }
    file.write(data);
    file.close();

    // Log version info from config
    QJsonObject root = doc.object();
    QJsonObject xmageObj = root.value("XMage").toObject();
    QString version = xmageObj.value("version").toString();
    if (!version.isEmpty())
    {
        log("Latest XMage version: " + version);
    }

    log("Config saved for build: " + settings->currentBuildName);
    updateLaunchReadiness();
}

void MainWindow::updateLaunchReadiness()
{
    QString buildPath = settings->getCurrentBuildInstallPath();
    bool hasXmage = QDir(buildPath + "/mage-client/lib").exists() ||
                    QDir(buildPath + "/xmage/mage-client/lib").exists();
    QFileInfo javaInfo(settings->javaInstallLocation);
    bool hasJava = javaInfo.isExecutable();

    // Enable/disable launch buttons based on readiness
    bool canLaunch = hasXmage && hasJava;

    if (canLaunch) 
    {
        ui->clientButton->setEnabled(canLaunch);
        ui->serverButton->setEnabled(canLaunch);
    } else 
    {
        ui->clientButton->setEnabled(false);
        ui->serverButton->setEnabled(false);
    }
    
    // Get version from cached config
    QString version;
    QJsonObject config;
    if (loadCachedConfig(&config))
    {
        version = config.value("XMage").toObject().value("version").toString();
    }

    log("Build: " + settings->currentBuildName);
    log("  Base path: " + settings->basePath);
    log("  Config: " + (version.isEmpty() ? "not fetched" : version));
    log("  XMage: " + QString(hasXmage ? "installed" : "not installed"));
    log("  Java: " + QString(hasJava ? "ready" : "not installed"));
}

bool MainWindow::loadCachedConfig(QJsonObject *config)
{
    QString configPath = settings->getCurrentBuildInstallPath() + "/config.json";
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (doc.isNull() || !doc.isObject())
    {
        return false;
    }
    *config = doc.object();
    return true;
}
