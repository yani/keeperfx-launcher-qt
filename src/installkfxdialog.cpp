#include "installkfxdialog.h"
#include "ui_installkfxdialog.h"

#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>

#include "apiclient.h"
#include "archiver.h"
#include "downloader.h"
#include "settings.h"
#include "updater.h"

InstallKfxDialog::InstallKfxDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::InstallKfxDialog)
{
    ui->setupUi(this);

    // Disable resizing and remove maximize button
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    // Setup signals and slots
    connect(this, &InstallKfxDialog::appendLog, this, &InstallKfxDialog::onAppendLog);
    connect(this, &InstallKfxDialog::clearProgressBar, this, &InstallKfxDialog::onClearProgressBar);
    connect(this, &InstallKfxDialog::setInstallFailed, this, &InstallKfxDialog::onInstallFailed);

    connect(this, &InstallKfxDialog::updateProgressBar, ui->progressBar, &QProgressBar::setValue);
    connect(this, &InstallKfxDialog::setProgressMaximum, ui->progressBar, &QProgressBar::setMaximum);
    connect(this, &InstallKfxDialog::setProgressBarFormat, ui->progressBar, &QProgressBar::setFormat);

    // Log the install path at the start
    emit appendLog(tr("Install path") + ": " + QCoreApplication::applicationDirPath());
}

InstallKfxDialog::~InstallKfxDialog()
{
    delete ui;
}

void InstallKfxDialog::on_installButton_clicked()
{
    // Change GUI
    ui->installButton->setDisabled(true);
    ui->progressBar->setTextVisible(true);

    // Tell user we start the installation
    emit appendLog(tr("Installation started"));

    // Get release type to install
    // Also remember that we want this release version for later updates
    if (ui->versionComboBox->currentIndex() == 0) {
        this->installReleaseType = KfxVersion::ReleaseType::STABLE;
        Settings::setLauncherSetting("CHECK_FOR_UPDATES_RELEASE", "STABLE");
    } else if (ui->versionComboBox->currentIndex() == 1) {
        this->installReleaseType = KfxVersion::ReleaseType::ALPHA;
        Settings::setLauncherSetting("CHECK_FOR_UPDATES_RELEASE", "ALPHA");
    }

    // Start the installation process
    // Even for the alpha release, we first need the stable
    startStableDownload();
}

void InstallKfxDialog::startStableDownload()
{
    emit appendLog(tr("Getting download URL for stable release"));
    downloadUrlStable = ApiClient::getDownloadUrlStable();

    if (downloadUrlStable.isEmpty()) {
        emit appendLog(tr("Failed to get download URL for stable release"));
        return;
    }

    emit appendLog(tr("Stable release URL") + ": " + downloadUrlStable.toString());

    QString outputFilePath = QCoreApplication::applicationDirPath() + "/" + downloadUrlStable.fileName() + ".tmp";
    QFile *outputFile = new QFile(outputFilePath);

    Downloader *downloader = new Downloader(this);
    connect(downloader, &Downloader::downloadProgress, this, &InstallKfxDialog::updateProgressBarDownload);
    connect(downloader, &Downloader::downloadCompleted, this, &InstallKfxDialog::onStableDownloadFinished);

    downloader->download(downloadUrlStable, outputFile);
}

void InstallKfxDialog::onStableDownloadFinished(bool success)
{
    if (!success) {
        emit setInstallFailed(tr("Failed to download stable release"));
        return;
    }

    emit appendLog(tr("KeeperFX stable release successfully downloaded"));
    emit clearProgressBar();

    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/" + this->downloadUrlStable.fileName() + ".tmp");

    // Test archive
    emit appendLog(tr("Testing stable release archive..."));
    QThreadPool::globalInstance()->start([this, outputFile]() {
        uint64_t archiveSize = Archiver::testArchiveAndGetSize(outputFile);
        QMetaObject::invokeMethod(this, "onStableArchiveTestComplete", Qt::QueuedConnection, Q_ARG(uint64_t, archiveSize));
    });
}

