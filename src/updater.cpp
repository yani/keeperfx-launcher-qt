#include "updater.h"
#include "archiver.h"

#include <QFile>
#include <QFileInfo>
#include <QTextEdit>
#include <QProgressBar>
#include <QCoreApplication>
#include <QJsonObject>

#include <bit7z/bitextractor.hpp>
#include <bit7z/bitabstractarchivehandler.hpp>
#include <bit7z/bitarchivereader.hpp>

bool Updater::updateFromArchive(
    QFile *archiveFile,
    std::function<bool(uint64_t processed_size)> progressCallback
) {
    try {

        // Get file info for the archive file
        QFileInfo archiveFileInfo(archiveFile->filesystemFileName());

        // Get archive reader
        bit7z::BitArchiveReader archive(Archiver::getReader(
            archiveFileInfo.absoluteFilePath().toStdString()
        ));

        // Destination folder for extraction
        std::string outputDir = QCoreApplication::applicationDirPath().toStdString();

        // Set progress callback
        archive.setProgressCallback(progressCallback);

        // Extract it
        archive.extractTo(outputDir);

    } catch ( const bit7z::BitException& ex ) {

        qWarning() << "bit7z BitException:" << ex.what();
        return false;
    }

    return true;
}
