#pragma once

#include <QFile>
#include <QString>

class Updater
{

public:
    static bool updateFromArchive(QFile *archiveFile,
                                  std::function<bool(uint64_t processed_size)> progressCallback);
    static bool updateFromGameFileMap(QMap<QString, QString> fileMap,
                                  std::function<bool(uint64_t processed_size)> progressCallback);

};
