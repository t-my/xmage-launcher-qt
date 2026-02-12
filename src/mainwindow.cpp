#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "downloadmanager.h"
#include "unzipthread.h"
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
    if (!settings->loadError.isEmpty())
    {
        log("ERROR: " + settings->loadError);
        QMessageBox::critical(this, "Configuration Error", settings->loadError);
    }
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
    if (decksSaveFile != nullptr)
    {
        delete decksSaveFile;
    }
    if (decksNetworkManager != nullptr)
    {
        delete decksNetworkManager;
    }
    delete settings;
    delete background;
    delete ui;
}

void MainWindow::log(QString message)
{
    ui->log->appendPlainText(message);
}

void MainWindow::setButtonsEnabled(bool enabled)
{
    ui->clientButton->setEnabled(enabled);
    ui->serverButton->setEnabled(enabled);
    ui->clientServerButton->setEnabled(enabled);
}

// =============================================================================
// Launch preparation chain: config → java → xmage → decks → launch
// =============================================================================

void MainWindow::prepareLaunch(std::function<void()> onReady)
{
    if (preparing)
    {
        log("Already preparing to launch...");
        return;
    }

    preparing = true;
    pendingLaunch = onReady;
    setButtonsEnabled(false);
    ui->progressBar->show();
    ui->progressBar->setValue(0);

    // Step 1: Fetch config
    prepareStepConfig();
}

void MainWindow::prepareFailed(const QString &error)
{
    log("Launch aborted: " + error);
    preparing = false;
    pendingLaunch = nullptr;
    ui->progressBar->hide();
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat("%p%");
    setButtonsEnabled(true);
}

void MainWindow::prepareStepConfig()
{
    log("Checking for updates...");

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
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    configNetworkManager->get(request);
}

void MainWindow::onConfigFetched(QNetworkReply *reply)
{
    configNetworkManager->disconnect();

    if (reply->error() != QNetworkReply::NoError)
    {
        log("Failed to fetch config: " + reply->errorString());
        reply->deleteLater();

        // If we have a cached config, continue with it
        QJsonObject config;
        if (preparing && loadCachedConfig(&config))
        {
            log("Using cached config instead");
            prepareStepJava();
        }
        else if (preparing)
        {
            prepareFailed("No config available");
        }
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject())
    {
        log("Error: Invalid JSON in config response");
        if (preparing)
        {
            prepareFailed("Invalid config from server");
        }
        return;
    }

    // Save to build folder
    QString buildPath = settings->getCurrentBuildInstallPath();
    QDir().mkpath(buildPath);
    QString configPath = buildPath + "/config.json";

    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(data);
        file.close();
    }

    QJsonObject root = doc.object();
    QString version = root.value("XMage").toObject().value("version").toString();
    if (!version.isEmpty())
    {
        log("Latest version: " + version);
    }

    // Continue preparation chain
    if (preparing)
    {
        prepareStepJava();
    }
}

void MainWindow::prepareStepJava()
{
    QFileInfo javaInfo(settings->javaInstallLocation);
    if (javaInfo.isExecutable())
    {
        // Java already installed, skip to next step
        prepareStepXmage();
        return;
    }

    QJsonObject config;
    if (!loadCachedConfig(&config))
    {
        prepareFailed("No config available for Java download");
        return;
    }

    QJsonObject javaObj = config.value("java").toObject();
    javaVersion = javaObj.value("version").toString();
    javaBaseUrl = javaObj.value("location").toString();

    if (javaBaseUrl.isEmpty())
    {
        prepareFailed("No Java download URL in config");
        return;
    }

    log("Java not found, downloading...");
    javaDownloading = true;
    startJavaDownload();
}

void MainWindow::prepareStepXmage()
{
    // Check if XMage is already installed
    QString buildPath = settings->getCurrentBuildInstallPath();
    bool hasXmage = QDir(buildPath + "/mage-client/lib").exists() ||
                    QDir(buildPath + "/xmage/mage-client/lib").exists();

    if (hasXmage)
    {
        // TODO: check for version updates
        prepareStepDecks();
        return;
    }

    QJsonObject config;
    if (!loadCachedConfig(&config))
    {
        prepareFailed("No config available for XMage download");
        return;
    }

    log("XMage not found, downloading...");
    startXmageDownload(config);
}

