#include "installkfxdialog.h"
#include "ui_installkfxdialog.h"

#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>

#include "apiclient.h"
#include "downloader.h"
#include "settings.h"
#include "updater.h"

#include <bit7z/bitabstractarchivehandler.hpp>
#include <bit7z/bitarchivereader.hpp>
#include <bit7z/bitextractor.hpp>

InstallKfxDialog::InstallKfxDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::InstallKfxDialog)
{
    ui->setupUi(this);

    // Log the install path at the start
    appendLog("Install path: " + QCoreApplication::applicationDirPath());
}

void InstallKfxDialog::appendLog(const QString &string)
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

void InstallKfxDialog::updateProgress(int value)
{
    ui->progressBar->setValue(value);
}

void InstallKfxDialog::setProgressMaximum(int value)
{
    ui->progressBar->setMaximum(value);
}

InstallKfxDialog::~InstallKfxDialog()
{
    delete ui;
}

void InstallKfxDialog::on_installButton_clicked()
{
    this->ui->installButton->setDisabled(true);

    // Variable(s)
    bool installAlpha = ui->versionComboBox->currentIndex() == 1;

    // Tell user we start the installation
    appendLog("Installation started");

    // Show progress bar text
    this->ui->progressBar->setTextVisible(true);

    // Remember that we want this release version
    if (installAlpha) {
        Settings::setLauncherSetting("CHECK_FOR_UPDATES_RELEASE", "ALPHA");
    } else {
        Settings::setLauncherSetting("CHECK_FOR_UPDATES_RELEASE", "STABLE");
    }

    // Get download URL for stable
    appendLog("Getting download URL for stable release");
    QUrl downloadUrlStable = ApiClient::getDownloadUrlStable();
    if (downloadUrlStable.isEmpty()) {
        appendLog("Failed to get download URL for stable release");
        return;
    }
    appendLog("Stable release URL: " + downloadUrlStable.toString());

    // Get output for stable
    QString outputFilePath = QCoreApplication::applicationDirPath() + "/"
                             + downloadUrlStable.fileName() + ".tmp";
    QFile *outputFile = new QFile(outputFilePath);

    // Download stable
    Downloader *downloader = new Downloader(this);
    downloader->download(
        downloadUrlStable,
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
        [this, outputFile, installAlpha](bool success) {
            if (!success) {
                this->setInstallFailed("Stable release download failed");
                return;
            }

            this->appendLog("KeeperFX stable successfully downloaded");
            this->clearProgressBar();

            // Test the archive and get the output size
            this->ui->progressBar->setFormat("Testing archive...");
            this->appendLog("Testing stable release archive...");
            uint64_t archiveSize = Updater::testArchiveAndGetSize(outputFile);
            if (archiveSize < 0) {
                this->setInstallFailed("Stable release archive test failed. It may be corrupted.");
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
                this->setInstallFailed("Stable release extraction failed");
                return;
            } else {
                this->appendLog("Stable release extraction completed");
            }

            // If we are only installing the stable
            if (installAlpha == false) {
                // Success
                this->appendLog("Done!");
                QMessageBox::information(this,
                                         "KeeperFX",
                                         "KeeperFX has been successfully installed!");
                this->accept();
                return;
            }

            // Get download URL for alpha
            this->appendLog("Getting download URL for alpha patch");
            QUrl downloadUrlAlpha = ApiClient::getDownloadUrlAlpha();
            if (downloadUrlAlpha.isEmpty()) {
                this->setInstallFailed("Failed to get download URL for alpha patch");
                return;
            }
            this->appendLog("Alpha patch URL: " + downloadUrlAlpha.toString());

            // Get output for stable
            QString alphaArchiveOutputFilePath = QCoreApplication::applicationDirPath() + "/"
                                                 + downloadUrlAlpha.fileName() + ".tmp";
            QFile *alphaArchiveOutputFile = new QFile(alphaArchiveOutputFilePath);

            // Download stable
            this->appendLog("Downloading alpha patch...");
            Downloader *downloader = new Downloader(this);
            downloader->download(
                downloadUrlAlpha,
                alphaArchiveOutputFile,

                // Progress update
                [this](qint64 bytesReceived, qint64 bytesTotal) {
                    if (bytesTotal > 0) {
                        this->ui->progressBar->setTextVisible(true);
                        this->ui->progressBar->setMaximum(
                            static_cast<int>(bytesTotal / 1024 / 1024));
                        this->ui->progressBar->setValue(
                            static_cast<int>(bytesReceived / 1024 / 1024));
                        this->ui->progressBar->setFormat(QString("Downloading: %p% (%vMiB)"));
                    }
                },

                // Completion
                [this, alphaArchiveOutputFile](bool success) {
                    if (!success) {
                        this->ui->progressBar->setValue(0);
                        this->ui->progressBar->setTextVisible(false);
                        this->appendLog("Alpha patch download failed.");
                        return;
                    }

                    this->appendLog("KeeperFX alpha patch successfully downloaded");

                    // Set progress bar
                    this->ui->progressBar->setMaximum(1);
                    this->ui->progressBar->setValue(0);
                    this->ui->progressBar->setFormat("");

                    // Test the archive and get the output size
                    this->appendLog("Testing alpha patch archive...");
                    uint64_t archiveSize = Updater::testArchiveAndGetSize(alphaArchiveOutputFile);
                    if (archiveSize < 0) {
                        this->setInstallFailed(
                            "Alpha patch archive test failed. It may be corrupted.");
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
                    bool updateResult
                        = Updater::updateFromArchive(alphaArchiveOutputFile,
                                                     [this](uint64_t processed_size) {
                                                         // Update progress bar
                                                         this->ui->progressBar->setValue(
                                                             static_cast<int>(processed_size));
                                                         return true; // Continue extraction
                                                     });

                    this->clearProgressBar();

                    // Clean up
                    alphaArchiveOutputFile->remove();

                    // Check if extraction was a success
                    if (updateResult == false) {
                        this->setInstallFailed("Alpha patch extraction failed");
                        return;
                    } else {
                        this->appendLog("Alpha patch extraction completed");
                    }

                    // Success
                    this->appendLog("Done!");
                    this->clearProgressBar();
                    QMessageBox::information(this,
                                             "KeeperFX",
                                             "KeeperFX has been successfully installed!");
                    this->accept();
                    return;
                });
        });
}

void InstallKfxDialog::clearProgressBar()
{
    this->ui->progressBar->setValue(0);
    this->ui->progressBar->setMaximum(1);
    this->ui->progressBar->setFormat("");
}

void InstallKfxDialog::setInstallFailed(const QString &reason)
{
    this->ui->installButton->setDisabled(false);
    this->clearProgressBar();
    this->appendLog(reason);

    QMessageBox::warning(this, "Installation failed", reason);
}

void InstallKfxDialog::on_versionComboBox_currentIndexChanged(int index)
{
    // 0 => Stable
    // 1 => Alpha

    if (index == 1) {
        int result = QMessageBox::question(
            this,
            "KeeperFX Alpha builds",
            "Alpha patches may contain new features and changes, but often contain "
            "bugs and unfinished functionality. These patches are mostly meant "
            "for testers, and it is suggested to be part of the Keeper Klan discord "
            "if you use them.\n\n"
            "Are you sure you want to use Alpha patches?");

        // Check whether or not user is sure
        if (result != QMessageBox::Yes) {
            // Set back to Stable if they are not sure
            ui->versionComboBox->setCurrentIndex(0);
        }
    }
}

void InstallKfxDialog::on_cancelButton_clicked()
{
    // Call the close() method to trigger the close event
    close();
}

void InstallKfxDialog::closeEvent(QCloseEvent *event)
{
    // Ask if user is sure
    int result = QMessageBox::question(this,
                                       "Confirmation",
                                       "Are you sure?\n\nYou will be unable to play KeeperFX.");
    if (result == QMessageBox::Yes) {
        // Allow the dialog to close
        event->accept();

    } else {
        // Ignore the close event
        event->ignore();
    }
}
