#pragma once

#include <optional>

#include <QFile>

#include <bit7z/bitextractor.hpp>
#include <bit7z/bitabstractarchivehandler.hpp>
#include <bit7z/bitarchivereader.hpp>

class Archiver
{
public:

    static bit7z::BitArchiveReader getReader(std::string filePath);
    static uint64_t testArchiveAndGetSize(QFile *archiveFile);

private:

    static std::optional<bit7z::Bit7zLibrary> lib;
    static void loadBit7zLib();

};
