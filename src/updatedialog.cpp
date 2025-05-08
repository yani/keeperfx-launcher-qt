#include "updatedialog.h"
#include "archiver.h"
#include "downloader.h"
#include "launcheroptions.h"
#include "savefile.h"
#include "settings.h"
#include "updater.h"

#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QThread>
#include <QThreadPool>
#include <QTimer>

#include <zlib.h>

#define GAME_FILE_BASE_URL "https://keeperfx.net/game-files"
#define AUTO_UPDATE_MESSAGEBOX_TIMER 2500

UpdateDialog::UpdateDialog(QWidget *parent, KfxVersion::VersionInfo versionInfo, bool autoUpdate)
    : QDialog(parent)
    , ui(new Ui::UpdateDialog)
    , networkManager(new QNetworkAccessManager(this))
{
    // Setup this UI
    ui->setupUi(this);
    this->originalTitleText = ui->titleLabel->text();

    // Disable resizing and remove maximize button
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    // Store version info in this dialog
    this->versionInfo = versionInfo;

    // Get current version string
    QString currentVersionString = KfxVersion::currentVersion.version;
    if (KfxVersion::currentVersion.type == KfxVersion::ReleaseType::ALPHA) {
        currentVersionString += " " + tr("Alpha", "Appended to version string");
    }

    // Get new version string
    QString newVersionString = this->versionInfo.version;
    if (this->versionInfo.type == KfxVersion::ReleaseType::ALPHA) {
        newVersionString += " " + tr("Alpha", "Appended to version string");
    }

    // Update version text in UI
    QString infoLabelText = ui->infoLabel->text();
    ui->infoLabel->setText(infoLabelText.arg(tr("Current version:", "Label")).arg(currentVersionString).arg(tr("New version:", "Label")).arg(newVersionString));

    // Log the update path
    emit appendLog(tr("Update path: %1", "Log Message").arg(QCoreApplication::applicationDirPath()));

    // Connect signals to slots
    connect(this, &UpdateDialog::fileDownloadProgress, this, &UpdateDialog::onFileDownloadProgress);

    connect(this, &UpdateDialog::appendLog, this, &UpdateDialog::onAppendLog);
    connect(this, &UpdateDialog::clearProgressBar, this, &UpdateDialog::onClearProgressBar);
    connect(this, &UpdateDialog::setUpdateFailed, this, &UpdateDialog::onUpdateFailed);

    connect(this, &UpdateDialog::updateProgress, ui->progressBar, &QProgressBar::setValue);
    connect(this, &UpdateDialog::setProgressMaximum, ui->progressBar, &QProgressBar::setMaximum);
    connect(this, &UpdateDialog::setProgressBarFormat, ui->progressBar, &QProgressBar::setFormat);

    // Handle auto update
    this->autoUpdate = autoUpdate;
    if (this->autoUpdate) {
        qDebug() << "Automatically starting update process";
        // Start process automatically
        ui->updateButton->click();
    }
}

UpdateDialog::~UpdateDialog()
{
    delete ui;
}

void UpdateDialog::onAppendLog(const QString &string)
{
    // Log to debug output
    qDebug() << "Update log:" << string;

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
        ui->progressBar->setFormat(tr("Downloading: %p% (%vMiB)", "Progress bar"));
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
    ui->titleLabel->setText(this->originalTitleText);
    onClearProgressBar();
    onAppendLog(reason);
    QMessageBox::warning(this, tr("Update failed", "MessageBox Title"), reason);
}

