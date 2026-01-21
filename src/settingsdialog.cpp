#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(Settings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->clientOptions->setText(settings->currentClientOptions.join(' '));
    ui->serverOptions->setText(settings->currentServerOptions.join(' '));

    // Populate build list with names
    for (const Build &build : settings->builds)
    {
        ui->configUrlList->addItem(build.name);
    }
    // Select the current build
    QList<QListWidgetItem*> items = ui->configUrlList->findItems(settings->currentBuildName, Qt::MatchExactly);
    if (!items.isEmpty())
    {
        ui->configUrlList->setCurrentItem(items.first());
    }

    this->settings = settings;
    connect(this, &QDialog::finished, this, &QObject::deleteLater);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_resetButton_clicked()
{
    ui->clientOptions->setText(settings->defaultClientOptions.join(' '));
    ui->serverOptions->setText(settings->defaultServerOptions.join(' '));
}

void SettingsDialog::on_addUrlButton_clicked()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Add Build",
                                         "Build name:", QLineEdit::Normal,
                                         "", &ok);
    if (!ok || name.isEmpty())
    {
        return;
    }

    // Check if name already exists
    if (!ui->configUrlList->findItems(name, Qt::MatchExactly).isEmpty())
    {
        QMessageBox::information(this, "Build Exists", "A build with this name already exists.");
        return;
    }

    QString url = QInputDialog::getText(this, "Add Build",
                                        "Config URL:", QLineEdit::Normal,
                                        "http://", &ok);
    if (!ok || url.isEmpty())
    {
        return;
    }

    settings->addBuild(name, url);
    ui->configUrlList->addItem(name);
    // Select the newly added item
    ui->configUrlList->setCurrentRow(ui->configUrlList->count() - 1);
}

void SettingsDialog::on_removeUrlButton_clicked()
{
    QListWidgetItem *item = ui->configUrlList->currentItem();
    if (item)
    {
        QString name = item->text();
        if (settings->isDefaultBuild(name))
        {
            QMessageBox::warning(this, "Cannot Remove", "Default builds cannot be removed.");
            return;
        }
        settings->removeBuild(name);
        delete ui->configUrlList->takeItem(ui->configUrlList->row(item));
    }
}

void SettingsDialog::accept()
{
    // Get selected build
    QListWidgetItem *selectedItem = ui->configUrlList->currentItem();
    if (selectedItem)
    {
        settings->setCurrentBuild(selectedItem->text());
    }

    settings->setClientOptions(ui->clientOptions->text());
    settings->setServerOptions(ui->serverOptions->text());
    QDialog::accept();
}
