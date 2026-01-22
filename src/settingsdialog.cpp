#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(Settings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->clientOptions->setText(settings->currentClientOptions.join(' '));
    ui->serverOptions->setText(settings->currentServerOptions.join(' '));

    // Populate build list with names and URLs
    for (const Build &build : settings->builds)
    {
        QListWidgetItem *item = new QListWidgetItem(build.name + "  -  " + build.url);
        item->setData(Qt::UserRole, build.name);  // Store name for lookup
        ui->configUrlList->addItem(item);
        if (build.name == settings->currentBuildName)
        {
            ui->configUrlList->setCurrentItem(item);
        }
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
    // Create a dialog with both name and URL fields
    QDialog dialog(this);
    dialog.setWindowTitle("Add Build");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *nameLabel = new QLabel("Build name:", &dialog);
    QLineEdit *nameEdit = new QLineEdit(&dialog);
    layout->addWidget(nameLabel);
    layout->addWidget(nameEdit);

    QLabel *urlLabel = new QLabel("Config URL:", &dialog);
    QLineEdit *urlEdit = new QLineEdit("http://", &dialog);
    layout->addWidget(urlLabel);
    layout->addWidget(urlEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    QString name = nameEdit->text().trimmed();
    QString url = urlEdit->text().trimmed();

    if (name.isEmpty() || url.isEmpty())
    {
        return;
    }

    // Check if name already exists
    for (int i = 0; i < ui->configUrlList->count(); ++i)
    {
        if (ui->configUrlList->item(i)->data(Qt::UserRole).toString() == name)
        {
            QMessageBox::information(this, "Build Exists", "A build with this name already exists.");
            return;
        }
    }

    settings->addBuild(name, url);
    QListWidgetItem *item = new QListWidgetItem(name + "  -  " + url);
    item->setData(Qt::UserRole, name);
    ui->configUrlList->addItem(item);
    // Select the newly added item
    ui->configUrlList->setCurrentRow(ui->configUrlList->count() - 1);
}

void SettingsDialog::on_removeUrlButton_clicked()
{
    QListWidgetItem *item = ui->configUrlList->currentItem();
    if (item)
    {
        QString name = item->data(Qt::UserRole).toString();
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
        settings->setCurrentBuild(selectedItem->data(Qt::UserRole).toString());
    }

    settings->setClientOptions(ui->clientOptions->text());
    settings->setServerOptions(ui->serverOptions->text());
    QDialog::accept();
}
