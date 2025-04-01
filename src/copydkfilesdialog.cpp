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

    // Disable resizing and remove maximize button
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    // Disable resizing and remove maximize button
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(size()); // Prevent resizing by setting fixed size

    // Automatically check for a DK install directory
    auto existingDkInstallDir = DkFiles::findExistingDkInstallDir();
    if (existingDkInstallDir) {
        qDebug() << "Automatically detected DK dir:" << existingDkInstallDir->absolutePath();
        ui->autoFoundLabel->setText(tr("A suitable DK installation has been automatically detected."));
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
    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Select Directory containing Dungeon Keeper files"), "", QFileDialog::ShowDirsOnly);

    // Check if a valid directory was selected
    if (DkFiles::isValidDkDirPath(dirPath)) {
        // Set the directory path in the QLineEdit (browseInput)
        ui->browseInput->setText(dirPath);
    } else {
        ui->browseInput->setText("");
        QMessageBox::warning(this,
            tr("Original DK files not found"),
            tr("The chosen directory does not contain all of the required files. Please select an original Dungeon Keeper installation.")
        );
    }

    // Hide the auto found text
    ui->autoFoundLabel->setText("");
}

void CopyDkFilesDialog::on_copyButton_clicked()
{
    // Make sure box is set
    if(ui->browseInput->text().isEmpty()){
        QMessageBox::information(this,
            tr("Select a directory"),
            tr("Please select an original Dungeon Keeper installation location.")
        );
        return;
    }

    QDir dkDir(ui->browseInput->text());

    // Check if given dir is a valid DK dir
    if(!DkFiles::isValidDkDir(dkDir)){
        QMessageBox::warning(this,
            tr("Original DK files not found"),
            tr("The chosen directory does not contain all of the required files. Please select an original Dungeon Keeper installation.")
        );
        return;
    }

    // Check if app dir is writable
    QFileInfo appDirFileInfo(QCoreApplication::applicationDirPath());
    if(appDirFileInfo.isWritable() == false){
        QMessageBox::warning(this,
            tr("Failed to copy files"),
            tr("The current directory is not writable.")
        );
    }

    // Copy the files
    QDir toDir(QCoreApplication::applicationDirPath());
    if(!DkFiles::copyDkDirToDir(dkDir, toDir)){
        QMessageBox::warning(this,
            tr("Failed to copy files"),
            tr("Something went wrong while copying the files.")
        );
        return;
    }

    // Success!
    QMessageBox::information(this,
        tr("DK files copied!"),
        tr("The required Dungeon Keeper files have been copied.\n\nYou can now play KeeperFX!")
    );
    this->accept();
}

void CopyDkFilesDialog::on_cancelButton_clicked()
{
    // Call the close() method to trigger the closeEvent
    close();
}

void CopyDkFilesDialog::closeEvent(QCloseEvent *event)
{
    int result = QMessageBox::question(this,
        tr("Confirmation"),
        tr("Are you sure?") + "\n\n" + tr("KeeperFX will not function correctly without these files.")
    );

    if (result == QMessageBox::Yes) {
        event->accept();  // Allow the dialog to close
    } else {
        event->ignore();  // Ignore the close event
    }
}
