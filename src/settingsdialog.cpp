#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(Settings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->clientOptions->setText(settings->currentClientOptions.join(' '));
    ui->serverOptions->setText(settings->currentServerOptions.join(' '));

    // Populate config URL list
    for (const QString &url : settings->configUrls)
    {
        ui->configUrlList->addItem(url);
    }
    // Select the current config URL
    QList<QListWidgetItem*> items = ui->configUrlList->findItems(settings->configUrl, Qt::MatchExactly);
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
    QString url = QInputDialog::getText(this, "Add Config URL",
                                        "Enter config URL:", QLineEdit::Normal,
                                        "http://", &ok);
    if (ok && !url.isEmpty())
    {
        // Check if URL already exists
        if (ui->configUrlList->findItems(url, Qt::MatchExactly).isEmpty())
        {
            settings->addConfigUrl(url);
            ui->configUrlList->addItem(url);
            // Select the newly added item
            ui->configUrlList->setCurrentRow(ui->configUrlList->count() - 1);
        }
        else
        {
            QMessageBox::information(this, "URL Exists", "This URL is already in the list.");
        }
    }
}

void SettingsDialog::on_removeUrlButton_clicked()
{
    QListWidgetItem *item = ui->configUrlList->currentItem();
    if (item)
    {
        QString url = item->text();
        if (url == "http://xmage.today/config.json")
        {
            QMessageBox::warning(this, "Cannot Remove", "The default URL cannot be removed.");
            return;
        }
        settings->removeConfigUrl(url);
        delete ui->configUrlList->takeItem(ui->configUrlList->row(item));
    }
}

void SettingsDialog::accept()
{
    // Get selected config URL
    QListWidgetItem *selectedItem = ui->configUrlList->currentItem();
    if (selectedItem)
    {
        settings->setConfigUrl(selectedItem->text());
    }

    settings->setClientOptions(ui->clientOptions->text());
    settings->setServerOptions(ui->serverOptions->text());
    QDialog::accept();
}
