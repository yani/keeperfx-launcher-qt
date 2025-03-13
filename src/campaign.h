#pragma once

#include <QFile>
#include <QSettings>
#include <QString>

class Campaign
{
public:
    Campaign(const QString &filePath);

    QFile file;
    QString fileName;
    QString campaignName;
    QString campaignShortName;

    QSettings *settings;

    bool isValid();
    QString toString();

    static QList<Campaign *> getAll();
};
