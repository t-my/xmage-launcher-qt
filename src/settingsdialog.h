#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include "settings.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(Settings *settings, QWidget *parent = nullptr);
    ~SettingsDialog();

public slots:
    void accept() override;

private slots:
    void on_resetButton_clicked();
    void on_addUrlButton_clicked();
    void on_removeUrlButton_clicked();

private:
    Ui::SettingsDialog *ui;
    Settings *settings;
};

#endif // SETTINGSDIALOG_H
