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
    // When working in Qt's UI editor you can change it to another tab by accident
    ui->tabWidget->setCurrentIndex(0);

    // Set versions in about label
    ui->labelAbout->setText(ui->labelAbout->text()
                                .replace("<kfx_version>", KfxVersion::currentVersion.string)
                                .replace("<launcher_version>", LAUNCHER_VERSION));
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