void InstallKfxDialog::onStableArchiveTestComplete(uint64_t archiveSize) {

    // Make sure test is successful and archive size is valid
    if (archiveSize < 0) {
        emit setInstallFailed(tr("Stable release archive test failed. It may be corrupted."));
        return;
    }

    // Get size
    double archiveSizeInMiB = static_cast<double>(archiveSize) / (1024 * 1024);
    QString archiveSizeString = QString::number(archiveSizeInMiB,
                                                'f',
                                                2); // Format to 2 decimal places
    emit appendLog(tr("Total size") + ": " + archiveSizeString + "MiB");

    // Start extraction process
    emit setProgressMaximum(static_cast<int>(archiveSize));
    emit setProgressBarFormat(tr("Extracting: %p%"));
    emit appendLog(tr("Extracting..."));

    // TODO: use temp file
    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/"
                                  + downloadUrlStable.fileName() + ".tmp");

    Updater *updater = new Updater(this);
    connect(updater, &Updater::progress, this, &InstallKfxDialog::updateProgressBar);
    connect(updater, &Updater::updateComplete, this, &InstallKfxDialog::onStableExtractComplete);
    connect(updater, &Updater::updateFailed, this, &InstallKfxDialog::setInstallFailed);

    updater->updateFromArchive(outputFile);
}

void InstallKfxDialog::onStableExtractComplete()
{
    emit appendLog(tr("Extraction completed"));
    emit clearProgressBar();

    // Remove temp archive
    emit appendLog(tr("Removing temporary archive"));
    QFile *archiveFile = new QFile(QCoreApplication::applicationDirPath() + "/" + downloadUrlStable.fileName() + ".tmp");
    if (archiveFile->exists()) {
        archiveFile->remove();
    }

    if (this->installReleaseType == KfxVersion::ReleaseType::ALPHA) {
        startAlphaDownload();
    } else {
        // Handle any settings update
        emit appendLog(tr("Loading settings"));
        Settings::load();

        emit appendLog(tr("Done!"));
        QMessageBox::information(this, "KeeperFX", tr("KeeperFX has been successfully installed!"));
        accept();
    }
}

void InstallKfxDialog::startAlphaDownload()
{
    emit appendLog(tr("Getting download URL for alpha patch"));
    downloadUrlAlpha = ApiClient::getDownloadUrlAlpha();

    if (downloadUrlAlpha.isEmpty()) {
        emit setInstallFailed(tr("Failed to get download URL for alpha patch"));
        return;
    }

    emit appendLog(tr("Alpha patch URL") + ": " + downloadUrlAlpha.toString());

    QString outputFilePath = QCoreApplication::applicationDirPath() + "/" + downloadUrlAlpha.fileName() + ".tmp";
    QFile *outputFile = new QFile(outputFilePath);

    Downloader *downloader = new Downloader(this);
    connect(downloader, &Downloader::downloadProgress, this, &InstallKfxDialog::updateProgressBarDownload);
    connect(downloader, &Downloader::downloadCompleted, this, &InstallKfxDialog::onAlphaDownloadFinished);

    downloader->download(downloadUrlAlpha, outputFile);
}

void InstallKfxDialog::onAlphaDownloadFinished(bool success)
{
    if (!success) {
        emit setInstallFailed(tr("Alpha patch download failed."));
        return;
    }

    emit appendLog(tr("KeeperFX alpha patch successfully downloaded"));

    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/" + downloadUrlAlpha.fileName() + ".tmp");

    // Test archive
    emit appendLog(tr("Testing alpha patch archive..."));
    QThreadPool::globalInstance()->start([this, outputFile]() {
        uint64_t archiveSize = Archiver::testArchiveAndGetSize(outputFile);
        QMetaObject::invokeMethod(this, "onAlphaArchiveTestComplete", Qt::QueuedConnection, Q_ARG(uint64_t, archiveSize));
    });
}

