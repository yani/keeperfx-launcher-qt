#include "map.h"

#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QSettings>
#include <QString>

// Constructor
Map::Map(const Map::Type type, const QString campaignOrMapPackName, const int mapNumber)
{
    this->type = type;
    this->campaignOrMapPackName = campaignOrMapPackName;
    this->mapNumber = mapNumber;
    this->format = Map::Format::UNKNOWN;

    // Get base directory
    QString baseDirString = QCoreApplication::applicationDirPath() + "/";
    if (type == Map::Type::CAMPAIGN) {
        baseDirString.append("campgns");
    } else if (type == Map::Type::STANDALONE) {
        baseDirString.append("levels");
    } else {
        qWarning() << "Map type not implemented";
        return;
    }

    // Make sure campaign/levels base directory exists
    QDir baseDir(baseDirString);
    if (baseDir.exists() == false) {
        qWarning() << "Map directory does not exist:" << baseDirString;
        return;
    }

    // Get specific campaign/levels dir
    QString campaignOrMapPackDirString = baseDirString + "/" + campaignOrMapPackName;
    QDir campaignOrMapPackDir(campaignOrMapPackDirString);
    if (campaignOrMapPackDir.exists() == false) {
        qWarning() << "Campaign or map pack does not exist:" << campaignOrMapPackDirString;
        return;
    }

    // Remember map base dir
    this->mapDirString = campaignOrMapPackDirString;

    // Map number string
    // Creates a 5 digit number string of the number
    // Ex: 123 -> "00123", 12345 -> "12345"
    QString mapNumberString = QString("%1").arg(this->mapNumber, 5, 10, QChar('0'));

    // Create the filename filter
    QStringList mapFileFilter;
    mapFileFilter << QString("*" + mapNumberString + ".*");

    // Loop trough map files
    for (const QString mapFileNameString : campaignOrMapPackDir.entryList(mapFileFilter, QDir::Files)) {
        QString mapFileNameStringLowerCase = mapFileNameString.toLower();

        // Check for LOF file
        if (mapFileNameStringLowerCase.endsWith("lof")) {
            QFile mapFile(campaignOrMapPackDirString + "/" + mapFileNameString);
            loadLof(mapFile);
            return;
        }

        // Check for LIF file
        if (mapFileNameStringLowerCase.endsWith("lif")) {
            QFile mapFile(campaignOrMapPackDirString + "/" + mapFileNameString);
            loadLif(mapFile);
            return;
        }
    }

    // Loop trough map files again for .txt level name workaround
    for (const QString mapFileNameString : campaignOrMapPackDir.entryList(mapFileFilter, QDir::Files)) {
        QString mapFileNameStringLowerCase = mapFileNameString.toLower();

        // Check for LOF file
        if (mapFileNameStringLowerCase.endsWith("txt")) {
            QFile mapFile(campaignOrMapPackDirString + "/" + mapFileNameString);
            loadTxtWorkaround(mapFile);
            return;
        }
    }
}

void Map::loadLof(QFile &file)
{
    // LOF = KFX format

    // Make sure lof file is opened
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open:" << file.fileName();
        return;
    }

    // Read file
    QString fileDataString = file.readAll();
    file.close();

    // Find map name in LOF file
    // We use a regex instead of QSettings here for performance
    // and because QSettings does not always work. (No idea why)
    QRegularExpression regex(R"(^NAME_TEXT\s*=\s*([^\r\n]+))", QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = regex.match(fileDataString);

    // Make sure map name is found
    if (match.hasMatch() == false) {
        qWarning() << "Failed to load 'NAME_TEXT' from LOF file:" << file.fileName();
        return;
    }

    // Load map into class
    this->mapName = match.captured(1);
    this->format = Map::Format::DK;
}

