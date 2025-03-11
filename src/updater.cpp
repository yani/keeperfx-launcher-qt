#include "updater.h"
#include "archiver.h"

#include <QFileInfo>
#include <QCoreApplication>
#include <QJsonObject>
#include <QDebug>
#include <QThread>

#include <bit7z/bitextractor.hpp>
#include <bit7z/bitabstractarchivehandler.hpp>
#include <bit7z/bitarchivereader.hpp>

Updater::Updater(QObject *parent)
    : QObject(parent)
{
}

void Updater::updateFromArchive(QFile *archiveFile)
{
    QThread::create([this, archiveFile]() {

        try {
            // Get file info for the archive file
            QFileInfo archiveFileInfo(archiveFile->fileName());

            // Get archive reader
            bit7z::BitArchiveReader archive(Archiver::getReader(
                archiveFileInfo.absoluteFilePath().toStdString()
                ));

            // Destination folder for extraction
            std::string outputDir = QCoreApplication::applicationDirPath().toStdString();

            // Set progress callback
            archive.setProgressCallback([this](uint64_t processedSize) -> bool {
                emit progress(processedSize);
                return true; // Continue processing
            });

            // Extract it
            archive.extractTo(outputDir);

            emit updateComplete();

        } catch (const bit7z::BitException &ex) {

            qWarning() << "bit7z BitException:" << ex.what();
            emit updateFailed(QString::fromStdString(ex.what()));
        }

    })->start();
}