void UpdateDialog::on_updateButton_clicked()
{
    // Disable update button
    ui->updateButton->setDisabled(true);

    // Ask if user wants to backup their saves
    if (this->autoUpdate == true) {
        backupSaves();
    } else {
        auto backupAnswer = QMessageBox::question(this,
                                                  tr("KeeperFX Update", "Messagebox Title"),
                                                  tr("Updating KeeperFX might break your save files.\n\nDo you want to back them up?", "Messagebox Text"),
                                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        // Handle backup answer
        if (backupAnswer == QMessageBox::No) {
            onAppendLog(tr("Skipping save backup", "Log message"));
        } else if (backupAnswer == QMessageBox::Cancel) {
            // Cancel update process
            ui->updateButton->setDisabled(false);
            ui->titleLabel->setText(this->originalTitleText);
            emit clearProgressBar();
            emit appendLog(tr("Update canceled", "Log message"));
            return;
        } else if (backupAnswer == QMessageBox::Yes) {
            backupSaves();
        }
    }

    // Update GUI to show we are updating
    ui->progressBar->setTextVisible(true);
    ui->titleLabel->setText(tr("Updating...", "Title label"));

    // Tell user we start the installation
    emit appendLog(tr("Updating to version %1", "Log Message").arg(versionInfo.fullString));

    // Try and get filemap
    emit appendLog(tr("Trying to get filemap", "Log Message"));
    auto fileMap = KfxVersion::getGameFileMap(versionInfo.type, versionInfo.version);

    // Check if filemap is found
    if (fileMap.has_value() && !fileMap->isEmpty()) {
        // Show filecount log
        QString fileCount = QString::number(fileMap->count());
        emit appendLog(tr("Filemap with %1 files retrieved", "Log Message (%1=count)").arg(fileCount));
        // Update using filemap
        updateUsingFilemap(fileMap.value());
    } else {
        emit appendLog(tr("No filemap found", "Log Message"));
        // Update using download URL
        updateUsingArchive(versionInfo.downloadUrl);
    }
}

void UpdateDialog::backupSaves()
{
    onAppendLog(tr("Backing up save files...", "Log message"));

    // Backup all save files
    if (SaveFile::backupAll()) {
        emit appendLog(tr("Savefiles have been backed up", "Log Message"));
    }
}

void UpdateDialog::updateUsingFilemap(QMap<QString, QString> fileMap)
{
    emit appendLog(tr("Comparing filemap against local files", "Log Message"));

    // Set progress bar
    emit setProgressMaximum(fileMap.count() - 1);
    emit setProgressBarFormat(tr("Comparing: %p%", "Progress bar (%p=percentage)"));

    // Create fresh file list
    updateList = QStringList();

    // Loop through the filemap
    int progressCount = 0;
    for (auto it = fileMap.begin(); it != fileMap.end(); ++it) {
        QString filePath = it.key();
        QString expectedChecksum = it.value();

        // Removed any leading zeros in the checksum
        expectedChecksum.replace(QRegularExpression("^0+(?!$)"), "");

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
            emit setUpdateFailed(tr("Failed to open file: %1", "Failure message (%1=local filepath)").arg(localFilePath));
            return;
        }

        // Calculate CRC32 checksum
        QByteArray fileData = file.readAll();
        uLong crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, reinterpret_cast<const Bytef *>(fileData.data()), fileData.size());
        QString localChecksum = QString::number(crc, 16);

        // Compare checksums
        if (localChecksum != expectedChecksum) {
            qDebug() << "Checksum difference:" << filePath << ":" << localChecksum << "->"
                     << expectedChecksum;
            updateList.append(filePath);
        }

        // Update progress
        emit updateProgress(++progressCount);
    }

    // Clear progress bar
    emit clearProgressBar();

    // If no files are found
    if (updateList.count() == 0) {
        emit appendLog(tr("No changes in the files have been detected", "Log Message"));
        emit appendLog(tr("Switching to archive download", "Log Message"));
        // Switch to archive download
        updateUsingArchive(versionInfo.downloadUrl);
        return;
    }

    // Log filecount
    emit appendLog(tr("%1 files need to be updated", "Log Message (%1=count)").arg(updateList.count()));

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
    emit appendLog(tr("Downloading archive: %1", "Log Message (%1=archive download URL)").arg(downloadUrlString));

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
        emit setUpdateFailed(tr("Archive download failed.", "Log Message"));
        return;
    }

    emit appendLog(tr("Archive successfully downloaded", "Log Message"));
    emit clearProgressBar();

    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/" + QUrl(versionInfo.downloadUrl).fileName() + ".tmp");

    // Test archive
    emit appendLog(tr("Testing archive...", "Log Message"));
    QThreadPool::globalInstance()->start([this, outputFile]() {
        uint64_t archiveSize = Archiver::testArchiveAndGetSize(outputFile);
        QMetaObject::invokeMethod(this, "onArchiveTestComplete", Qt::QueuedConnection, Q_ARG(uint64_t, archiveSize));
    });
}

