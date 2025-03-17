#include "dkfiles.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QOperatingSystemVersion>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#include "helper.h"

#ifdef Q_OS_WINDOWS
// Common DK installation paths under Windows
const QStringList DkFiles::installPaths = {
    "C:\\GOG Games\\Dungeon Keeper Gold",
    "C:\\Program Files (x86)\\GOG Galaxy\\Games\\Dungeon Keeper Gold",
    "C:\\Program Files (x86)\\Origin Games\\Dungeon Keeper\\Data",
    "C:\\Program Files (x86)\\Origin Games\\Dungeon Keeper\\DATA",
    "C:\\Program Files (x86)\\Origin Games\\Dungeon Keeper",
};

// Common Dk installation paths when running under Wine
// These would map to the UNIX filesystem of the host
const QStringList DkFiles::wineInstallPaths = {
    "Z:\\home\\" + qgetenv("USER") + "\\.steam\\steam\\steamapps\\common\\Dungeon Keeper",
};
#else
// Common DK installation paths under UNIX
const QStringList DkFiles::installPaths = {
    QDir::homePath() + "/.wine/drive_c/GOG Games/Dungeon Keeper Gold",
    QDir::homePath() + "/.wine/drive_c/Program Files (x86)/GOG Galaxy/Games/Dungeon Keeper Gold",
    QDir::homePath() + "/.wine/drive_c/Program Files (x86)/Origin Games/Dungeon Keeper/Data",
    QDir::homePath() + "/.wine/drive_c/Program Files (x86)/Origin Games/Dungeon Keeper/DATA",
    QDir::homePath() + "/.wine/drive_c/Program Files (x86)/Origin Games/Dungeon Keeper",
    QDir::homePath() + "/.steam/steam/steamapps/common/Dungeon Keeper",
    QDir::homePath() + "/Games/dungeon-keeper/drive_c/KeeperFX", // A Common Lutris location
};
#endif

// Files under ./data/
const QStringList DkFiles::dataFiles = {
    "bluepal.dat",
    "bluepall.dat",
    "dogpal.pal",
    "hitpall.dat",
    "lightng.pal",
    "redpal.col",
    "redpall.dat",
    "slab0-0.dat",
    "slab0-1.dat",
    "vampal.pal",
    "whitepal.col",
};

// Files under ./sound/
const QStringList DkFiles::soundFiles = {
    "atmos1.sbk",
    "atmos2.sbk",
    "bullfrog.sbk",
};

const QStringList DkFiles::musicFiles = {
    "keeper02.ogg",
    "keeper03.ogg",
    "keeper04.ogg",
    "keeper05.ogg",
    "keeper06.ogg",
    "keeper07.ogg",
};

QStringList DkFiles::getInstallPaths() {

    // Paths to return
    QStringList paths = installPaths;

#ifdef Q_OS_WINDOWS
    if (Helper::isRunningUnderWine()) {
        paths.append(wineInstallPaths);
    }
#endif

    return paths;
}

QStringList DkFiles::getFilePathCases(QString dir, QString fileName)
{
    QStringList(returnStrings);

#ifdef Q_OS_WINDOWS
    // Windows doesn't care about filepath cases
    returnStrings.append(dir + "/" + fileName);
#else
    // Unix is normal and cares about filepath cases
    returnStrings.append(dir.toLower() + "/" + fileName.toUpper());
    returnStrings.append(dir.toUpper() + "/" + fileName.toUpper());
    returnStrings.append(dir.toLower() + "/" + fileName.toLower());
    returnStrings.append(dir.toUpper() + "/" + fileName.toLower());
#endif

    return returnStrings;
}

bool DkFiles::isValidDkDir(QDir dir)
{
    // Check for DATA files
    for (const QString& dataFileName : dataFiles)
    {
        bool dataFileFound = false;

        for (const QString& dataFilePath : getFilePathCases("data", dataFileName))
        {
            QFile file(dir.absolutePath() + "/" + dataFilePath);
            if(file.exists()){
                dataFileFound = true;
                break;
            }
        }

        if(!dataFileFound){
            return false;
        }
    }

    // Check for SOUND files
    for (const QString& soundFileName : soundFiles)
    {
        bool soundFileFound = false;

        for (const QString& soundFilePath : getFilePathCases("sound", soundFileName))
        {
            QFile file(dir.absolutePath() + "/" + soundFilePath);
            if(file.exists()){
                soundFileFound = true;
                break;
            }
        }

        if(!soundFileFound){
            return false;
        }
    }

    return true;
}

bool DkFiles::isValidDkDirPath(QString path)
{
    QDir dir(path);
    return (dir.exists() && isValidDkDir(dir));
}

std::optional<QDir> DkFiles::findExistingDkInstallDir()
{
    // Search hardcoded paths
    for (const QString& path : getInstallPaths()) {
        QDir dir(path);

        if(dir.exists() && isValidDkDir(dir)){
            return dir;
        }
    }

    // Search for path in a possible Steam installation
    std::optional<QDir> steamInstallDir = findSteamDkInstallDir();
    if (steamInstallDir) {
        return steamInstallDir;
    }

    return std::nullopt;
}

