#pragma once

#include <QDialog>
#include <QDateTime>
#include <QScrollBar>
#include <QDir>
#include <QNetworkAccessManager>

#include "kfxversion.h"
#include "ui_updatedialog.h"

namespace Ui {
class UpdateDialog;
}

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    UpdateDialog(QWidget *parent = nullptr, KfxVersion::VersionInfo versionInfo = KfxVersion::VersionInfo(), bool autoUpdate = false);
    ~UpdateDialog();

private slots:
    void on_updateButton_clicked();
    void on_cancelButton_clicked();

    void onFileDownloadProgress();
    void onArchiveDownloadFinished(bool success);
    void onArchiveTestComplete(uint64_t archiveSize);
    void onUpdateComplete();

    void onAppendLog(const QString &string);
    void onUpdateFailed(const QString &reason);
    void onClearProgressBar();
    void updateProgressBar(qint64 bytesReceived, qint64 bytesTotal);

signals:
    void fileDownloadProgress();
    void appendLog(const QString &string);
    void setUpdateFailed(const QString &reason);
    void clearProgressBar();
    void updateProgress(int value);
    void setProgressMaximum(int value);
    void setProgressBarFormat(QString format);

private:
    Ui::UpdateDialog *ui;
    KfxVersion::VersionInfo versionInfo;
    QNetworkAccessManager *networkManager;

    void closeEvent(QCloseEvent *event) override;

    QStringList updateList;
    void updateUsingFilemap(QMap<QString, QString> fileMap);
    void updateUsingArchive(QString downloadUrl);

    QDir tempDir;
    int totalFiles;
    int downloadedFiles;
    void downloadFiles(const QString &baseUrl);

    bool autoUpdate;
    QString originalTitleText;

    void backupSaves();
};
