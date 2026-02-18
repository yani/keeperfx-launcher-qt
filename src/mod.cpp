#include "mod.h"

#include "kfxversion.h"

#include <QCoreApplication>
#include <QSettings>

Mod::Mod(const QDir directory)
{
    this->directory = directory;
    this->identifier = directory.dirName();

    // Check if the dir exists
    if (directory.exists() == false) {
        qWarning() << "Invalid mod directory:" << directory.absolutePath();
        return;
    }

    // Check if the dir is readable
    if (directory.isReadable() == false) {
        qWarning() << "Mod directory not readable:" << directory.absolutePath();
        return;
    }

    // Get mod metadata file
    QFile modMetadataFile(directory.absoluteFilePath("mod.cfg"));
    if (modMetadataFile.exists() == false) {
        qWarning() << "Mod directory does not have 'mod.cfg' metadata file:" << directory.absolutePath();
        return;
    }

    // Load metadata file
    QSettings *modMetadata = new QSettings(modMetadataFile.fileName(), QSettings::IniFormat);
    if (modMetadata->status() != QSettings::NoError) {
        qWarning() << "Failed to load mod metadata file. Invalid ini format:" << modMetadataFile.fileName();
        return;
    }

    // Load info [mod]
    this->name = modMetadata->value("mod/Name").toString();
    this->author = modMetadata->value("mod/Author").toString();
    this->description = modMetadata->value("mod/Description").toString();
    this->version = modMetadata->value("mod/Version").toString();
    this->minimumGameVersion = modMetadata->value("mod/MinimumGameVersion").toString();

    // Load translated info [mod]
    // TODO
    QString nameTranslated;
    QString descriptionTranslated;

    // Load dates [mod]
    // TODO
    QDate createdDate;
    QDate lastUpdatedDate;

    // Load thumbnail [mod]
    this->thumbnailFilename = modMetadata->value("mod/Thumbnail").toString();
    if (this->thumbnailFilename.isEmpty() == false) {
        // Get thumbnail filepath
        QString thumbnailFilepath = this->directory.absoluteFilePath(this->thumbnailFilename);

        // Get thumbnail file
        QFile thumbnailFile(thumbnailFilepath);
        if (thumbnailFile.exists()) {
            if (this->thumbnailPixmap.load(thumbnailFilepath) != true) {
                qWarning() << "Failed to load mod thumbnail:" << thumbnailFilepath;
            }
        } else {
            qWarning() << "Mod thumbnail does not exist:" << thumbnailFilepath;
        }
    }

    // Load website information [webinfo]
    this->kfxNetAuthorUsername = modMetadata->value("web/KfxNetAuthorUsername").toString();
    this->kfxNetWorkshopItemId = modMetadata->value("web/KfxNetWorkshopItemId").toString();

    //qDebug() << "Translation test:" << modMetadata->value("mod/Name[DUT]").toString();

    this->valid = true;
}

bool Mod::isGameVersionCompatible()
{
    if (this->minimumGameVersion.isEmpty()) {
        return true;
    }

    return KfxVersion::isVersionHigherOrEqual(KfxVersion::currentVersion.version, this->minimumGameVersion);
}

QString Mod::toString() const
{
    if (this->nameTranslated.isEmpty() == false) {
        return QString(this->nameTranslated + " [" + this->identifier + "]");
    }

    if (this->name.isEmpty() == false) {
        return QString(this->name + " [" + this->identifier + "]");
    }

    return QString("[" + this->identifier + "]");
}

bool Mod::isValid()
{
    return this->valid;
}
