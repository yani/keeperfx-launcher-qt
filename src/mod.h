#pragma once

#include <QDir>
#include <QPixmap>
#include <QString>

class Mod
{
public:
    Mod(const QDir directory);

    bool isGameVersionCompatible();

    QString toString() const;

    // Identifier
    QString identifier;
    QDir directory;

    // Info
    QString name;
    QString author;
    QString description;
    QString version;
    QString minimumGameVersion;

    // Translations
    // TODO: do this
    QString nameTranslated;
    QString descriptionTranslated;

    // Thumbnail
    QString thumbnailFilename;
    QPixmap thumbnailPixmap;

    // Dates
    QDate createdDate;
    QDate lastUpdatedDate;

    // Online information
    QString kfxNetAuthorUsername;
    QString kfxNetWorkshopItemId;

    bool isValid();

private:
    bool valid = false;
};