void UpdateDialog::onArchiveTestComplete(uint64_t archiveSize){

    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/" + QUrl(versionInfo.downloadUrl).fileName() + ".tmp");

    // Show total size
    double archiveSizeInMiB = static_cast<double>(archiveSize) / (1024 * 1024);
    QString archiveSizeString = QString::number(archiveSizeInMiB, 'f', 2); // Format to 2 decimal places
    emit appendLog(tr("Total size: %1MiB", "Log Message").arg(archiveSizeString));

    // Start extraction process
    emit setProgressMaximum(static_cast<int>(archiveSize));
    emit setProgressBarFormat(tr("Extracting: %p%", "Progress bar"));
    emit appendLog(tr("Extracting...", "Log Message"));

    Updater *updater = new Updater(this);
    connect(updater, &Updater::progress, this, &UpdateDialog::updateProgress);
    connect(updater, &Updater::updateComplete, this, &UpdateDialog::onUpdateComplete);
    connect(updater, &Updater::updateFailed, this, &UpdateDialog::setUpdateFailed);

    updater->updateFromArchive(outputFile);
}

void UpdateDialog::onUpdateComplete()
{
    ui->titleLabel->setText(tr("Update complete!", "Title label"));
    emit appendLog(tr("Extraction completed", "Log Message"));
    emit clearProgressBar();

    // Remove temp archive
    emit appendLog(tr("Removing temporary archive", "Log Message"));
    QFile *archiveFile = new QFile(QCoreApplication::applicationDirPath() + "/" + QUrl(versionInfo.downloadUrl).fileName() + ".tmp");
    if (archiveFile->exists()) {
        archiveFile->remove();
    }

    // Handle any settings update
    emit appendLog(tr("Copying over any new settings", "Log Message"));
    Settings::load();

    // We are done!
    emit appendLog(tr("Done!", "Log Message"));

    // Create message box
    QMessageBox *msgBox = new QMessageBox(QMessageBox::Information,
                                          "KeeperFX",
                                          tr("KeeperFX has been successfully updated to version %1!", "MessageBox Text").arg(versionInfo.version),
                                          QMessageBox::Ok);

    // Close messagebox after a delay if auto updating
    if (this->autoUpdate) {
        QTimer::singleShot(AUTO_UPDATE_MESSAGEBOX_TIMER, msgBox, &QMessageBox::accept);
    }

    // Show messagebox
    msgBox->exec();

    // Accept and close the update dialog
    accept();
}

void UpdateDialog::on_cancelButton_clicked()
{
    close();
}

