#pragma once

#include <QFile>

#include <bit7z/bitextractor.hpp>
#include <bit7z/bitabstractarchivehandler.hpp>
#include <bit7z/bitarchivereader.hpp>

class Updater
{

public:

    static uint64_t testArchiveAndGetSize(QFile *archiveFile);

    static bool updateFromArchive(
        QFile *archiveFile,
        std::function<bool(uint64_t processed_size)> progressCallback
    );

};
