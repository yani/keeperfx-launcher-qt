#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "version.h"
#include "kfxversion.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    // Make sure this widget starts at the first tab
    ui->tabWidget->setCurrentIndex(0);

    // Set version in about
    ui->labelKfxLauncherVersion->setText(ui->labelKfxLauncherVersion->text() + LAUNCHER_VERSION);
    ui->labelKfxVersion->setText(ui->labelKfxVersion->text() + KfxVersion::currentVersion.string);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
