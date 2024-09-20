#include "updater.h"

#include <QFile>
#include <QFileInfo>
#include <QTextEdit>
#include <QProgressBar>
#include <QCoreApplication>

#include <bit7z/bitextractor.hpp>
#include <bit7z/bitabstractarchivehandler.hpp>
#include <bit7z/bitarchivereader.hpp>

uint64_t Updater::testArchiveAndGetSize(QFile *archiveFile)
{
    // Get file info for the archive file
    QFileInfo archiveFileInfo(archiveFile->filesystemFileName());

    // Load the library
#ifdef WIN32
    bit7z::Bit7zLibrary lib{ "7z.dll" };
#else
    bit7z::Bit7zLibrary lib{ "7z.so" };
#endif

    // Get a reader for this archive
    bit7z::BitArchiveReader archive{
        lib,
        archiveFileInfo.absoluteFilePath().toStdString(),
        bit7z::BitFormat::SevenZip
    };

    try{

        // Test the archive
        // Throws a BitException when it is invalid
        archive.test();

        // Return the total size of the uncompressed files
        return archive.size();

    } catch (const bit7z::BitException& ex) {

        qWarning() << "Archive test failure:" << ex.what();
        return -1;
    }
}

bool Updater::updateFromArchive(
    QFile *archiveFile,
    std::function<bool(uint64_t processed_size)> progressCallback
) {
    try {

        // Get file info for the archive file
        QFileInfo archiveFileInfo(archiveFile->filesystemFileName());

        // Load the library
        #ifdef WIN32
                bit7z::Bit7zLibrary lib{ "7z.dll" };
        #else
                bit7z::Bit7zLibrary lib{ "7z.so" };
        #endif

        // Get a reader for this archive
        bit7z::BitArchiveReader archive{
            lib,
            archiveFileInfo.absoluteFilePath().toStdString(),
            bit7z::BitFormat::SevenZip
        };

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
