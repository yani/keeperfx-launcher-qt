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
    this->fileName = fileInfo.fileName();
    this->campaignShortName = fileInfo.baseName();

    // Load the campaign settings file
    settings = new QSettings(fileInfo.absoluteFilePath(), QSettings::IniFormat);
    if (settings->status() != QSettings::NoError) {
        qWarning() << "Failed to load campaign:" << this->fileName;
        return;
    }

    // Make sure campaign has a name
    QString campaignNameString = settings->value("common/NAME").toString();
    if (campaignNameString.isEmpty()) {
        qWarning() << "Unable to find campaign name:" << this->campaignShortName;
        return;
    }

    this->campaignName = campaignNameString;
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
    for (const QString &campaignFilename : std::as_const(campaignFiles)) {
        // Try to load this file as a campaign
        Campaign *campaignFile = new Campaign(campaignFileDir.absoluteFilePath(campaignFilename));

        if (campaignFile->isValid()) {
            list << campaignFile;
        }

        qDebug() << "Campaign:" << campaignFile->toString();
    }

    return list;
}
