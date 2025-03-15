#include "map.h"

#include "settingscfgformat.h"

#include <QCoreApplication>
#include <QDir>
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
            break;
        }

        // Check for LIF file
        if (mapFileNameStringLowerCase.endsWith("lif")) {
            QFile mapFile(campaignOrMapPackDirString + "/" + mapFileNameString);
            loadLif(mapFile);
            break;
        }
    }
}

void Map::loadLof(QFile &file)
{
    // LOF = KFX format

    // Load the file
    QSettings *lofFile = new QSettings(file.fileName(), SettingsCfgFormat::registerFormat());

    // Load the map name
    QString mapNameString = lofFile->value("NAME_TEXT", QString()).toString();
    if (mapNameString.isEmpty() == true) {
        qWarning() << "Failed to load 'NAME_TEXT' from LOF file:" << file.fileName();
        return;
    }

    // Load map
    this->mapName = mapNameString;
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
        this->mapName = data.mid(data.indexOf(";") + 1);
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

QString Map::getMapName()
{
    return this->mapName;
}

Map::Format Map::getFormat()
{
    return this->format;
}

bool Map::isValid()
{
    return this->mapName.isEmpty() == false;
}
