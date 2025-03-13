#include "updatedialog.h"
#include "archiver.h"
#include "downloader.h"
#include "settings.h"
#include "updater.h"

#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QThread>

#include <zlib.h>

#define GAME_FILE_BASE_URL "https://keeperfx.net/game-files"

UpdateDialog::UpdateDialog(QWidget *parent, KfxVersion::VersionInfo versionInfo)
    : QDialog(parent)
    , ui(new Ui::UpdateDialog)
    , networkManager(new QNetworkAccessManager(this))
{
    // Setup this UI
    ui->setupUi(this);

    // Store version info in this dialog
    this->versionInfo = versionInfo;

    // Get current version string
    QString currentVersionString = KfxVersion::currentVersion.version;
    if (KfxVersion::currentVersion.type == KfxVersion::ReleaseType::ALPHA) {
        currentVersionString += " Alpha";
    }

    // Get new version string
    QString newVersionString = this->versionInfo.version;
    if (this->versionInfo.type == KfxVersion::ReleaseType::ALPHA) {
        newVersionString += " Alpha";
    }

    // Update version text in UI
    QString infoLabelText = ui->infoLabel->text();
    infoLabelText.replace("<CURRENT_VERSION>", currentVersionString);
    infoLabelText.replace("<NEW_VERSION>", newVersionString);
    ui->infoLabel->setText(infoLabelText);

    // Log the update path
    emit appendLog("Update path: " + QCoreApplication::applicationDirPath());

    // Connect signals to slots
    connect(this, &UpdateDialog::fileDownloadProgress, this, &UpdateDialog::onFileDownloadProgress);

    connect(this, &UpdateDialog::appendLog, this, &UpdateDialog::onAppendLog);
    connect(this, &UpdateDialog::clearProgressBar, this, &UpdateDialog::onClearProgressBar);
    connect(this, &UpdateDialog::setUpdateFailed, this, &UpdateDialog::onUpdateFailed);

    connect(this, &UpdateDialog::updateProgress, ui->progressBar, &QProgressBar::setValue);
    connect(this, &UpdateDialog::setProgressMaximum, ui->progressBar, &QProgressBar::setMaximum);
    connect(this, &UpdateDialog::setProgressBarFormat, ui->progressBar, &QProgressBar::setFormat);
}

UpdateDialog::~UpdateDialog()
{
    delete ui;
}

void UpdateDialog::onAppendLog(const QString &string)
{
    // Log to debug output
    qDebug() << "Install:" << string;

    // Add string to log with timestamp
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString timestampString = currentDateTime.toString("HH:mm:ss");
    ui->logTextArea->insertPlainText("[" + timestampString + "] " + string + "\n");

    // Scroll to the left on new text
    QScrollBar *hScrollBar = ui->logTextArea->horizontalScrollBar();
    if (hScrollBar) {
        hScrollBar->setValue(hScrollBar->minimum());
    }

    // Scroll to the bottom on new text
    QScrollBar *vScrollBar = ui->logTextArea->verticalScrollBar();
    if (vScrollBar) {
        vScrollBar->setValue(vScrollBar->maximum());
    }

    // Force redraw
    QApplication::processEvents();
}

void UpdateDialog::updateProgressBar(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        ui->progressBar->setMaximum(static_cast<int>(bytesTotal / 1024 / 1024));
        ui->progressBar->setValue(static_cast<int>(bytesReceived / 1024 / 1024));
        ui->progressBar->setFormat(QString("Downloading: %p% (%vMiB)"));
    }
}

void UpdateDialog::onClearProgressBar()
{
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(1);
    ui->progressBar->setFormat("");
}

void UpdateDialog::onUpdateFailed(const QString &reason)
{
    ui->updateButton->setDisabled(false);
    onClearProgressBar();
    onAppendLog(reason);
    QMessageBox::warning(this, "Update failed", reason);
}

void UpdateDialog::on_updateButton_clicked()
{
    // Update GUI
    ui->updateButton->setDisabled(true);
    ui->progressBar->setTextVisible(true);

    // Tell user we start the installation
    emit appendLog("Updating to version " + versionInfo.fullString);

    // Try and get filemap
    emit appendLog("Trying to get filemap");
    auto fileMap = KfxVersion::getGameFileMap(versionInfo.type, versionInfo.version);

    // Check if filemap is found
    if (fileMap.has_value() && !fileMap->isEmpty()) {
        // Show filecount log
        QString fileCount = QString::number(fileMap->count());
        emit appendLog("Filemap with " + fileCount + " files retrieved");
        // Update using filemap
        updateUsingFilemap(fileMap.value());
    } else {
        emit appendLog("No filemap found");
        // Update using download URL
        updateUsingArchive(versionInfo.downloadUrl);
    }
}

