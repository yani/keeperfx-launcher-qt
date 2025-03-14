#include "savefile.h"

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
    for (const QString &saveFileFilename : saveFiles) {
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
