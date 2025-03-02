#pragma once

#include <optional>

#include <QFile>
#include <QString>

#include <bit7z/bitextractor.hpp>
#include <bit7z/bitabstractarchivehandler.hpp>
#include <bit7z/bitarchivereader.hpp>

class Updater
{

public:
    static uint64_t testArchiveAndGetSize(QFile *archiveFile);
    static bool updateFromArchive(QFile *archiveFile,
                                  std::function<bool(uint64_t processed_size)> progressCallback);
    static bool updateFromGameFileMap(QMap<QString, QString> fileMap,
                                  std::function<bool(uint64_t processed_size)> progressCallback);

private:
    static bool is64BitDLL(const std::string& dllPath);
    static std::optional<bit7z::Bit7zLibrary> lib;
    static void loadBit7zLib();

};