void Map::loadLif(QFile &file)
{
    // LIF = DK format

    // Make sure file can be opened
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open LIF file:" << file.fileName();
        return;
    }

    // Read the file
    QString data = file.readAll(); // Read entire file
    file.close();

    // Make sure some data is read
    if (data.isEmpty() == true) {
        return;
    }

    // Get mapname from a format that a translation ID
    if (data.contains("\n") && data.contains(";")) {
        this->mapName = data.mid(data.indexOf(";") + 1).split("\r")[0].split("\n")[0];
        this->format = Map::Format::DK;
        return;
    }

    // Get map name from the first line
    // We do some string splits that are crossplatform
    QString firstLineMapName = data.split("\r")[0].split("\n")[0].split(", ")[1];
    if (firstLineMapName.isEmpty() == false) {
        this->mapName = firstLineMapName;
        this->format = Map::Format::DK;
        return;
    }

    // Unknown format
    // If this happens it should be fixed
    qWarning() << "Unknown .lif format";
}

void Map::loadTxtWorkaround(QFile &file)
{
    // .txt = Map script
    // We use this method only to load some default levels that do not have lif or lof files

    // Make sure lof file is opened
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open:" << file.fileName();
        return;
    }

    // Read file
    QString fileDataString = file.readAll();
    file.close();

    // Find map name in LOF file
    // We use a regex instead of QSettings here for performance
    // and because QSettings does not always work. (No idea why)
    QRegularExpression regex(R"(^REM  Script for (?:Level )?([^\r\n]+))", QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = regex.match(fileDataString);

    // Check if map name is found
    if (match.hasMatch() == true) {
        this->mapName = match.captured(1);
        this->format = Map::Format::DK;
        return;
    }

    qWarning() << "Failed to load 'Script for Level' from map script:" << file.fileName();
}

QString Map::getMapName()
{
    return this->mapName;
}

Map::Format Map::getFormat()
{
    return this->format;
}

int Map::getMapNumber()
{
    return this->mapNumber;
}

bool Map::isValid()
{
    return this->mapName.isEmpty() == false;
}

QList<Map *> Map::getAll(const Map::Type type, const QString campaignOrMapPackName)
{
    // List to return
    QList<Map *> list;

    // Get base directory
    QString baseDirString = QCoreApplication::applicationDirPath() + "/";
    if (type == Map::Type::CAMPAIGN) {
        baseDirString.append("campgns");
    } else if (type == Map::Type::STANDALONE) {
        baseDirString.append("levels");
    } else {
        qWarning() << "Map type not implemented";
        return list; // Empty list
    }

    // Make sure campaign/levels base directory exists
    QDir baseDir(baseDirString);
    if (baseDir.exists() == false) {
        qWarning() << "Map directory does not exist:" << baseDirString;
        return list; // Empty list
    }

    // Get specific campaign/levels dir
    QString campaignOrMapPackDirString = baseDirString + "/" + campaignOrMapPackName;
    QDir campaignOrMapPackDir(campaignOrMapPackDirString);
    if (campaignOrMapPackDir.exists() == false) {
        qWarning() << "Campaign or map pack does not exist:" << campaignOrMapPackDirString;
        return list; // Empty list
    }

    // Loop trough the files
    for (const QString fileNameString : campaignOrMapPackDir.entryList(QDir::Filter::Files)) {
        QString fileNameStringLowerCase = fileNameString.toLower();

        // Check if this is a mapXXXXX.dat file
        // We first check for .dat to decrease the amount of checking
        if (fileNameStringLowerCase.endsWith(".dat") == false || fileNameStringLowerCase.startsWith("map") == false) {
            continue;
        }

        // Get mapnumber
        QString mapNumberString = fileNameStringLowerCase.mid(3, 5);
        int mapNumber = mapNumberString.toInt();

        // Try to load this map
        Map *map = new Map(type, campaignOrMapPackName, mapNumber);
        if (map->isValid() == false) {
            qWarning() << "Map could not be loaded:" << campaignOrMapPackName << "->" << mapNumber;
            continue;
        }

        // Add map to list
        list << map;
        qDebug() << "Map loaded:" << map->toString();
    }

    return list;
}

QString Map::toString()
{
    if (this->isValid() == false) {
        return QString("Invalid map");
    }

    QString typeString;
    if (type == Map::Type::CAMPAIGN) {
        typeString = "campgns";
    } else if (type == Map::Type::STANDALONE) {
        typeString = "levels";
    }

    QString mapNumberString = QString("%1").arg(this->mapNumber, 5, 10, QChar('0'));

    return QString(this->mapName + " (" + typeString + "/" + this->campaignOrMapPackName + "/map" + mapNumberString + ")");
}
