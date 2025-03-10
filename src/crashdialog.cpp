#include "crashdialog.h"
#include "apiclient.h"
#include "archiver.h"
#include "savefile.h"
#include "ui_crashdialog.h"

CrashDialog::CrashDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CrashDialog)
    , saveFileList(SaveFile::getAll())
{
    ui->setupUi(this);

    if (saveFileList.empty() == true) {
        ui->saveFileComboBox->setDisabled(true);
        ui->saveFileComboBox->setPlaceholderText("No saves found");
    } else {
        ui->saveFileComboBox->addItem("None");
        for (SaveFile *saveFile : saveFileList) {
            ui->saveFileComboBox->addItem(saveFile->toString());
        }
    }
}

CrashDialog::~CrashDialog()
{
    delete ui;
}

void CrashDialog::on_cancelButton_clicked()
{
    this->close();
}

void CrashDialog::on_sendButton_clicked()
{
    // Get the text details from the form
    QString contactDetails = ui->contactInfoLineEdit->text();
    QString infoDetails = ui->infoTextEdit->toPlainText();

    // Variable for possible save file archive
    QFile *saveFileArchive = nullptr;

    // Get savefile
    SaveFile *saveFile = nullptr;
    int saveFileIndex = ui->saveFileComboBox->currentIndex();
    if (saveFileIndex > 0) { // -1 = nothing selected, 0 = "None"
        saveFile = saveFileList.at(saveFileIndex - 1);
    }

    // Compress savefile
    if (saveFile) {
        QString outputPath(QCoreApplication::applicationDirPath() + "/.crashreport-savefile.7z.tmp");
        bool saveFileArchiveResult = Archiver::compressSingleFile(&saveFile->file,
                                                                  outputPath.toStdString());
        if (saveFileArchiveResult) {
            saveFileArchive = new QFile(outputPath);
        } else {
            qDebug() << "Failed to create temporary savefile archive";
        }
    }

    //QJsonDocument jsonDoc = ApiClient::getJsonResponse(QUrl endpointPath, HttpMethod method, const QByteArray &postData)
}
