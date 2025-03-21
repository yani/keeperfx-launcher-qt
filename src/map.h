#pragma once

#include <QFile>
#include <QString>

class Map
{
public:
    enum Type {
        CAMPAIGN,
        STANDALONE,
    };

    enum Format { DK, KFX, UNKNOWN };

    Map(const Map::Type type, const QString campaignOrMapPackName, const int mapNumber);

    bool isValid();
    QString getMapName();
    Map::Format getFormat();
    int getMapNumber();

    static QList<Map *> getAll(const Map::Type type, const QString campaignOrMapPackName);

    QString toString();

private:
    Map::Type type;
    QString campaignOrMapPackName;
    int mapNumber;
    QString mapDirString;

    Map::Format format;
    QString mapName;

    void loadLif(QFile &file);
    void loadLof(QFile &file);
    void loadTxtWorkaround(QFile &file);
};
