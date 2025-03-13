#include "campaign.h"

#include <QApplication>
#include <QDir>
#include <QSettings>

Campaign::Campaign(const QString &filePath)
{
    // Set the path of the campaign file
    file.setFileName(filePath);

    // Make sure the campaign can be loaded
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open campaign file:" << filePath;
        return;
    }

    // Load the campaign file variables
    QFileInfo fileInfo(file);
    fileName = fileInfo.fileName();
    campaignShortName = fileInfo.baseName();

    // Load the campaign settings file
    settings = new QSettings(fileInfo.absoluteFilePath(), QSettings::IniFormat);
    if (settings->status() != QSettings::NoError) {
        qWarning() << "Failed to load campaign:" << fileName;
        return;
    }

    campaignName = settings->value("common/NAME").toString();
}

bool Campaign::isValid()
{
    return !campaignName.isEmpty();
}

QString Campaign::toString()
{
    // return fileName + ": " + saveName + " (" + campaignName + ")";
    return campaignName + " (" + campaignShortName + ")";
}

QList<Campaign *> Campaign::getAll()
{
    QList<Campaign *> list;

    // Check if the save file dir exists
    QDir campaignFileDir(QApplication::applicationDirPath() + "/campgns");
    if (campaignFileDir.exists() == false) {
        return list; // Empty list
    }

    // Create the filename filter
    QStringList campaignFileFilter;
    campaignFileFilter << "*.cfg";

    // Get campaign file paths
    QStringList campaignFiles = campaignFileDir.entryList(campaignFileFilter, QDir::Files);
    if (campaignFiles.isEmpty()) {
        return list; // Empty list
    }

    // Loop trough all files
    for (const QString &campaignFilename : campaignFiles) {
        // Try to load this file as a campaign
        Campaign *campaignFile = new Campaign(campaignFileDir.absoluteFilePath(campaignFilename));

        if (campaignFile->isValid()) {
            list << campaignFile;
        }

        qDebug() << "Campaign:" << campaignFile->toString();
    }

    return list;
}
