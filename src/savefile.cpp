#include "savefile.h"

#include "archiver.h"
#include "kfxversion.h"

#include <QApplication>
#include <QDir>

// Constructor
SaveFile::SaveFile(const QString &filePath)
{
    // Set the path of the savefile
    file.setFileName(filePath);

    // Make sure the save can be loaded
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        qWarning() << "Savefile can not be opened:" << filePath;
        return;
    }

    // Get the filename of the file
    fileName = QFileInfo(file).fileName();

    try {

        // Check the header of the savefile
        if (!checkFileHeader(file)) {
            file.close();
            return;
        }

        // Read save name
        // You can use any hex editor to figure out the starting pos
        // We force the loop to end at the end pos, but we should always return earlier
        qint64 pos = 0x12;
        while (pos <= 0x20) {
            file.seek(pos);
            QByteArray buff = file.read(1);
            if (buff.isEmpty() || buff.at(0) == '\0') {
                break;
            }
            saveName += buff;
            pos++;
        }

        // Read campaign/ruleset name
        pos = 0x25;
        while (pos < 0xC4) {
            file.seek(pos);
            QByteArray buff = file.read(1);
            if (buff.isEmpty() || buff.at(0) == '\0') {
                break;
            }
            campaignName += buff;
            pos++;
        }

        qDebug() << "Savefile object created:" << toString();

    } catch (QException& ex) {
        qWarning() << "Savefile exception: " << ex.what();
    }

    // Make sure the file handle is closed again
    file.close();
}

bool SaveFile::isValid() {
    return !saveName.isEmpty() && !campaignName.isEmpty();
}

QString SaveFile::toString() {
    // return fileName + ": " + saveName + " (" + campaignName + ")";
    return saveName + " (" + campaignName + ")";
}

QList<SaveFile *> SaveFile::getAll()
{
    QList<SaveFile *> list;

    // Check if the save file dir exists
    QDir saveFileDir(QApplication::applicationDirPath() + "/save");
    if (saveFileDir.exists() == false) {
        return list; // Empty list
    }

    // Create the filename filter
    QStringList saveFileFilter;
    saveFileFilter << "fx1g*.sav";

    // Get savefile paths
    QStringList saveFiles = saveFileDir.entryList(saveFileFilter, QDir::Files);
    if (saveFiles.isEmpty()) {
        return list; // Empty list
    }

    // Loop trough all files
    for (const QString &saveFileFilename : std::as_const(saveFiles)) {
        // Try to load this file as a SaveFile
        SaveFile *saveFile = new SaveFile(saveFileDir.absoluteFilePath(saveFileFilename));

        if (saveFile->isValid()) {
            list << saveFile;
        }
    }

    return list;
}

bool SaveFile::checkFileHeader(QFile& file) {
    file.seek(0x4);
    QByteArray header = file.read(4);
    return (header == "INFO");
}

bool SaveFile::backupAll()
{
    return SaveFile::backupAll(SaveFile::getAll());
}

bool SaveFile::backupAll(QList<SaveFile *> saveFiles)
{
    // Check if there are savefiles to backup
    if (saveFiles.length() == 0) {
        qDebug() << "No save files found to backup";
        return true;
    }

    // Get the backup dir and make it if it does not exist yet
    QString backupDirPath = QCoreApplication::applicationDirPath() + "/save/backup";
    QDir dir;
    if (!dir.exists(backupDirPath)) {
        dir.mkpath(backupDirPath);
    }

    // Create filename and path
    QString archiveFileName = "keeperfx-save-backup-" + QDate::currentDate().toString("yyyy-MM-dd") + "-v" + KfxVersion::currentVersion.version + ".7z";
    QString archiveFilePath = backupDirPath + "/" + archiveFileName;
    QFile archive(archiveFilePath);

    // Make sure archive output file does not exist
    if (archive.exists()) {
        archive.remove();
    }

    // Debug
    qDebug() << "Save backup archive path:" << archiveFilePath;
    qDebug().noquote() << QString("Archiving %1 save(s)").arg(saveFiles.length()).toStdString();

    // Archive all saves
    for (SaveFile *saveFile : saveFiles) {
        qDebug() << "Archiving:" << saveFile->saveName;
        if (Archiver::compressSingleFile(&saveFile->file, archiveFilePath.toStdString()) == false) {
            return false;
        }
    }

    return true;
}
