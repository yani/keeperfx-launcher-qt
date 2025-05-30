#pragma once

#include <optional>

#include <QStringList>
#include <QDir>

class DkFiles {

public:

    static QStringList getInstallPaths();
    static QStringList getFilePathCases(QString dir, QString fileName);
    static bool isValidDkDir(QDir dir);
    static bool isValidDkDirPath(QString path);
    static std::optional<QDir> findExistingDkInstallDir();
    static bool copyDkDirToDir(QDir dir, QDir toDir);
    static bool isCurrentAppDirValidDkDir();
    static bool areAllSoundFilesPresent();
    static bool isOriginalDkExecutableFound();

private:

    static const QStringList installPaths;
    static const QStringList wineInstallPaths;

    static const QStringList dataFiles;
    static const QStringList soundFiles;
    static const QStringList musicFiles;
    static const QStringList originalDkExecutableFiles;

    static std::optional<QDir> findSteamDkInstallDir();
};