void MainWindow::prepareStepDecks()
{
    // Check if decks already exist
    if (QDir(settings->basePath + "/decks").exists())
    {
        // Decks already downloaded, ready to launch
        preparing = false;
        ui->progressBar->hide();
        ui->progressBar->setValue(0);
        ui->progressBar->setFormat("%p%");
        setButtonsEnabled(true);

        if (pendingLaunch)
        {
            auto launch = pendingLaunch;
            pendingLaunch = nullptr;
            launch();
        }
        return;
    }

    log("Downloading metagame decks...");
    decksDownloading = true;

    QString url = "https://github.com/t-my/metagame-decks/releases/latest/download/metagame-decks.zip";
    QString fileName = settings->basePath + "/metagame-decks.zip";

    if (decksNetworkManager == nullptr)
    {
        decksNetworkManager = new QNetworkAccessManager(this);
    }
    else
    {
        decksNetworkManager->disconnect();
    }

    decksSaveFile = new QSaveFile(fileName);
    if (!decksSaveFile->open(QIODevice::WriteOnly))
    {
        log("Failed to create file: " + fileName);
        delete decksSaveFile;
        decksSaveFile = nullptr;
        decksDownloading = false;
        prepareFailed("Could not create decks download file");
        return;
    }

    QUrl downloadUrl(url);
    QNetworkRequest request(downloadUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    connect(decksNetworkManager, &QNetworkAccessManager::finished, this, &MainWindow::onDecksDownloadFinished);
    decksDownloadReply = decksNetworkManager->get(request);
    connect(decksDownloadReply, &QNetworkReply::downloadProgress, this, &MainWindow::onDecksDownloadProgress);
    connect(decksDownloadReply, &QNetworkReply::readyRead, this, &MainWindow::onDecksDownloadReadyRead);
}

// =============================================================================
// Button handlers
// =============================================================================

void MainWindow::on_clientButton_clicked()
{
    if (clientProcess != nullptr)
    {
        stopClient();
    }
    else
    {
        prepareLaunch([this]() { doLaunchClient(); });
    }
}

void MainWindow::on_serverButton_clicked()
{
    if (serverProcess == nullptr)
    {
        prepareLaunch([this]() { doLaunchServer(); });
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

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog *settingsDialog = new SettingsDialog(settings, this);
    connect(settingsDialog, &QDialog::accepted, this, [this]() {
        log("Build changed to: " + settings->currentBuildName);
        updateLaunchReadiness();
    });
    settingsDialog->open();
}

// =============================================================================
// Actual launch methods (called after preparation completes)
// =============================================================================

void MainWindow::doLaunchClient()
{
    QString clientJar;
    if (!findClientJar(&clientJar))
    {
        return;
    }

    QFileInfo jarInfo(clientJar);
    QString clientDir = jarInfo.dir().absolutePath();  // .../mage-client/lib
    clientDir = QFileInfo(clientDir).dir().absolutePath();  // .../mage-client

    log("Launching XMage client...");
    log("  Java: " + settings->javaInstallLocation);
    log("  Build: " + settings->currentBuildName);
    log("  Client dir: " + clientDir);
    ui->clientButton->setText("Stop Client");
    clientConsole->show();
    clientProcess = new XMageProcess(clientConsole);
    connect(clientProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::client_finished);
    clientProcess->setWorkingDirectory(clientDir);
    QStringList arguments = settings->currentClientOptions;
    arguments << "-jar" << clientJar;
    clientProcess->start(settings->javaInstallLocation, arguments);
}

void MainWindow::doLaunchServer()
{
    if (serverProcess != nullptr)
    {
        return;
    }

    QString serverJar;
    if (!findServerJar(&serverJar))
    {
        return;
    }

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

void MainWindow::stopClient()
{
#ifdef Q_OS_WINDOWS
    clientProcess->kill();
#else
    clientProcess->terminate();
#endif
}

void MainWindow::stopServer()
{
#ifdef Q_OS_WINDOWS
    serverProcess->kill();
#else
    serverProcess->terminate();
#endif
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (clientProcess != nullptr)
    {
        stopClient();
    }
    if (serverProcess != nullptr && QMessageBox::question(this, "XMage Server Running", "XMage server is still running. Would you like to close it? If not, it will need to be closed manually.") == QMessageBox::Yes)
    {
        stopServer();
    }
    delete clientConsole;
    delete serverConsole;
    event->accept();
}

void MainWindow::client_finished()
{
    clientProcess = nullptr;
    ui->clientButton->setText("Launch Client");
}

void MainWindow::server_finished()
{
    serverProcess = nullptr;
    ui->serverButton->setText("Launch Server");
}

// =============================================================================
// XMage download
// =============================================================================

void MainWindow::startXmageDownload(const QJsonObject &config)
{
    QJsonObject xmageObj = config.value("XMage").toObject();
    QString downloadUrl = xmageObj.value("full").toString();
    QString version = xmageObj.value("version").toString();

    if (downloadUrl.isEmpty())
    {
        if (preparing)
        {
            prepareFailed("No XMage download URL in config");
        }
        return;
    }

    QString downloadLocation = settings->getCurrentBuildInstallPath();
    QDir().mkpath(downloadLocation);

    log("Downloading XMage " + version + " to: " + downloadLocation);
    DownloadManager *downloadManager = new DownloadManager(downloadLocation, this);
    downloadManager->downloadXmageFromUrl(downloadUrl, version);
}

void MainWindow::download_fail(QString errorMessage)
{
    log(errorMessage);
    ui->progressBar->hide();
    ui->progressBar->setValue(0);

    if (preparing)
    {
        prepareFailed("XMage download failed");
    }
    else
    {
        setButtonsEnabled(true);
    }
}

void MainWindow::download_success(QString installLocation)
{
    log("XMage installed to: " + installLocation);
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat("%p%");

    if (preparing)
    {
        // Continue to next step in preparation chain
        prepareStepDecks();
    }
    else
    {
        ui->progressBar->hide();
        setButtonsEnabled(true);
        updateLaunchReadiness();
    }
}

// =============================================================================
// Java download
// =============================================================================

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
    ZipExtractThread *extractThread = new ZipExtractThread(filePath, extractPath);
    connect(extractThread, &ZipExtractThread::log, this, &MainWindow::log);
    connect(extractThread, &ZipExtractThread::progress, this, &MainWindow::update_progress_bar);
    connect(extractThread, &ZipExtractThread::extractComplete,
            this, [this, filePath, extractPath](QString) {
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
                javaDownloadFailed("Java extraction failed");
                QFile::remove(filePath);
            });
    connect(extractThread, &ZipExtractThread::finished, extractThread, &QObject::deleteLater);
    extractThread->start();
#else
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, filePath, extractPath](int exitCode, QProcess::ExitStatus) {
                if (exitCode == 0)
                {
                    QDir dir(extractPath);
                    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                    QString jrePath = extractPath;
                    if (!entries.isEmpty())
                    {
                        jrePath = extractPath + "/" + entries.first();
#if defined(Q_OS_MACOS)
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
                    javaDownloadComplete();
                }
                else
                {
                    log("Extraction failed");
                    javaDownloadFailed("Java extraction failed");
                }
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

    if (preparing)
    {
        // Continue to next step in preparation chain
        prepareStepXmage();
    }
    else
    {
        ui->progressBar->hide();
        ui->progressBar->setValue(0);
        setButtonsEnabled(true);
    }
}

void MainWindow::javaDownloadFailed(const QString &error)
{
    log("Java download error: " + error);
    javaDownloading = false;

    if (preparing)
    {
        prepareFailed("Java download failed");
    }
    else
    {
        ui->progressBar->hide();
        ui->progressBar->setValue(0);
        ui->progressBar->setFormat("%p%");
        setButtonsEnabled(true);
    }
}

// =============================================================================
// Decks download
// =============================================================================

void MainWindow::onDecksDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0)
    {
        int percent = static_cast<int>((bytesReceived * 100) / bytesTotal);
        ui->progressBar->setValue(percent);
        ui->progressBar->setFormat(QString("Downloading decks... %1 MB / %2 MB")
                                       .arg(bytesReceived / 1048576.0, 0, 'f', 1)
                                       .arg(bytesTotal / 1048576.0, 0, 'f', 1));
    }
}

void MainWindow::onDecksDownloadReadyRead()
{
    if (decksSaveFile && decksDownloadReply)
    {
        decksSaveFile->write(decksDownloadReply->readAll());
    }
}

void MainWindow::onDecksDownloadFinished(QNetworkReply *reply)
{
    decksNetworkManager->disconnect();
    decksDownloadReply = nullptr;

    if (reply->error() != QNetworkReply::NoError)
    {
        if (decksSaveFile)
        {
            decksSaveFile->cancelWriting();
            delete decksSaveFile;
            decksSaveFile = nullptr;
        }
        log("Decks download failed: " + reply->errorString());
        decksDownloading = false;
        reply->deleteLater();

        if (preparing)
        {
            // Decks are optional — continue to launch anyway
            log("Continuing without decks...");
            preparing = false;
            ui->progressBar->hide();
            ui->progressBar->setValue(0);
            ui->progressBar->setFormat("%p%");
            setButtonsEnabled(true);

            if (pendingLaunch)
            {
                auto launch = pendingLaunch;
                pendingLaunch = nullptr;
                launch();
            }
        }
        return;
    }

    if (decksSaveFile)
    {
        decksSaveFile->write(reply->readAll());
        QString fileName = decksSaveFile->fileName();

        if (decksSaveFile->commit())
        {
            log("Download complete. Extracting decks...");
            ui->progressBar->setValue(100);
            ui->progressBar->setFormat("Extracting...");

            // Clean old decks before extracting
            QDir(settings->basePath + "/decks").removeRecursively();

            UnzipThread *unzip = new UnzipThread(fileName, settings->basePath, false);
            connect(unzip, &UnzipThread::log, this, &MainWindow::log);
            connect(unzip, &UnzipThread::progress, this, &MainWindow::update_progress_bar);
            connect(unzip, &UnzipThread::unzip_fail, this, [this, fileName](QString error) {
                log("Decks extraction failed: " + error);
                decksDownloading = false;
                QFile::remove(fileName);

                if (preparing)
                {
                    // Decks are optional — continue to launch anyway
                    log("Continuing without decks...");
                    preparing = false;
                    ui->progressBar->hide();
                    ui->progressBar->setValue(0);
                    ui->progressBar->setFormat("%p%");
                    setButtonsEnabled(true);

                    if (pendingLaunch)
                    {
                        auto launch = pendingLaunch;
                        pendingLaunch = nullptr;
                        launch();
                    }
                }
            });
            connect(unzip, &UnzipThread::unzip_complete, this, [this, fileName](QString location) {
                log("Metagame decks installed to: " + location);
                decksDownloading = false;
                QFile::remove(fileName);

                if (preparing)
                {
                    // All done — launch!
                    preparing = false;
                    ui->progressBar->hide();
                    ui->progressBar->setValue(0);
                    ui->progressBar->setFormat("%p%");
                    setButtonsEnabled(true);

                    if (pendingLaunch)
                    {
                        auto launch = pendingLaunch;
                        pendingLaunch = nullptr;
                        launch();
                    }
                }
            });
            connect(unzip, &UnzipThread::finished, unzip, &QObject::deleteLater);
            unzip->start();
        }
        else
        {
            log("Failed to save decks file");
            decksDownloading = false;

            if (preparing)
            {
                log("Continuing without decks...");
                preparing = false;
                ui->progressBar->hide();
                ui->progressBar->setValue(0);
                ui->progressBar->setFormat("%p%");
                setButtonsEnabled(true);

                if (pendingLaunch)
                {
                    auto launch = pendingLaunch;
                    pendingLaunch = nullptr;
                    launch();
                }
            }
        }
        delete decksSaveFile;
        decksSaveFile = nullptr;
    }

    reply->deleteLater();
}

// =============================================================================
// Utility
// =============================================================================

void MainWindow::update_progress_bar(qint64 bytesReceived, qint64 bytesTotal)
{
    float percentage = (float)bytesReceived / (float)bytesTotal * 100.0f + 0.5f;
    ui->progressBar->setValue((int)percentage);
}

bool MainWindow::findClientJar(QString *jar)
{
    QString buildPath = settings->getCurrentBuildInstallPath();
    QStringList filter;
    filter << "mage-client*.jar";

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
    return false;
}

bool MainWindow::findServerJar(QString *jar)
{
    QString buildPath = settings->getCurrentBuildInstallPath();
    QStringList filter;
    filter << "mage-server*.jar";

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
    return false;
}

void MainWindow::fetchConfig()
{
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
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    configNetworkManager->get(request);
}

void MainWindow::updateLaunchReadiness()
{
    QString buildPath = settings->getCurrentBuildInstallPath();
    bool hasXmage = QDir(buildPath + "/mage-client/lib").exists() ||
                    QDir(buildPath + "/xmage/mage-client/lib").exists();
    QFileInfo javaInfo(settings->javaInstallLocation);
    bool hasJava = javaInfo.isExecutable();

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
