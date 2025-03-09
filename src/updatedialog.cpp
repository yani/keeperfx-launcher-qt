#include "updatedialog.h"
#include "downloader.h"
#include "updater.h"

#include <QFile>
#include <QDir>
#include <QCloseEvent>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>

#include <zlib.h>

#define GAME_FILE_BASE_URL "https://keeperfx.net/game-files"

UpdateDialog::UpdateDialog(QWidget *parent, KfxVersion::VersionInfo versionInfo)
    : QDialog(parent)
    , ui(new Ui::UpdateDialog)
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
    appendLog("Update path: " + QCoreApplication::applicationDirPath());

    // Connect the download progress signal to the slot
    connect(this, &UpdateDialog::fileDownloadProgress, this, &UpdateDialog::onFileDownloadProgress);
}

void UpdateDialog::updateProgress(int value)
{
    ui->progressBar->setValue(value);
}

void UpdateDialog::setProgressMaximum(int value)
{
    ui->progressBar->setMaximum(value);
}

void UpdateDialog::clearProgressBar()
{
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(1);
    ui->progressBar->setFormat("");
}

void UpdateDialog::appendLog(const QString &string)
{
    // Log to debug output
    qDebug() << "Update:" << this->versionInfo.fullString << "->" << string;

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

void UpdateDialog::setUpdateFailed(const QString &reason)
{
    this->ui->updateButton->setDisabled(false);
    this->clearProgressBar();
    this->appendLog(reason);

    QMessageBox::warning(this, "Update failed", reason);
}

void UpdateDialog::on_updateButton_clicked()
{
    this->ui->updateButton->setDisabled(true);

    // Tell user we start the installation
    appendLog("Updating to version " + this->versionInfo.fullString);

    // Show progress bar text
    this->ui->progressBar->setTextVisible(true);

    // Try and get filemap
    appendLog("Trying to get filemap");
    auto fileMap = KfxVersion::getGameFileMap(this->versionInfo.type, this->versionInfo.version);

    // Check if filemap is found
    if (fileMap.has_value() && !fileMap->isEmpty()) {
        // Show filecount log
        QString fileCount = QString::number(fileMap->count());
        appendLog("Filemap with " + fileCount + " files retrieved");
        // Update using filemap
        updateUsingFilemap(fileMap.value());
    } else {
        appendLog("No filemap found");
        // Update using download URL
        updateUsingArchive(this->versionInfo.downloadUrl);
    }
}

void UpdateDialog::updateUsingFilemap(QMap<QString, QString> fileMap)
{
    appendLog("Comparing filemap against local files");

    // Set progress bar
    setProgressMaximum(fileMap.count() - 1);
    this->ui->progressBar->setFormat(QString("Comparing: %p%"));

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
        if (file.exists() == false) {
            updateList.append(filePath);
            continue;
        }

        // Make sure local file is readable
        // It needs to be for the checksum to work
        if (!file.open(QIODevice::ReadOnly)) {
            this->setUpdateFailed("Failed to open file: " + localFilePath);
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
        updateProgress(++progressCount);
    }

    // Clear progress bar
    clearProgressBar();

    // If no files are found
    if (updateList.count() == 0) {
        appendLog("No changes in the files have been detected");
        appendLog("Switching to archive download");
        // Switch to archive download
        updateUsingArchive(this->versionInfo.downloadUrl);
        return;
    }

    // Log filecount
    appendLog(QString::number(updateList.count()) + " files need to be updated");

    // Get type as string
    QString typeString;
    if (this->versionInfo.type == KfxVersion::ReleaseType::STABLE) {
        typeString = "stable";
    } else if (this->versionInfo.type == KfxVersion::ReleaseType::ALPHA) {
        typeString = "alpha";
    } else {
        return;
    }

    // Get download base URL
    QString baseUrl = QString(GAME_FILE_BASE_URL) + "/" + typeString + "/"
                      + this->versionInfo.version;

    // Start downloading files
    downloadFiles(baseUrl);
}

void UpdateDialog::updateUsingArchive(QString downloadUrlString)
{

    appendLog("Downloading archive URL: " + downloadUrlString);

    QUrl downloadUrl(downloadUrlString);

    // Get output file
    QString outputFilePath = QCoreApplication::applicationDirPath() + "/"
                             + downloadUrl.fileName() + ".tmp";
    QFile *outputFile = new QFile(outputFilePath);

    // Download
    Downloader *downloader = new Downloader(this);
    downloader->download(
        downloadUrl,
        outputFile,

        // Progress update
        [this](qint64 bytesReceived, qint64 bytesTotal) {
            if (bytesTotal > 0) {
                this->ui->progressBar->setMaximum(static_cast<int>(bytesTotal / 1024 / 1024));
                this->ui->progressBar->setValue(static_cast<int>(bytesReceived / 1024 / 1024));
                this->ui->progressBar->setFormat(QString("Downloading: %p% (%vMiB)"));
            }
        },

        // Completion
        [this, outputFile](bool success) {
            if (!success) {
                this->setUpdateFailed("Download failed");
                return;
            }

            this->appendLog("Archive successfully downloaded");
            this->clearProgressBar();

            // Test the archive and get the output size
            this->ui->progressBar->setFormat("Testing archive...");
            this->appendLog("Testing archive...");
            uint64_t archiveSize = Updater::testArchiveAndGetSize(outputFile);
            if (archiveSize < 0) {
                this->setUpdateFailed("Archive test failed. It may be corrupted.");
                return;
            } else {
                double archiveSizeInMiB = static_cast<double>(archiveSize) / (1024 * 1024);
                QString archiveSizeString = QString::number(archiveSizeInMiB,
                                                            'f',
                                                            2); // Format to 2 decimal places
                this->appendLog("Total size: " + archiveSizeString + "MiB");
            }

            // Set progressbar vars
            this->ui->progressBar->setMaximum(static_cast<int>(archiveSize));
            this->ui->progressBar->setFormat(QString("Extracting: %p%"));

            // Extract the archive
            this->appendLog("Extracting...");
            bool updateResult = Updater::updateFromArchive(outputFile,
                                                           [this](uint64_t processed_size) {
                                                               // Update progress bar
                                                               this->ui->progressBar->setValue(
                                                                   static_cast<int>(processed_size));
                                                               return true; // Continue extraction
                                                           });

            // Clean up
            outputFile->remove();

            // Clear the progress bar
            this->clearProgressBar();

            // Check if extraction was a success
            if (updateResult == false) {
                this->setUpdateFailed("Archive extraction failed");
                return;
            } else {
                this->appendLog("Archive extraction completed");
            }

            // Success
            this->appendLog("Done!");
            QMessageBox::information(this,
                                     "KeeperFX",
                                     "KeeperFX has been successfully updated to version " + this->versionInfo.version + "!");
            this->accept();
            return;
        });
}

void UpdateDialog::on_cancelButton_clicked() {
    // Call the close() method to trigger the close event
    close();
}

void UpdateDialog::closeEvent(QCloseEvent *event)
{
    // Ask if user is sure
    int result = QMessageBox::question(this,
                                       "Confirmation",
                                       "Are you sure you want to cancel the update?");
    if (result == QMessageBox::Yes) {
        // Allow the dialog to close
        event->accept();

    } else {
        // Ignore the close event
        event->ignore();
    }
}


UpdateDialog::~UpdateDialog()
{
    delete ui;
}

void UpdateDialog::downloadFiles(const QString &baseUrl)
{
    // Create temp dir
    this->tempDir = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                         + "/kfx-update-" + this->versionInfo.version);
    appendLog("Temp directory path: " + tempDir.absolutePath());

    // Make temp directory
    if (!this->tempDir.exists()) {
        this->tempDir.mkpath(".");
    }

    // Make sure temp directory exists now
    if (!this->tempDir.exists()) {
        this->setUpdateFailed("Failed to create temp directory");
        return;
    }

    // Set Variables
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    this->totalFiles = updateList.size();
    this->downloadedFiles = 0;

    // Set progress bar to track total files
    setProgressMaximum(this->totalFiles);
    this->ui->progressBar->setFormat(QString("Downloading: %p%"));

    for (const QString &filePath : updateList) {
        QUrl url(baseUrl + filePath);
        QNetworkRequest request(url);

        QNetworkReply *reply = networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, filePath]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray fileData = reply->readAll();

                QString tempFilePath = this->tempDir.absolutePath() + filePath;
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
                        appendLog("Downloaded: " + filePath);
                    } else {
                        appendLog("Failed to write: " + tempFilePath);
                    }
                } else {
                    appendLog("Failed to write file: " + tempFilePath);
                }
            } else {
                appendLog("Failed to download: " + filePath + " - " + reply->errorString());
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
    updateProgress(downloadedFiles);

    // If all files are downloaded
    if (downloadedFiles == totalFiles) {
        // Show user
        clearProgressBar();
        qDebug() << "All files for update are downloaded";
        appendLog("All files downloaded successfully");

        // Start copying files to KeeperFX dir
        appendLog("Moving files to KeeperFX directory");

        // Make sure temp dir exists
        if (tempDir.exists() == false) {
            this->setUpdateFailed("The temporary directory does not exist");
            return;
        }

        // Define rename rules
        QMap<QString, QString> renameRules = {
            {"/keeperfx.cfg", "/_keeperfx.cfg"},
            {"/keeperfx-launcher-qt.ex", "/keeperfx-launcher-qt-new.exe"},
        };

        // Set progress bar
        this->ui->progressBar->setFormat(QString("Copying: %p%"));
        setProgressMaximum(totalFiles);

        // Vars
        QDir appDir(QCoreApplication::applicationDirPath());
        int copiedFiles = 0;

        // Move and rename files
        for (const QString &filePath : updateList) {
            QString srcFilePath = tempDir.absolutePath() + filePath;

            // Make sure source file exists
            QFile srcFile(srcFilePath);
            if (srcFile.exists() == false) {
                qDebug() << "Source file from tmp dir does not exist:" << srcFilePath;
                appendLog("File does not exist: " + srcFilePath);
                return;
            }

            // Use the new name if exists, otherwise use the same name
            QString destFileName = renameRules.value(filePath, filePath);
            if (destFileName != filePath) {
                qDebug() << "Renaming file during copy:" << filePath << "->" << destFileName;
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
                appendLog("File moved: " + filePath);
            } else {
                qDebug() << "Failed to move" << srcFilePath << "to" << destFilePath;
                appendLog("Failed to move file: " + filePath);
                return;
            }

            updateProgress(++copiedFiles);
        }

        // If all files have been updated
        if (copiedFiles == updateList.count()) {
            this->appendLog("Done!");
            this->clearProgressBar();
            QMessageBox::information(this,
                                     "KeeperFX",
                                     "KeeperFX has been successfully updated to version "
                                         + this->versionInfo.version + "!");
            this->accept();
            return;
        } else {
            this->setUpdateFailed("Failed to move all downloaded files");
        }

    }
}
