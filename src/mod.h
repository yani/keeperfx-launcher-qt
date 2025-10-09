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

private:
    QString identifier;
    QDir directory;
    // Mod information
    QString author;
    QString version;
    QString minimumGameVersion;
    QString name;
    QString nameTranslated;
    QString description;
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
};
