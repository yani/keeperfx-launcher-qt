#include "archiver.h"
#ifdef WIN32
#include "helper.h" // For 64bit check on lib dll
#endif

#include <QCoreApplication>
#include <QFileInfo>

#include <bit7z/bitextractor.hpp>
#include <bit7z/bitabstractarchivehandler.hpp>
#include <bit7z/bitarchivereader.hpp>

std::optional<bit7z::Bit7zLibrary> Archiver::lib;

// Initialize the library if it's not already loaded
void Archiver::loadBit7zLib()
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

        if (!Helper::is64BitDLL(libPath)) {
            qWarning() << "Not a 64 bit dll:" << libPath;
        }
#else
        bit7z::tstring libPath = BIT7Z_STRING(QCoreApplication::applicationDirPath().toStdString()
                                              + "/7z.so");
#endif
        qDebug() << "7z lib path:" << libPath;
        lib.emplace(libPath); // Initialize the static library
    }
}

bit7z::BitArchiveReader Archiver::getReader(std::string filePath)
{
    // Make sure library is loaded
    Archiver::loadBit7zLib();
    if (!Archiver::lib) {
        throw std::runtime_error("Failed to load bit7z library");
    }

    // Create the reader
    // For now only 7z because we use .tmp file extension
    return bit7z::BitArchiveReader{*lib, filePath, bit7z::BitFormat::SevenZip};
}

uint64_t Archiver::testArchiveAndGetSize(QFile *archiveFile)
{
    // Get file info for the archive file
    QFileInfo archiveFileInfo(archiveFile->filesystemFileName());

    // Get archive reader
    bit7z::BitArchiveReader archive = Archiver::getReader(
        archiveFileInfo.absoluteFilePath().toStdString()
    );

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
