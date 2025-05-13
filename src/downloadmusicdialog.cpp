#include "downloadmusicdialog.h"
#include "apiclient.h"
#include "archiver.h"
#include "downloader.h"
#include "settings.h"
#include "ui_downloadmusicdialog.h"
#include "updater.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QMessageBox>
#include <QScrollBar>
#include <QThreadPool>

DownloadMusicDialog::DownloadMusicDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DownloadMusicDialog)
{
    ui->setupUi(this);

    // Disable resizing and remove maximize button
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    // Disable resizing and remove maximize button
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(size()); // Prevent resizing by setting fixed size

    // Setup signals and slots
    connect(this, &DownloadMusicDialog::appendLog, this, &DownloadMusicDialog::onAppendLog);
    connect(this, &DownloadMusicDialog::clearProgressBar, this, &DownloadMusicDialog::onClearProgressBar);
    connect(this, &DownloadMusicDialog::setDownloadFailed, this, &DownloadMusicDialog::onDownloadFailed);

    connect(this, &DownloadMusicDialog::updateProgressBar, ui->progressBar, &QProgressBar::setValue);
    connect(this, &DownloadMusicDialog::setProgressMaximum, ui->progressBar, &QProgressBar::setMaximum);
    connect(this, &DownloadMusicDialog::setProgressBarFormat, ui->progressBar, &QProgressBar::setFormat);
}

DownloadMusicDialog::~DownloadMusicDialog()
{
    delete ui;
}

void DownloadMusicDialog::on_cancelButton_clicked()
{
    this->close();
}

void DownloadMusicDialog::on_downloadButton_clicked()
{
    // Change GUI
    ui->downloadButton->setDisabled(true);
    ui->progressBar->setTextVisible(true);

    // Start download
    startDownload();
}

void DownloadMusicDialog::startDownload()
{
    // Get download URL
    emit appendLog(tr("Getting download URL for music archive", "Log Message"));
    this->downloadUrl = ApiClient::getDownloadUrlMusic();
    if (downloadUrl.isEmpty()) {
        emit setDownloadFailed(tr("Failed to get download URL for music archive", "Log Message"));
        return;
    }

    // Show download URL to end user
    emit appendLog(tr("Music archive URL: %1", "Log Message").arg(downloadUrl.toString()));

    // Make sure file is a 7zip archive
    if (downloadUrl.toString().endsWith(".7z") == false) {
        emit setDownloadFailed(tr("Invalid music archive file extension. It must be a 7zip archive.", "Log Message"));
        return;
    }

    QString outputFilePath = QCoreApplication::applicationDirPath() + "/" + downloadUrl.fileName() + ".tmp";
    QFile *outputFile = new QFile(outputFilePath);

    Downloader *downloader = new Downloader(this);
    connect(downloader, &Downloader::downloadProgress, this, &DownloadMusicDialog::updateProgressBarDownload);
    connect(downloader, &Downloader::downloadCompleted, this, &DownloadMusicDialog::onDownloadFinished);

    downloader->download(downloadUrl, outputFile);
}

void DownloadMusicDialog::onDownloadFinished(bool success)
{
    if (!success) {
        emit setDownloadFailed(tr("Failed to download music archive", "Log Message"));
        return;
    }

    emit appendLog(tr("Music archive successfully downloaded", "Log Message"));
    emit clearProgressBar();

    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/" + this->downloadUrl.fileName() + ".tmp");

    // Test archive
    emit appendLog(tr("Testing music archive...", "Log Message"));
    QThreadPool::globalInstance()->start([this, outputFile]() {
        uint64_t archiveSize = Archiver::testArchiveAndGetSize(outputFile);
        QMetaObject::invokeMethod(this, "onArchiveTestComplete", Qt::QueuedConnection, Q_ARG(uint64_t, archiveSize));
    });
}

void DownloadMusicDialog::onArchiveTestComplete(uint64_t archiveSize)
{
    // Make sure test is successful and archive size is valid
    if (archiveSize < 0) {
        emit setDownloadFailed(tr("Music archive test failed. It may be corrupted.", "Failure message"));
        return;
    }

    // Get size
    double archiveSizeInMiB = static_cast<double>(archiveSize) / (1024 * 1024);
    QString archiveSizeString = QString::number(archiveSizeInMiB, 'f',
                                                2); // Format to 2 decimal places
    emit appendLog(tr("Total size: %1MiB", "Log Message").arg(archiveSizeString));

    // Start extraction process
    emit setProgressMaximum(static_cast<int>(archiveSize));
    emit setProgressBarFormat(tr("Extracting: %p%", "Progress bar"));
    emit appendLog(tr("Extracting...", "Log Message"));

    // TODO: use temp file
    QFile *outputFile = new QFile(QCoreApplication::applicationDirPath() + "/" + downloadUrl.fileName() + ".tmp");

    Updater *updater = new Updater(this);
    connect(updater, &Updater::progress, this, &DownloadMusicDialog::updateProgressBar);
    connect(updater, &Updater::updateComplete, this, &DownloadMusicDialog::onExtractComplete);
    connect(updater, &Updater::updateFailed, this, &DownloadMusicDialog::setDownloadFailed);

    updater->updateFromArchive(outputFile);
}

void DownloadMusicDialog::onExtractComplete()
{
    emit appendLog(tr("Extraction completed", "Log Message"));
    emit clearProgressBar();

    // Remove temp archive
    emit appendLog(tr("Removing temporary archive", "Log Message"));
    QFile *archiveFile = new QFile(QCoreApplication::applicationDirPath() + "/" + downloadUrl.fileName() + ".tmp");
    if (archiveFile->exists()) {
        archiveFile->remove();
    }

    // Disable CD music setting
    emit appendLog(tr("Disabling CD music setting", "Log Message"));
    Settings::setLauncherSetting("GAME_PARAM_USE_CD_MUSIC", false);

    // Done!
    emit appendLog(tr("Done!", "Log Message"));
    QMessageBox::information(this, "KeeperFX", tr("The KeeperFX background music has been successfully downloaded!", "MessageBox Text"));
    accept();
}

void DownloadMusicDialog::updateProgressBarDownload(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        ui->progressBar->setMaximum(static_cast<int>(bytesTotal / 1024 / 1024));
        ui->progressBar->setValue(static_cast<int>(bytesReceived / 1024 / 1024));
        ui->progressBar->setFormat(tr("Downloading: %p% (%vMiB)", "Progress bar"));
    }
}

void DownloadMusicDialog::onAppendLog(const QString &string)
{
    // Log to debug output
    qDebug() << "Download log:" << string;

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

void DownloadMusicDialog::onClearProgressBar()
{
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(1);
    ui->progressBar->setFormat("");
}

void DownloadMusicDialog::onDownloadFailed(const QString &reason)
{
    ui->downloadButton->setDisabled(false);
    onClearProgressBar();
    onAppendLog(reason);
    QMessageBox::warning(this, tr("Download failed", "MessageBox Title"), reason);
}

void DownloadMusicDialog::closeEvent(QCloseEvent *event)
{
    // Ask if user is sure
    int result = QMessageBox::question(this,
                                       tr("Confirmation", "MessageBox Title"),
                                       tr("Are you sure?\n\nYou will not be able to hear the background music without having your CD in your disk drive.", "MessageBox Text"));

    // Handle answer
    if (result == QMessageBox::Yes) {
        event->accept();
    } else {
        event->ignore();
    }
}