void UpdateDialog::updateUsingFilemap(QMap<QString, QString> fileMap)
{
    emit appendLog("Comparing filemap against local files");

    // Set progress bar
    emit setProgressMaximum(fileMap.count() - 1);
    emit setProgressBarFormat(QString("Comparing: %p%"));

    // Create fresh file list
    updateList = QStringList();

    // Loop through the filemap
    int progressCount = 0;
    for (auto it = fileMap.begin(); it != fileMap.end(); ++it) {
        QString filePath = it.key();
        QString expectedChecksum = it.value();

        // Construct the full path to the local file
        QString localFilePath = QCoreApplication::applicationDirPath() + filePath;
        QFile file(localFilePath);

        // Check if local file exists
        if (!file.exists()) {
            updateList.append(filePath);
            continue;
        }

        // Make sure local file is readable
        // It needs to be for the checksum to work
        if (!file.open(QIODevice::ReadOnly)) {
            emit setUpdateFailed("Failed to open file: " + localFilePath);
            return;
        }

        // Calculate CRC32 checksum
        QByteArray fileData = file.readAll();
        uLong crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, reinterpret_cast<const Bytef *>(fileData.data()), fileData.size());
        QString localChecksum = QString::number(crc, 16);

        // Compare checksums
        if (localChecksum != expectedChecksum) {
            updateList.append(filePath);
        }

        // Update progress
        emit updateProgress(++progressCount);
    }

    // Clear progress bar
    emit clearProgressBar();

    // If no files are found
    if (updateList.count() == 0) {
        emit appendLog("No changes in the files have been detected");
        emit appendLog("Switching to archive download");
        // Switch to archive download
        updateUsingArchive(versionInfo.downloadUrl);
        return;
    }

    // Log filecount
    emit appendLog(QString::number(updateList.count()) + " files need to be updated");

    // Get type as string
    QString typeString;
    if (versionInfo.type == KfxVersion::ReleaseType::STABLE) {
        typeString = "stable";
    } else if (versionInfo.type == KfxVersion::ReleaseType::ALPHA) {
        typeString = "alpha";
    } else {
        return;
    }

    // Get download base URL
    QString baseUrl = QString(GAME_FILE_BASE_URL) + "/" + typeString + "/"
                      + versionInfo.version;

    // Start downloading files
    downloadFiles(baseUrl);
}

void UpdateDialog::updateUsingArchive(QString downloadUrlString)
{
    emit appendLog("Downloading archive URL: " + downloadUrlString);

    QUrl downloadUrl(downloadUrlString);

    // Get output file
    QString outputFilePath = QCoreApplication::applicationDirPath() + "/" + downloadUrl.fileName() + ".tmp";
    QFile *outputFile = new QFile(outputFilePath);

    Downloader *downloader = new Downloader(this);
    connect(downloader, &Downloader::downloadProgress, this, &UpdateDialog::updateProgressBar);
    connect(downloader, &Downloader::downloadCompleted, this, &UpdateDialog::onArchiveDownloadFinished);

    downloader->download(downloadUrl, outputFile);
}

void UpdateDialog::onArchiveDownloadFinished(bool success)
{
    if (!success) {
        emit setUpdateFailed("Archive download failed.");
        return;
    }

    emit appendLog("Archive successfully downloaded");
    emit clearProgressBar();

    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/" + QUrl(versionInfo.downloadUrl).fileName() + ".tmp");

    // Test archive
    emit appendLog("Testing stable release archive...");
    QThread::create([this, outputFile]() {
        uint64_t archiveSize = Archiver::testArchiveAndGetSize(outputFile);
        onArchiveTestComplete(archiveSize);
    })->start();

}

void UpdateDialog::onArchiveTestComplete(uint64_t archiveSize){

    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/" + QUrl(versionInfo.downloadUrl).fileName() + ".tmp");

    // Show total size
    double archiveSizeInMiB = static_cast<double>(archiveSize) / (1024 * 1024);
    QString archiveSizeString = QString::number(archiveSizeInMiB, 'f', 2); // Format to 2 decimal places
    emit appendLog("Total size: " + archiveSizeString + "MiB");

    // Start extraction process
    emit setProgressMaximum(static_cast<int>(archiveSize));
    emit setProgressBarFormat("Extracting: %p%");
    emit appendLog("Extracting...");

    Updater *updater = new Updater(this);
    connect(updater, &Updater::progress, this, &UpdateDialog::updateProgress);
    connect(updater, &Updater::updateComplete, this, &UpdateDialog::onUpdateComplete);
    connect(updater, &Updater::updateFailed, this, &UpdateDialog::setUpdateFailed);

    updater->updateFromArchive(outputFile);
}

void UpdateDialog::onUpdateComplete()
{
    emit appendLog("Extraction completed");
    emit clearProgressBar();

    // Remove temp archive
    emit appendLog("Removing temporary archive");
    QFile *archiveFile = new QFile(QCoreApplication::applicationDirPath() + "/" + QUrl(versionInfo.downloadUrl).fileName() + ".tmp");
    if (archiveFile->exists()) {
        archiveFile->remove();
    }

    emit appendLog("Done!");
    QMessageBox::information(this, "KeeperFX", "KeeperFX has been successfully updated to version " + versionInfo.version + "!");
    accept();
}

