#pragma once

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDebug>
#include <QByteArray>
#include <QException>

class SaveFile
{
public:
    SaveFile(const QString& filePath);

    QFile file;
    QString fileName;
    QString saveName;
    QString campaignName;

    bool isValid();

    QString toString();

private:

    bool checkFileHeader(QFile& file);
};
