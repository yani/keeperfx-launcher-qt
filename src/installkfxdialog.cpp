#include "installkfxdialog.h"
#include "ui_installkfxdialog.h"

#include <QDateTime>
#include <QFileInfo>
#include <QMainWindow>
#include <QMessageBox>
#include <QScrollBar>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>

#include "apiclient.h"
#include "archiver.h"
#include "downloader.h"
#include "launcheroptions.h"
#include "settings.h"
#include "translator.h"
#include "updater.h"

InstallKfxDialog::InstallKfxDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::InstallKfxDialog)
{
    ui->setupUi(this);

    // Turn this dialog into a normal window
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    // Fixed size, portable across Wayland/X11/Windows
    setFixedSize(size());

    // Move to center of primary screen
    QRect geometry = QGuiApplication::primaryScreen()->geometry();
    move(geometry.left() + (geometry.width() - width()) / 2, geometry.top() + (geometry.height() - height()) / 2 - 75); // move 75 pixels up

    // Bring to foreground
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();

    // Release dropdown
    ui->versionComboBox->addItem(tr("Stable (Default)", "Game Release Build"), "STABLE");
    ui->versionComboBox->addItem(tr("Alpha", "Game Release Build"), "ALPHA");
    ui->versionComboBox->setCurrentIndex(ui->versionComboBox->findData("STABLE"));

    // Setup signals and slots
    connect(this, &InstallKfxDialog::appendLog, this, &InstallKfxDialog::onAppendLog);
    connect(this, &InstallKfxDialog::clearProgressBar, this, &InstallKfxDialog::onClearProgressBar);
    connect(this, &InstallKfxDialog::setInstallFailed, this, &InstallKfxDialog::onInstallFailed);

    connect(this, &InstallKfxDialog::updateProgressBar, ui->progressBar, &QProgressBar::setValue);
    connect(this, &InstallKfxDialog::setProgressMaximum, ui->progressBar, &QProgressBar::setMaximum);
    connect(this, &InstallKfxDialog::setProgressBarFormat, ui->progressBar, &QProgressBar::setFormat);

    // Log the install path at the start
    emit appendLog(QString("Install path: %1").arg(QCoreApplication::applicationDirPath()));
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
    emit appendLog("Installation started");

    // Reset settings
    Settings::resetKfxSettings();
    Settings::resetLauncherSettings();

    // Get release type to install
    // Also remember that we want this release version for later updates
    if (ui->versionComboBox->currentData() == "STABLE") {
        this->installReleaseType = KfxVersion::ReleaseType::STABLE;
        Settings::setLauncherSetting("CHECK_FOR_UPDATES_RELEASE", "STABLE");
    } else if (ui->versionComboBox->currentData() == "ALPHA") {
        this->installReleaseType = KfxVersion::ReleaseType::ALPHA;
        Settings::setLauncherSetting("CHECK_FOR_UPDATES_RELEASE", "ALPHA");
    }

    // Set launcher language to installation language
    if (LauncherOptions::isSet("language")) {
        QString languageCode = LauncherOptions::getValue("language").toLower();
        if (languageCode == "en") {
            Settings::setLauncherSetting("LAUNCHER_LANGUAGE", "en");
        } else {
            // Create dummy translator to check if this language works
            Translator *translator = new Translator;
            if (translator->loadLanguage(languageCode)) {
                Settings::setLauncherSetting("LAUNCHER_LANGUAGE", languageCode);
            }
        }
    }

    // Start the installation process
    // Even for the alpha release, we first need the stable
    startStableDownload();
}

void InstallKfxDialog::startStableDownload()
{
    emit appendLog("Getting download URL for stable release...");
    downloadUrlStable = ApiClient::getDownloadUrlStable();

    if (downloadUrlStable.isEmpty()) {
        emit appendLog("Failed to get download URL for stable release");
        emit setInstallFailed(tr("Failed to get download URL for stable release", "Failure Message"));
        return;
    }

    emit appendLog(QString("Stable release URL: %1").arg(downloadUrlStable.toString()));

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
        emit appendLog("Failed to download stable release");
        emit setInstallFailed(tr("Failed to download stable release", "Failure Message"));
        return;
    }

    emit appendLog("KeeperFX stable release successfully downloaded");
    emit clearProgressBar();

    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/" + this->downloadUrlStable.fileName() + ".tmp");

    // Test archive
    emit appendLog("Testing stable release archive...");
    QThreadPool::globalInstance()->start([this, outputFile]() {
        uint64_t archiveSize = Archiver::testArchiveAndGetSize(outputFile);
        QMetaObject::invokeMethod(this, "onStableArchiveTestComplete", Qt::QueuedConnection, Q_ARG(uint64_t, archiveSize));
    });
}

