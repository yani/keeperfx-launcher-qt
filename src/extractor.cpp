#include "extractor.h"
#include "archiver.h"

#include <QFileInfo>
#include <QCoreApplication>
#include <QJsonObject>
#include <QDebug>
#include <QThread>

#include <bit7z/bitextractor.hpp>
#include <bit7z/bitabstractarchivehandler.hpp>
#include <bit7z/bitarchivereader.hpp>

Extractor::Extractor(QObject *parent)
    : QObject(parent)
{
}

void Extractor::extract(QFile *archiveFile, QString outputDir)
{
    QThread::create([this, archiveFile, outputDir]() {

        try {
            // Get file info for the archive file
            QFileInfo archiveFileInfo(archiveFile->fileName());

            // Get archive reader
            bit7z::BitArchiveReader archive(Archiver::getReader(
                archiveFileInfo.absoluteFilePath().toStdString()
                ));

            // Set progress callback
            archive.setProgressCallback([this](uint64_t processedSize) -> bool {
                emit progress(processedSize);
                return true; // Continue processing
            });

            // Extract it
            archive.extractTo(outputDir.toStdString());

            emit extractComplete();

        } catch (const bit7z::BitException &ex) {

            qWarning() << "bit7z BitException:" << ex.what();
            emit extractFailed(QString::fromStdString(ex.what()));
        }

    })->start();
}