void UpdateDialog::closeEvent(QCloseEvent *event)
{
    // Ask if user is sure
    int result = QMessageBox::question(this, tr("Confirmation", "MessageBox Title"), tr("Are you sure you want to cancel the update?", "MessageBox Text"));
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
    emit appendLog(tr("Temp directory path: %1", "Log Message (%1=temp dir path)").arg(tempDir.absolutePath()));

    // Make temp directory
    if (!tempDir.exists()) {
        tempDir.mkpath(".");
    }

    // Make sure temp directory exists now
    if (!tempDir.exists()) {
        emit setUpdateFailed(tr("Failed to create temp directory", "Failure message"));
        return;
    }

    // Set Variables
    totalFiles = updateList.size();
    downloadedFiles = 0;
    bool skipLauncherUpdate = LauncherOptions::isSet("skip-launcher-update");

    // Set progress bar to track total files
    emit setProgressMaximum(totalFiles);
    emit setProgressBarFormat(tr("Downloading: %p%", "Progress bar"));

    for (const QString &filePath : updateList) {
        // Check if we need to skip the launcher binary itself
        if (skipLauncherUpdate) {
            if (filePath == "/keeperfx-launcher-qt.exe") {
                qDebug() << "Skipping launcher update:" << filePath;
                continue;
            }
        }

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
                        emit appendLog(tr("Downloaded: %1", "Log Message (%1=filepath)").arg(filePath));
                    } else {
                        emit appendLog(tr("Failed to write: %1", "Log Message (%1=filepath)").arg(tempFilePath));
                    }
                } else {
                    emit appendLog(tr("Failed to open file for writing: %1", "Log Message (%1=filepath)").arg(tempFilePath));
                }
            } else {
                emit appendLog(tr("Failed to download: %1 -> %2", "Log Message (%1=filepath,%2=error message)").arg(filePath).arg(reply->errorString()));
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
        emit appendLog(tr("All files downloaded successfully", "Log Message"));

        // Start copying files to KeeperFX dir
        emit appendLog(tr("Moving files to KeeperFX directory", "Log Message"));

        // Make sure temp dir exists
        if (!tempDir.exists()) {
            emit setUpdateFailed(tr("The temporary directory does not exist", "Log Message"));
            return;
        }

        // Define rename rules
        QMap<QString, QString> renameRules = {
            {"/keeperfx.cfg", "/_keeperfx.cfg"},
            {"/keeperfx-launcher-qt.exe", "/keeperfx-launcher-qt-new.exe"},
        };

        // Set progress bar
        emit setProgressBarFormat(tr("Copying: %p%", "Progress bar"));
        emit setProgressMaximum(totalFiles);

        // Vars
        QDir appDir(QCoreApplication::applicationDirPath());
        int copiedFiles = 0;
        bool skipLauncherUpdate = LauncherOptions::isSet("skip-launcher-update");

        // Move and rename files
        for (const QString &filePath : updateList) {
            // Check if we need to skip the launcher binary itself
            if (skipLauncherUpdate) {
                if (filePath == "/keeperfx-launcher-qt.exe") {
                    qDebug() << "Skipping launcher update:" << filePath;
                    continue;
                }
            }

            QString srcFilePath = tempDir.absolutePath() + filePath;

            // Make sure source file exists
            QFile srcFile(srcFilePath);
            if (!srcFile.exists()) {
                emit appendLog(tr("File does not exist: %1", "Log Message (%1=filepath)").arg(srcFilePath));
                return;
            }

            // Use the new name if exists, otherwise use the same name
            QString destFileName = renameRules.value(filePath, filePath);
            if (destFileName != filePath) {
                emit appendLog(tr("Renaming file during copy: %1 -> %2", "Log Message (%1=filepath,2%=renamed filepath)").arg(filePath).arg(destFileName));
            }

            // Get destination path
            QString destFilePath = appDir.absolutePath() + destFileName;

            // Make sure destination directory exists
            QFileInfo destInfo(destFilePath);
            if (QDir().mkpath(destInfo.absolutePath()) == false) {
                emit appendLog(tr("Failed to create destination directory: %1", "Log Message").arg(destInfo.absolutePath()));
                return;
            }

            // Remove destination file if it exists
            QFile destFile(destFilePath);
            if (destFile.exists()) {
                destFile.remove();
            }

            // Move file
            if (QFile::rename(srcFilePath, destFilePath)) {
                emit appendLog(tr("File moved: %1", "Log Message (%1=filepath)").arg(filePath));
            } else {
                emit appendLog(tr("Failed to move file: %1", "Log Message (%1=filepath)").arg(filePath));
                return;
            }

            emit updateProgress(++copiedFiles);
        }

        // If all files have been updated
        if (copiedFiles == updateList.count()) {
            ui->titleLabel->setText(tr("Update complete!", "Title label"));
            emit clearProgressBar();

            // Copy new settings
            emit appendLog(tr("Copying any new settings", "Log Message"));
            Settings::load();

            // Success!
            emit appendLog(tr("Done!", "Log Message"));

            // Create message box
            QMessageBox *msgBox = new QMessageBox(QMessageBox::Information,
                                                  "KeeperFX",
                                                  tr("KeeperFX has been successfully updated to version %1!", "MessageBox Text").arg(versionInfo.version),
                                                  QMessageBox::Ok);

            // Close messagebox after a delay if auto updating
            if (this->autoUpdate) {
                QTimer::singleShot(AUTO_UPDATE_MESSAGEBOX_TIMER, msgBox, &QMessageBox::accept);
            }

            // Show messagebox
            msgBox->exec();

            // Accept and close update dialog
            accept();
            return;

        } else {
            emit setUpdateFailed(tr("Failed to move all downloaded files", "Failure message"));
        }
    }
}
