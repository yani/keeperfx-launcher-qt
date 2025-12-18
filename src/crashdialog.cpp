#include "crashdialog.h"
#include "ui_crashdialog.h"

#include "apiclient.h"
#include "archiver.h"
#include "gzip.h"
#include "savefile.h"
#include "version.h"
#include "settings.h"
#include "launcheroptions.h"

#include <QDir>
#include <QMessageBox>

#define COMPRESS_KEEPERFX_LOG_GZIP true

CrashDialog::CrashDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CrashDialog)
    , saveFileList(SaveFile::getAll())
{
    ui->setupUi(this);

    // Disable resizing and remove maximize button
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    // Load save file list
    if (saveFileList.empty() == true) {
        ui->saveFileComboBox->setDisabled(true);
        ui->saveFileComboBox->setPlaceholderText(tr("No saves found", "Menu Placeholder"));
    } else {
        ui->saveFileComboBox->addItem(tr("None", "Default Savefile"));
        for (SaveFile *saveFile : saveFileList) {
            ui->saveFileComboBox->addItem(saveFile->toString());
        }
    }

    // Handle information label
    ui->crashInfoLabel2->setTextFormat(Qt::RichText);
    ui->crashInfoLabel2->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->crashInfoLabel2->setOpenExternalLinks(true);
    ui->crashInfoLabel2->setText(ui->crashInfoLabel2->text().arg(QString("<a href=\"%1\">%1</a>").arg("https://github.com/dkfans/keeperfx/issues")));

    // Load preset contact details
    ui->contactInfoDiscordLineEdit->setText(Settings::getLauncherSetting("CRASH_REPORTING_CONTACT_DISCORD").toString());
    ui->contactInfoKfxNetLineEdit->setText(Settings::getLauncherSetting("CRASH_REPORTING_CONTACT_KEEPERFX_NET").toString());
    ui->contactInfoEmailLineEdit->setText(Settings::getLauncherSetting("CRASH_REPORTING_CONTACT_EMAIL").toString());
}

CrashDialog::~CrashDialog()
{
    delete ui;
}

void CrashDialog::setStdErrorString(QString stdError)
{
    this->stdErrorString = stdError;
}

void CrashDialog::on_cancelButton_clicked()
{
    this->close();
}

