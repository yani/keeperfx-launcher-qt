#pragma once

#include <optional>

#include <QFile>

#include <bit7z/bitextractor.hpp>
#include <bit7z/bitabstractarchivehandler.hpp>
#include <bit7z/bitarchivereader.hpp>
#include <bit7z/bitfilecompressor.hpp>
#include <bit7z/bitfileextractor.hpp>

class Archiver
{
public:
    static bit7z::BitArchiveReader getReader(std::string filePath);
    static bit7z::BitFileExtractor getExtractor();
    static bit7z::BitFileCompressor getCompressor();

    static bool compressSingleFile(QFile *inputFile, std::string outputPath);

    static uint64_t testArchiveAndGetSize(QFile *archiveFile);

private:

    static std::optional<bit7z::Bit7zLibrary> lib;
    static void loadBit7zLib();

};
