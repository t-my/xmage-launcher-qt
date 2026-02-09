#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(Settings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    // Populate build list with names and URLs
    for (const Build &build : settings->builds)
    {
        QListWidgetItem *item = new QListWidgetItem(build.name + "  -  " + build.url);
        item->setData(Qt::UserRole, build.name);
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

void SettingsDialog::accept()
{
    QListWidgetItem *selectedItem = ui->configUrlList->currentItem();
    if (selectedItem)
    {
        settings->setCurrentBuild(selectedItem->data(Qt::UserRole).toString());
    }
    QDialog::accept();
}
