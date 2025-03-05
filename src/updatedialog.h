#pragma once

#include <QDialog>
#include <QDateTime>
#include <QScrollBar>

#include "kfxversion.h"
#include "ui_updatedialog.h"

namespace Ui {
class UpdateDialog;
}

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    UpdateDialog(QWidget *parent = nullptr, KfxVersion::VersionInfo versionInfo = KfxVersion::VersionInfo());
    ~UpdateDialog();

private slots:
    void on_updateButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::UpdateDialog *ui;

    void appendLog(const QString &string);
    void updateProgress(int value);
    void setProgressMaximum(int value);
    void clearProgressBar();

    void closeEvent(QCloseEvent *event) override;
};