bool DkFiles::copyDkDirToDir(QDir dir, QDir toDir)
{
    // Double check that the given directory is a valid DK dir
    if(!isValidDkDir(dir)){
        return false;
    }

    // Make sure the directories exist in the app dir
    QDir dataDir(toDir.absolutePath() + "/data");
    if (!dataDir.exists()){
        if(!dataDir.mkpath(".")){
            return false;
        }
    }
    QDir soundDir(toDir.absolutePath() + "/sound");
    if (!soundDir.exists()){
        if(!soundDir.mkpath(".")){
            return false;
        }
    }
    QDir musicDir(toDir.absolutePath() + "/music");
    if (!musicDir.exists()){
        if(!musicDir.mkpath(".")){
            return false;
        }
    }

    // Copy DATA files
    for (const QString& dataFileName : dataFiles)
    {
        for (const QString& dataFilePath : getFilePathCases("data", dataFileName))
        {
            QFile file(dir.absolutePath() + "/" + dataFilePath);
            if(file.exists()){
                QString copyDest = dataDir.absolutePath() + "/" + dataFileName.toLower();
                QFile copyDestFile(copyDest);
                if(copyDestFile.exists()){
                    if(copyDestFile.remove() == false){
                        return false;
                    }
                }
                if(file.copy(copyDest) == false) {
                    return false;
                }
                break;
            }
        }
    }

    // Copy SOUND files
    for (const QString& soundFileName : soundFiles)
    {
        for (const QString& soundFilePath : getFilePathCases("sound", soundFileName))
        {
            QFile file(dir.absolutePath() + "/" + soundFilePath);
            if(file.exists()){
                QString copyDest = soundDir.absolutePath() + "/" + soundFileName.toLower();
                QFile copyDestFile(copyDest);
                if(copyDestFile.exists()){
                    if(copyDestFile.remove() == false){
                        return false;
                    }
                }
                if(file.copy(copyDest) == false) {
                    return false;
                }
                break;
            }
        }
    }

    // Copy MUSIC files
    for (const QString& musicFileName : musicFiles)
    {
        // Get the destination file
        QString destFilePath = toDir.absolutePath() + "/music/" + musicFileName.toLower();
        QFile destFile(destFilePath);

        // Remove music file if it already exists
        // We always want a clean install
        if(destFile.exists()){
            if(destFile.remove() == false){
                return false;
            }
        }

        // Lowercase
        // Music files are in the root Digital OG DK dir, and not in "/music"
        QFile musicFileLowercase(dir.absolutePath() + "/" + musicFileName.toLower());
        if(musicFileLowercase.exists()){
            musicFileLowercase.copy(destFilePath);
            continue;
        }

        // Uppercase
        // Music files are in the root Digital OG DK dir, and not in "/music"
        QFile musicFileUppercase(dir.absolutePath() + "/" + musicFileName.toUpper());
        if(musicFileUppercase.exists()){
            musicFileUppercase.copy(destFilePath);
        }
    }

    return true;
}

bool DkFiles::isCurrentAppDirValidDkDir()
{
    QDir dir(QCoreApplication::applicationDirPath());
    return isValidDkDir(dir);
}

std::optional<QDir> DkFiles::findSteamDkInstallDir()
{
    // Search for Steam installation
    qDebug() << "Searching for Steam installation";

#ifdef Q_OS_WINDOWS
    // Open the Steam registry key
    QSettings steamRegistry("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Valve\\Steam", QSettings::NativeFormat);
    QString steamPath = steamRegistry.value("InstallPath").toString();
#else
    QString steamPath = QDir::homePath() + "/.steam/steam";
#endif

    // Check if steam is found
    if (steamPath.isEmpty()) {
        qDebug() << "Steam installation not found";
        return std::nullopt;
    }

    // Log Steam path
    qDebug() << "Steam installation found:" << steamPath;

    // Get the libraryfolders.vdf file for library paths
    QString libraryFoldersFile = steamPath + "/steamapps/libraryfolders.vdf";
    QFile file(libraryFoldersFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open libraryfolders.vdf";
        return std::nullopt;
    }

    QStringList libraryPaths;
    libraryPaths.append(steamPath); // Add the default Steam path

    // Read the libraryfolders.vdg file
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QRegularExpression regex("path\"\\s+\"([^\"]+)\"");
        QRegularExpressionMatch match = regex.match(line);
        if (match.hasMatch() == true) {
            QString libraryPath = match.captured(1);
            qDebug() << "Steam library found:" << libraryPath;
            libraryPaths.append(libraryPath);
        }
    }
    file.close();

    // Check if some paths are found
    if (libraryPaths.empty()) {
        return std::nullopt;
    }

    // Search library locations for DK
    for (const QString& libraryPath : libraryPaths) {
        QString possibleDkInstallPath = libraryPath + "/steamapps/common/Dungeon Keeper";
        QDir dir(possibleDkInstallPath);

        // Check if its a valid dir
        if (dir.exists() && isValidDkDir(dir)) {
            return dir;
        }
    }

    return std::nullopt;
}
