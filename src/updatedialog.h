#pragma once

#include <QDialog>
#include <QDateTime>
#include <QScrollBar>
#include <QDir>

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

    void onFileDownloadProgress();

signals:
    void fileDownloadProgress();

private:
    Ui::UpdateDialog *ui;
    KfxVersion::VersionInfo versionInfo;

    void appendLog(const QString &string);
    void updateProgress(int value);
    void setProgressMaximum(int value);
    void clearProgressBar();
    void setUpdateFailed(const QString &reason);

    void closeEvent(QCloseEvent *event) override;

    QStringList updateList;
    void updateUsingFilemap(QMap<QString, QString> fileMap);
    void updateUsingArchive(QString downloadUrl);

    QDir tempDir;
    int totalFiles;
    int downloadedFiles;
    void downloadFiles(const QString &baseUrl);
};
