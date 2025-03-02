#include "updater.h"
#include "apiclient.h"
#include "kfxversion.h"

#include <QFile>
#include <QFileInfo>
#include <QTextEdit>
#include <QProgressBar>
#include <QCoreApplication>
#include <QJsonObject>

#include <LIEF/PE.hpp>

#include <bit7z/bitextractor.hpp>
#include <bit7z/bitabstractarchivehandler.hpp>
#include <bit7z/bitarchivereader.hpp>

std::optional<bit7z::Bit7zLibrary> Updater::lib;

bool Updater::is64BitDLL(const std::string &dllPath)
{
    try {
        auto pe = LIEF::PE::Parser::parse(dllPath);
        return LIEF::PE::Header::x86_64(pe->header().machine());
    } catch (const std::exception &e) {
        qWarning() << "LIEF error: " << e.what();
        return false; // Assume 32-bit or invalid file if parsing fails
    }
}

// Initialize the library if it's not already loaded
void Updater::loadBit7zLib()
{
    if (!lib) { // Only load if it's not already initialized
#ifdef WIN32
        bit7z::tstring libPath;
        if (QFile(QCoreApplication::applicationDirPath() + "/7za.dll").exists()) {
            libPath = BIT7Z_STRING(QCoreApplication::applicationDirPath().toStdString()
                                   + "/7za.dll");
        } else if (QFile(QCoreApplication::applicationDirPath() + "/7z.dll").exists()) {
            libPath = BIT7Z_STRING(QCoreApplication::applicationDirPath().toStdString() + "/7z.dll");
        } else {
            qWarning() << "Failed to find 7zip lib to load";
            return;
        }

        if (!is64BitDLL(libPath)) {
            qWarning() << "Not a 64 bit dll:" << libPath;
        }
#else
        bit7z::tstring libPath = BIT7Z_STRING(QCoreApplication::applicationDirPath().toStdString()
                                              + "/7z.so");
#endif
        qDebug() << "7z lib path:" << libPath;
        lib.emplace(libPath);  // Initialize the static library
    }
}

uint64_t Updater::testArchiveAndGetSize(QFile *archiveFile)
{
    // Get file info for the archive file
    QFileInfo archiveFileInfo(archiveFile->filesystemFileName());

    // Make sure library is loaded
    loadBit7zLib();
    if (!lib) {
        return -1;
    }

    // Get a reader for this archive
    bit7z::BitArchiveReader archive{
        *lib,
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

        // Make sure library is loaded
        loadBit7zLib();
        if (!lib) {
            return -1;
        }

        // Get a reader for this archive
        bit7z::BitArchiveReader archive{
            *lib,
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

bool updateFromGameFileMap(QMap<QString, QString> fileMap,
                           std::function<bool(uint64_t processed_size)> progressCallback)
{
    return true;
}