void UpdateDialog::on_cancelButton_clicked()
{
    close();
}

void UpdateDialog::closeEvent(QCloseEvent *event)
{
    // Ask if user is sure
    int result = QMessageBox::question(this,
                                       "Confirmation",
                                       "Are you sure you want to cancel the update?");
    if (result == QMessageBox::Yes) {
        event->accept();
    } else {
        event->ignore();
    }
}

void UpdateDialog::downloadFiles(const QString &baseUrl)
{
    // Create temp dir
    tempDir = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                   + "/kfx-update-" + versionInfo.version);
    emit appendLog("Temp directory path: " + tempDir.absolutePath());

    // Make temp directory
    if (!tempDir.exists()) {
        tempDir.mkpath(".");
    }

    // Make sure temp directory exists now
    if (!tempDir.exists()) {
        emit setUpdateFailed("Failed to create temp directory");
        return;
    }

    // Set Variables
    totalFiles = updateList.size();
    downloadedFiles = 0;

    // Set progress bar to track total files
    emit setProgressMaximum(totalFiles);
    emit setProgressBarFormat(QString("Downloading: %p%"));

    for (const QString &filePath : updateList) {
        QUrl url(baseUrl + filePath);
        QNetworkRequest request(url);

        QNetworkReply *reply = networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, filePath]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray fileData = reply->readAll();

                QString tempFilePath = tempDir.absolutePath() + filePath;
                QDir().mkpath(QFileInfo(tempFilePath).path());

                // Open the temporary file
                QFile tempFile(tempFilePath);
                if (tempFile.open(QIODevice::WriteOnly)) {
                    // Write the file
                    qint64 bytesWritten = tempFile.write(fileData);
                    tempFile.flush(); // Ensure data is written
                    tempFile.close();

                    // Verify file is written
                    if (bytesWritten == fileData.size()) {
                        emit appendLog("Downloaded: " + filePath);
                    } else {
                        emit appendLog("Failed to write: " + tempFilePath);
                    }
                } else {
                    emit appendLog("Failed to write file: " + tempFilePath);
                }
            } else {
                emit appendLog("Failed to download: " + filePath + " - " + reply->errorString());
            }
            reply->deleteLater();

            emit fileDownloadProgress();
        });
    }
}

void UpdateDialog::onFileDownloadProgress()
{
    // Increase counter and update progress bar
    downloadedFiles++;
    emit updateProgress(downloadedFiles);

    // If all files are downloaded
    if (downloadedFiles == totalFiles) {
        // Show user
        emit clearProgressBar();
        emit appendLog("All files downloaded successfully");

        // Start copying files to KeeperFX dir
        emit appendLog("Moving files to KeeperFX directory");

        // Make sure temp dir exists
        if (!tempDir.exists()) {
            emit setUpdateFailed("The temporary directory does not exist");
            return;
        }

        // Define rename rules
        QMap<QString, QString> renameRules = {
            {"/keeperfx.cfg", "/_keeperfx.cfg"},
            {"/keeperfx-launcher-qt.exe", "/keeperfx-launcher-qt-new.exe"},
        };

        // Set progress bar
        emit setProgressBarFormat(QString("Copying: %p%"));
        emit setProgressMaximum(totalFiles);

        // Vars
        QDir appDir(QCoreApplication::applicationDirPath());
        int copiedFiles = 0;

        // Move and rename files
        for (const QString &filePath : updateList) {
            QString srcFilePath = tempDir.absolutePath() + filePath;

            // Make sure source file exists
            QFile srcFile(srcFilePath);
            if (!srcFile.exists()) {
                emit appendLog("File does not exist: " + srcFilePath);
                return;
            }

            // Use the new name if exists, otherwise use the same name
            QString destFileName = renameRules.value(filePath, filePath);
            if (destFileName != filePath) {
                emit appendLog("Renaming file during copy: " + filePath + " -> " + destFileName);
            }

            // Get destination path
            QString destFilePath = appDir.absolutePath() + destFileName;

            // Remove destination file if it exists
            QFile destFile(destFilePath);
            if (destFile.exists()) {
                destFile.remove();
            }

            // Move file
            if (QFile::rename(srcFilePath, destFilePath)) {
                emit appendLog("File moved: " + filePath);
            } else {
                emit appendLog("Failed to move file: " + filePath);
                return;
            }

            emit updateProgress(++copiedFiles);
        }

        // If all files have been updated
        if (copiedFiles == updateList.count()) {
            emit clearProgressBar();

            // Copy new settings
            emit appendLog("Copying any new settings");
            Settings::load();

            // Success!
            emit appendLog("Done!");
            QMessageBox::information(this,
                                     "KeeperFX",
                                     "KeeperFX has been successfully updated to version "
                                         + versionInfo.version + "!");
            accept();
            return;

        } else {
            emit setUpdateFailed("Failed to move all downloaded files");
        }
    }
}
