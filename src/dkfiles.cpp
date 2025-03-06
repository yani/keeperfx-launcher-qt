#include "dkfiles.h"

#include <QDir>
#include <QUrl>
#include <QFile>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QOperatingSystemVersion>

#ifdef WIN32
    // Common DK installation paths under Windows
    const QStringList DkFiles::installPaths = {
        "C:\\GOG Games\\Dungeon Keeper Gold",
        "C:\\Program Files (x86)\\GOG Galaxy\\Games\\Dungeon Keeper Gold",
        "C:\\Program Files (x86)\\Origin Games\\Dungeon Keeper\\Data",
        "C:\\Program Files (x86)\\Origin Games\\Dungeon Keeper\\DATA",
        "C:\\Program Files (x86)\\Origin Games\\Dungeon Keeper",
        "Z:\\home\\<user>\\.steam\\steam\\steamapps\\common\\Dungeon Keeper", // When running under Wine
    };
#else
    // Common DK installation paths under UNIX
    const QStringList DkFiles::installPaths = {
        "<userhome>/.wine/drive_c/GOG Games/Dungeon Keeper Gold",
        "<userhome>/.wine/drive_c/Program Files (x86)/GOG Galaxy/Games/Dungeon Keeper Gold",
        "<userhome>/.wine/drive_c/Program Files (x86)/Origin Games/Dungeon Keeper/Data",
        "<userhome>/.wine/drive_c/Program Files (x86)/Origin Games/Dungeon Keeper/DATA",
        "<userhome>/.wine/drive_c/Program Files (x86)/Origin Games/Dungeon Keeper",
        "<userhome>/.steam/steam/steamapps/common/Dungeon Keeper",
        "<userhome>/Games/dungeon-keeper/drive_c/KeeperFX", // A Common Lutris location
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
    QStringList paths;

    // Loop through the paths and replace some vars
    for (const QString &installPath : installPaths) {

        // Load path
        QString path = installPath;

        // Replace some default vars
        path.replace("<user>", qgetenv("USER"));
        path.replace("<userhome>", QDir::homePath());

        // Add to list
        paths.append(path);
    }

    return paths;
}

QStringList DkFiles::getFilePathCases(QString dir, QString fileName)
{
    QStringList(returnStrings);

    #ifdef WIN32
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

QDir DkFiles::findExistingDkInstallDir()
{
    for (const QString& path : getInstallPaths())
    {
        QDir dir(path);

        if(dir.exists() && isValidDkDir(dir)){
            return dir;
        }
    }

    return QDir();
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