void InstallKfxDialog::onAlphaArchiveTestComplete(uint64_t archiveSize)
{
    // Make sure test is successful and archive size is valid
    if (archiveSize < 0) {
        emit setInstallFailed(tr("Alpha patch archive test failed. It may be corrupted."));
        return;
    }

    // Get size
    double archiveSizeInMiB = static_cast<double>(archiveSize) / (1024 * 1024);
    QString archiveSizeString = QString::number(archiveSizeInMiB, 'f', 2); // Format to 2 decimal places
    emit appendLog(tr("Total size") + ": " + archiveSizeString + "MiB");

    // Start extraction process
    emit setProgressMaximum(static_cast<int>(archiveSize));
    emit setProgressBarFormat(tr("Extracting: %p%"));
    emit appendLog(tr("Extracting..."));

    // TODO: use temp file
    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/"
                                  + downloadUrlAlpha.fileName() + ".tmp");

    Updater *updater = new Updater(this);
    connect(updater, &Updater::progress, this, &InstallKfxDialog::updateProgressBar);
    connect(updater, &Updater::updateComplete, this, &InstallKfxDialog::onAlphaExtractComplete);
    connect(updater, &Updater::updateFailed, this, &InstallKfxDialog::setInstallFailed);

    updater->updateFromArchive(outputFile);
}

void InstallKfxDialog::onAlphaExtractComplete()
{
    emit appendLog(tr("Extraction completed"));
    emit clearProgressBar();

    // Remove temp archive
    emit appendLog(tr("Removing temporary archive"));
    QFile *archiveFile = new QFile(QCoreApplication::applicationDirPath() + "/" + downloadUrlAlpha.fileName() + ".tmp");
    if (archiveFile->exists()) {
        archiveFile->remove();
    }

    // Handle any settings update
    emit appendLog(tr("Loading settings"));
    Settings::load();

    emit appendLog(tr("Setting game language to system language"));
    Settings::autoSetGameLanguageToLocaleLanguage();

    emit appendLog(tr("Done!"));
    QMessageBox::information(this, "KeeperFX", tr("KeeperFX has been successfully installed!"));
    this->accept();
}

void InstallKfxDialog::updateProgressBarDownload(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        ui->progressBar->setMaximum(static_cast<int>(bytesTotal / 1024 / 1024));
        ui->progressBar->setValue(static_cast<int>(bytesReceived / 1024 / 1024));
        ui->progressBar->setFormat(QString(tr("Downloading") + ": %p% (%vMiB)"));
    }
}

void InstallKfxDialog::onAppendLog(const QString &string)
{
    // Log to debug output
    qDebug() << "Install log:" << string;

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

void InstallKfxDialog::onClearProgressBar()
{
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(1);
    ui->progressBar->setFormat("");
}

void InstallKfxDialog::onInstallFailed(const QString &reason)
{
    ui->installButton->setDisabled(false);
    onClearProgressBar();
    onAppendLog(reason);
    QMessageBox::warning(this, tr("Installation failed"), reason);
}

void InstallKfxDialog::on_versionComboBox_currentIndexChanged(int index)
{
    if (index == 1) {
        int result = QMessageBox::question(
            this,
            tr("KeeperFX Alpha builds"),
            tr("Alpha patches may contain new features and changes, but often contain "
               "bugs and unfinished functionality. These patches are mostly meant "
               "for testers, and it is suggested to be part of the Keeper Klan discord "
               "if you use them.") +
            "\n\n" +
            tr("Are you sure you want to use Alpha patches?"));

        // Check whether or not user is sure
        if (result != QMessageBox::Yes) {
            // Set back to Stable if they are not sure
            ui->versionComboBox->setCurrentIndex(0);
        }
    }
}

void InstallKfxDialog::on_cancelButton_clicked()
{
    close();
}

void InstallKfxDialog::closeEvent(QCloseEvent *event)
{
    // Ask if user is sure
    int result = QMessageBox::question(this, tr("Confirmation"),
        tr("Are you sure?") + "\n\n" + tr("You will be unable to play KeeperFX."));

    // Handle answer
    if (result == QMessageBox::Yes) {
        event->accept();
    } else {
        event->ignore();
    }
}