void CrashDialog::on_sendButton_clicked()
{
    // Disable the send button
    this->ui->sendButton->setDisabled(true);

    // Create post object
    QJsonObject jsonPostObject;

    // Game and launcher version
    jsonPostObject["game_version"] = KfxVersion::currentVersion.fullString;
    jsonPostObject["source"] = "KfxLauncherQt " + QString(LAUNCHER_VERSION);

    // Description of the crash
    QString infoDetails = ui->infoTextEdit->toPlainText();
    if (infoDetails.isEmpty() == false) {
        jsonPostObject["description"] = infoDetails;
    }

    // Create contact details list
    QStringList contactDetails;

    // Contact details (Discord)
    QString contactDetailsDiscord = ui->contactInfoDiscordLineEdit->text();
    if (contactDetailsDiscord.isEmpty() == false) {
        contactDetails << ("Discord: " + contactDetailsDiscord);
        Settings::setLauncherSetting("CRASH_REPORTING_CONTACT_DISCORD", contactDetailsDiscord);
    } else {
        Settings::setLauncherSetting("CRASH_REPORTING_CONTACT_DISCORD", "");
    }

    // Contact details (KeeperFX.net)
    QString contactDetailsKfxNet = ui->contactInfoKfxNetLineEdit->text();
    if (contactDetailsKfxNet.isEmpty() == false) {
        contactDetails << ("KfxNet: " + contactDetailsKfxNet);
        Settings::setLauncherSetting("CRASH_REPORTING_CONTACT_KEEPERFX_NET", contactDetailsKfxNet);
    } else {
        Settings::setLauncherSetting("CRASH_REPORTING_CONTACT_KEEPERFX_NET", "");
    }

    // Contact details (Email)
    QString contactDetailsEmail = ui->contactInfoEmailLineEdit->text();
    if (contactDetailsEmail.isEmpty() == false) {
        contactDetails << ("Email: " + contactDetailsEmail);
        Settings::setLauncherSetting("CRASH_REPORTING_CONTACT_EMAIL", contactDetailsEmail);
    } else {
        Settings::setLauncherSetting("CRASH_REPORTING_CONTACT_EMAIL", "");
    }

    // Add contact details
    if (contactDetails.isEmpty() == false) {
        jsonPostObject["contact_details"] = contactDetails.join(" - ");
    }

    // keeperfx.cfg
    QFile kfxConfigFile = Settings::getKfxConfigFile();
    if (kfxConfigFile.exists() && kfxConfigFile.open(QIODevice::ReadOnly)) {
        jsonPostObject["game_config"] = QString(kfxConfigFile.readAll());
        kfxConfigFile.close();
    }

    // keeperfx.log
    QFile kfxLogFile(QCoreApplication::applicationDirPath() + "/keeperfx.log");
    if (kfxLogFile.exists() && kfxLogFile.open(QIODevice::ReadOnly)) {

        // Check if logfile is reasonable size (<8MiB)
        if (kfxLogFile.size() > 8 * 1024LL * 1024LL) {
            QMessageBox::warning(this, tr("Crash Report", "MessageBox Title"), tr("Failed to submit crash report.", "MessageBox Text"));
            qWarning() << "Log file too big to be sent with crash report:" << kfxLogFile.fileName() << "Size:" << kfxLogFile.size();
            this->close();
            return;
        }

        // Read logfile
        QByteArray logData = kfxLogFile.readAll();
        kfxLogFile.close();

#if COMPRESS_KEEPERFX_LOG_GZIP

        // Check command line override
        if (LauncherOptions::isSet("disable-gzip-upload") != true) {

            // Compress using gzip
            QByteArray gzipped = GZip::compress(logData);

            if (!gzipped.isEmpty()) {
                jsonPostObject["game_log"] = QString(gzipped.toBase64());
                jsonPostObject["game_log_encoding"] = "gzip+base64";
            } else {
                qDebug() << "GZip::Compress failed, sending plain text log";
                jsonPostObject["game_log"] = QString(logData);
            }

        } else {
            qDebug() << "--disable-gzip set, sending plain text log";
            jsonPostObject["game_log"] = QString(logData);
        }
#else
        // gzip disabled at compile time
        jsonPostObject["game_log"] = QString(logData);
#endif
    }

    // Game std output
    if (this->stdErrorString.isEmpty() == false) {
        jsonPostObject["game_output"] = this->stdErrorString;
    }

    // Savefile
    int saveFileIndex = ui->saveFileComboBox->currentIndex();
    if (saveFileIndex > 0) { // -1 = nothing selected, 0 = "None"

        // Get selected savefile
        SaveFile *saveFile = saveFileList.at(saveFileIndex - 1);
        if (saveFile) {
            // Variables for temporary archive for savefile
            QString tempArchivePath = QDir::temp().filePath("crashreport-savefile.7z.tmp");
            QFile tempArchive(tempArchivePath);

            // Make sure archive output file does not exist
            if (tempArchive.exists()) {
                tempArchive.remove();
            }

            // Compress the save into the archive
            if (Archiver::compressSingleFile(&saveFile->file, tempArchivePath.toStdString())) {
                // Make sure archive exists and can be opened
                if (tempArchive.exists() && tempArchive.open(QIODevice::ReadOnly)) {
                    // Add savefile data to post object
                    jsonPostObject["save_file_name"] = saveFile->fileName + ".7z";
                    jsonPostObject["save_file_data"] = QString(tempArchive.readAll().toBase64());
                    tempArchive.close();
                    tempArchive.remove();
                }
            } else {
                qDebug() << "Failed to create temporary savefile archive";
            }
        }
    }

    // Make request
    QJsonDocument jsonDoc = ApiClient::getJsonResponse(QUrl("v1/crash-report"),
                                                       ApiClient::HttpMethod::POST,
                                                       jsonPostObject);

    // Make sure response is an object
    if (jsonDoc.isObject() == false) {
        this->close();
        return;
    }

    // Get object
    QJsonObject jsonObj = jsonDoc.object();

    // Make sure response was succesful
    bool success = jsonObj["success"].toBool();
    if (!success) {
        QMessageBox::warning(this, tr("Crash Report", "MessageBox Title"), tr("Failed to submit crash report.", "MessageBox Text"));
        qWarning() << "Crash Report API response:" << jsonObj["error"].toString();
        this->close();
        return;
    }

    // Show success and the report ID number
    QMessageBox::information(this,
                             tr("Crash Report", "MessageBox Title"),
                             tr("Your crash report has been successfully submitted!\n\n"
                                "The KeeperFX team can not guarantee immediate results, "
                                "but your feedback is very helpful for the developers working on KeeperFX.\n\n"
                                "Report ID: %1",
                                "MessageBox Text")
                                 .arg(QString::number(jsonObj["id"].toInt())));

    // Accept crash dialog (close it)
    this->accept();
}
