#include "copydkfilesdialog.h"
#include "ui_copydkfilesdialog.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>

#include "dkfiles.h"
#include "downloadmusicdialog.h"

CopyDkFilesDialog::CopyDkFilesDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CopyDkFilesDialog)
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

    // Automatically check for a DK install directory
    auto existingDkInstallDir = DkFiles::findExistingDkInstallDir();
    if (existingDkInstallDir) {
        qDebug() << "Automatically detected DK dir:" << existingDkInstallDir->absolutePath();
        ui->autoFoundLabel->setText(tr("A suitable DK installation has been automatically detected.", "Label"));
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
    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Select Directory containing Dungeon Keeper files", "Directory Browse Dialog"), "", QFileDialog::ShowDirsOnly);

    // Check if a valid directory was selected
    if (DkFiles::isValidDkDirPath(dirPath)) {
        // Set the directory path in the QLineEdit (browseInput)
        ui->browseInput->setText(dirPath);
    } else {
        ui->browseInput->setText("");
        QMessageBox::warning(this,
                             tr("Original DK files not found", "MessageBox Title"),
                             tr("The chosen directory does not contain all of the required files. Please select an original Dungeon Keeper installation.", "MessageBox Text"));
    }

    // Hide the auto found text
    ui->autoFoundLabel->setText("");
}

void CopyDkFilesDialog::on_copyButton_clicked()
{
    // Make sure box is set
    if(ui->browseInput->text().isEmpty()){
        QMessageBox::information(this, tr("Select a directory", "MessageBox Title"), tr("Please select an original Dungeon Keeper installation location.", "MessageBox Text"));
        return;
    }

    QDir dkDir(ui->browseInput->text());

    // Check if given dir is a valid DK dir
    if(!DkFiles::isValidDkDir(dkDir)){
        QMessageBox::warning(this,
                             tr("Original DK files not found", "MessageBox Title"),
                             tr("The chosen directory does not contain all of the required files. Please select an original Dungeon Keeper installation.", "MessageBox Text"));
        return;
    }

    // Check if app dir is writable
    QFileInfo appDirFileInfo(QCoreApplication::applicationDirPath());
    if(appDirFileInfo.isWritable() == false){
        QMessageBox::warning(this, tr("Failed to copy files", "MessageBox Title"), tr("The current directory is not writable.", "MessageBox Text"));
    }

    // Copy the files
    QDir toDir(QCoreApplication::applicationDirPath());
    if(!DkFiles::copyDkDirToDir(dkDir, toDir)){
        QMessageBox::warning(this, tr("Failed to copy files", "MessageBox Title"), tr("Something went wrong while copying the files.", "MessageBox Text"));
        return;
    }

    // Check if music files have been copied
    if (DkFiles::areAllSoundFilesPresent() == false) {
        DownloadMusicDialog downloadMusicDialog(this);
        downloadMusicDialog.exec();
    }

    // Success!
    QMessageBox::information(this,
                             tr("DK files copied!", "MessageBox Title"),
                             tr("The required Dungeon Keeper files have been copied.\n\nYou can now play KeeperFX!", "MessageBox Text"));
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
                                       tr("Confirmation", "MessageBox Title"),
                                       tr("Are you sure?\n\nKeeperFX will not function correctly without these files.", "MessageBox Text"));

    if (result == QMessageBox::Yes) {
        event->accept();  // Allow the dialog to close
    } else {
        event->ignore();  // Ignore the close event
    }
}