void InstallKfxDialog::onStableArchiveTestComplete(uint64_t archiveSize) {

    // Make sure test is successful and archive size is valid
    if (archiveSize < 0) {
        emit appendLog("Stable release archive test failed");
        emit setInstallFailed(tr("Stable release archive test failed. It may be corrupted.", "Failure message"));
        return;
    }

    // Get size
    double archiveSizeInMiB = static_cast<double>(archiveSize) / (1024 * 1024);
    QString archiveSizeString = QString::number(archiveSizeInMiB,
                                                'f',
                                                2); // Format to 2 decimal places
    emit appendLog(QString("Total size: %1MiB").arg(archiveSizeString));

    // Start extraction process
    emit setProgressMaximum(static_cast<int>(archiveSize));
    emit setProgressBarFormat(tr("Extracting: %p%", "Progress bar"));
    emit appendLog("Extracting...");

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
    emit appendLog("Extraction completed");
    emit clearProgressBar();

    // Remove temp archive
    emit appendLog("Removing temporary archive");
    QFile *archiveFile = new QFile(QCoreApplication::applicationDirPath() + "/" + downloadUrlStable.fileName() + ".tmp");
    if (archiveFile->exists()) {
        archiveFile->remove();
    }

    if (this->installReleaseType == KfxVersion::ReleaseType::ALPHA) {
        startAlphaDownload();
    } else {
        // Handle any settings update
        emit appendLog("Loading settings");
        Settings::load();

        emit appendLog("Done!");
        QMessageBox::information(this, "KeeperFX", tr("KeeperFX has been successfully installed!", "MessageBox Text"));
        accept();
    }
}

void InstallKfxDialog::startAlphaDownload()
{
    emit appendLog("Getting download URL for alpha patch...");
    downloadUrlAlpha = ApiClient::getDownloadUrlAlpha();

    if (downloadUrlAlpha.isEmpty()) {
        emit appendLog("Failed to get download URL for alpha patch");
        emit setInstallFailed(tr("Failed to get download URL for alpha patch", "Failure message"));
        return;
    }

    emit appendLog(QString("Alpha patch URL: %1").arg(downloadUrlAlpha.toString()));

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
        emit appendLog("Alpha patch download failed");
        emit setInstallFailed(tr("Alpha patch download failed.", "Failure message"));
        return;
    }

    emit appendLog("KeeperFX alpha patch successfully downloaded");

    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/" + downloadUrlAlpha.fileName() + ".tmp");

    // Test archive
    emit appendLog("Testing alpha patch archive...");
    QThreadPool::globalInstance()->start([this, outputFile]() {
        uint64_t archiveSize = Archiver::testArchiveAndGetSize(outputFile);
        QMetaObject::invokeMethod(this, "onAlphaArchiveTestComplete", Qt::QueuedConnection, Q_ARG(uint64_t, archiveSize));
    });
}

void InstallKfxDialog::onAlphaArchiveTestComplete(uint64_t archiveSize)
{
    // Make sure test is successful and archive size is valid
    if (archiveSize < 0) {
        emit appendLog("Alpha patch archive test failed");
        emit setInstallFailed(tr("Alpha patch archive test failed. It may be corrupted.", "Failure Message"));
        return;
    }

    // Get size
    double archiveSizeInMiB = static_cast<double>(archiveSize) / (1024 * 1024);
    QString archiveSizeString = QString::number(archiveSizeInMiB, 'f', 2); // Format to 2 decimal places
    emit appendLog(QString("Total size: %1MiB").arg(archiveSizeString));

    // Start extraction process
    emit setProgressMaximum(static_cast<int>(archiveSize));
    emit setProgressBarFormat(tr("Extracting: %p%", "Progress bar"));
    emit appendLog("Extracting...");

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
    emit appendLog("Extraction completed");
    emit clearProgressBar();

    // Remove temp archive
    emit appendLog("Removing temporary archive");
    QFile *archiveFile = new QFile(QCoreApplication::applicationDirPath() + "/" + downloadUrlAlpha.fileName() + ".tmp");
    if (archiveFile->exists()) {
        archiveFile->remove();
    }

    // Handle any settings update
    emit appendLog("Loading settings");
    Settings::load();

    emit appendLog("Setting game language to system language");
    Settings::autoSetGameLanguageToLocaleLanguage();

    emit appendLog("Done!");
    QMessageBox::information(this, "KeeperFX", tr("KeeperFX has been successfully installed!", "MessageBox Text"));
    this->accept();
}

void InstallKfxDialog::updateProgressBarDownload(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        ui->progressBar->setMaximum(static_cast<int>(bytesTotal / 1024 / 1024));
        ui->progressBar->setValue(static_cast<int>(bytesReceived / 1024 / 1024));
        ui->progressBar->setFormat(tr("Downloading: %p% (%vMiB)", "Progress bar"));
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
    QMessageBox::warning(this, tr("Installation failed", "MessageBox Title"), reason);
}

void InstallKfxDialog::on_versionComboBox_currentIndexChanged(int index)
{
    if (ui->versionComboBox->currentData() == "ALPHA") {
        int result = QMessageBox::question(this,
                                           tr("KeeperFX Alpha builds", "MessageBox Title"),
                                           tr("Alpha patches may contain new features and changes, but often contain "
                                              "bugs and unfinished functionality. These patches are mostly meant "
                                              "for testers, and it is suggested to be part of the Keeper Klan discord "
                                              "if you use them.\n\nAre you sure you want to use Alpha patches?",
                                              "MessageBox Text"));

        // Check whether or not user is sure
        if (result != QMessageBox::Yes) {
            // Set back to Stable if they are not sure
            ui->versionComboBox->setCurrentIndex(ui->versionComboBox->findData("STABLE"));
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
    int result = QMessageBox::question(this, tr("Confirmation", "MessageBox Title"), tr("Are you sure?\n\nYou will be unable to play KeeperFX.", "MessageBox Text"));

    // Handle answer
    if (result == QMessageBox::Yes) {
        event->accept();
    } else {
        event->ignore();
    }
}
