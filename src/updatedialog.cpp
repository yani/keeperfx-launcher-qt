#include "updatedialog.h"
#include <QMessageBox>
#include <QCloseEvent>

UpdateDialog::UpdateDialog(QWidget *parent, KfxVersion::VersionInfo versionInfo)
    : QDialog(parent)
    , ui(new Ui::UpdateDialog)
{
    // Setup this UI
    ui->setupUi(this);

    // Get current version string
    QString currentVersionString = KfxVersion::currentVersion.string;
    if (KfxVersion::currentVersion.type == KfxVersion::ReleaseType::ALPHA) {
        currentVersionString += " Alpha";
    }

    // Get new version string
    QString newVersionString = versionInfo.version;
    if (versionInfo.type == KfxVersion::ReleaseType::ALPHA) {
        newVersionString += " Alpha";
    }

    // Update version text in UI
    QString infoLabelText = ui->infoLabel->text();
    infoLabelText.replace("<CURRENT_VERSION>", currentVersionString);
    infoLabelText.replace("<NEW_VERSION>", newVersionString);
    ui->infoLabel->setText(infoLabelText);

    // Log the update path
    appendLog("Update path: " + QCoreApplication::applicationDirPath());

    // Try to get filemap for this update
    /*    auto fileMap = KfxVersion::getGameFileMap(latestVersionInfo->type,
                                              latestVersionInfo->version);
    */

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

void UpdateDialog::on_updateButton_clicked() {}

void UpdateDialog::on_cancelButton_clicked() {
    // Call the close() method to trigger the close event
    close();
}

void UpdateDialog::closeEvent(QCloseEvent *event)
{
    // Ask if user is sure
    int result = QMessageBox::question(this,
                                       "Confirmation",
                                       "Are you sure you don't want to update?");
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
