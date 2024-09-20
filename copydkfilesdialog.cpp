#include "copydkfilesdialog.h"
#include "ui_copydkfilesdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>

#include "dkfiles.h"

CopyDkFilesDialog::CopyDkFilesDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CopyDkFilesDialog)
{
    ui->setupUi(this);
    ui->copyButton->setDisabled(true);

    // Disable resizing and remove maximize button
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(size()); // Prevent resizing by setting fixed size

    // Automatically check for a DK install directory
    QDir *existingDkInstallDir = DkFiles::findExistingDkInstallDir();
    if(existingDkInstallDir){
        qDebug() << "Automatically detected DK dir:" << existingDkInstallDir->absolutePath();
        ui->copyButton->setDisabled(false);
        ui->browseInput->setText(existingDkInstallDir->absolutePath());
    }
}

CopyDkFilesDialog::~CopyDkFilesDialog()
{
    delete ui;
}

void CopyDkFilesDialog::on_browseButton_clicked()
{
    // Create a QFileDialog instance and set options
    QString dirPath = QFileDialog::getExistingDirectory(this, "Select Directory containing Dungeon Keeper files", "", QFileDialog::ShowDirsOnly);

    // Check if a valid directory was selected
    if (DkFiles::isValidDkDirPath(dirPath)) {
        // Set the directory path in the QLineEdit (browseInput)
        ui->browseInput->setText(dirPath);
        ui->copyButton->setDisabled(false);
    } else {
        ui->browseInput->setText("");
        ui->copyButton->setDisabled(true);
        QMessageBox::warning(this, "Original DK files not found", "The chosen directory does not contain all of the required files. Please select an original Dungeon Keeper installation.");
    }
}

void CopyDkFilesDialog::on_copyButton_clicked()
{
    QDir *dkDir = new QDir(ui->browseInput->text());

    // Check if given dir is a valid DK dir
    if(!DkFiles::isValidDkDir(dkDir)){
        QMessageBox::warning(this, "Original DK files not found", "The chosen directory does not contain all of the required files. Please select an original Dungeon Keeper installation.");
        return;
    }

    // Check if app dir is writable
    QFileInfo appDirFileInfo(QCoreApplication::applicationDirPath());
    if(appDirFileInfo.isWritable() == false){
        QMessageBox::warning(this, "Failed to copy files", "The current directory is not writable.");
    }

    // Copy the files
    QDir *toDir = new QDir(QCoreApplication::applicationDirPath());
    if(!DkFiles::copyDkDirToDir(dkDir, toDir)){
        QMessageBox::warning(this, "Failed to copy files", "Something went wrong while copying the files.");
        return;
    }

    // Success!
    QMessageBox::information(this, "DK files copied!", "The required Dungeon Keeper files have been copied.\n\nYou can now play KeeperFX!");
    this->accept();
}

void CopyDkFilesDialog::on_cancelButton_clicked()
{
    // Call the close() method to trigger the closeEvent
    close();
}

void CopyDkFilesDialog::closeEvent(QCloseEvent *event)
{
    int result = QMessageBox::question(this, "Confirmation", "Are you sure?\n\nKeeperFX will not function correctly without these files.");

    if (result == QMessageBox::Yes) {
        event->accept();  // Allow the dialog to close
    } else {
        event->ignore();  // Ignore the close event
    }
}
