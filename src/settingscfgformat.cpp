#include <QSaveFile>
#include <QFile>
#include <QCoreApplication>
#include <QDir>

#include "settingscfgformat.h"

QSettings::Format SettingsCfgFormat::registerFormat()
{
    return QSettings::registerFormat("cfg", readFile, writeFile);
}

bool SettingsCfgFormat::readFile(QIODevice &device, QSettings::SettingsMap &map)
{
    QTextStream stream(&device);
    QString line;

    while (!stream.atEnd()) {
        line = stream.readLine().trimmed();

        // Skip empty lines or comments
        if (line.isEmpty() || line.startsWith(";") || line.startsWith("#")) {
            continue;
        }

        // Split the line into key and value
        QStringList keyValue = line.split('=', Qt::KeepEmptyParts);
        if (keyValue.size() == 2) {
            map.insert(keyValue[0].trimmed(), keyValue[1].trimmed());
        }
    }

    return true;
}

bool SettingsCfgFormat::writeFile(QIODevice &device, const QSettings::SettingsMap &map)
{
    // Original file content
    // We will use this as a template to update
    QString contents;

    // Try to get the filename
    QString filePath;
    if (QSaveFile *sf = qobject_cast<QSaveFile*>(&device)) {
        filePath = sf->fileName();
    } else if (QFile *f = qobject_cast<QFile*>(&device)) {
        filePath = f->fileName();
    }

    // Check if we got the filename of the device
    if (!filePath.isEmpty()) {

        // Check if this is the standard KeeperFX config file
        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();
        if(fileName == "keeperfx.cfg") {
            // Check for the defaults file
            // We can use this as a template so we always get all the new comments in it after updates
            QFile kfxDefaultConfigFile(QCoreApplication::applicationDirPath() + QDir::separator() + "_keeperfx.cfg");
            if(kfxDefaultConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                contents = kfxDefaultConfigFile.readAll();
            }
        }

        if(contents.isEmpty()){

            // Read the original content of the current config file
            QFile readFile(filePath);
            if (readFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&readFile);
                contents = in.readAll();
                readFile.close();
            }
        }
    }

    // Split into lines
    QStringList lines = contents.split('\n');
    QSet<QString> updatedKeys;

    // Copy the map so we can track unhandled keys
    QSettings::SettingsMap tmpMap = map;

    // Update existing keys
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();

        // Skip comments and empty lines
        if (line.isEmpty() || line.startsWith(";") || line.startsWith("#"))
            continue;

        int idx = line.indexOf('=');
        if (idx <= 0)
            continue;

        QString key = line.left(idx).trimmed();

        if (tmpMap.contains(key)) {
            QString newValue = tmpMap[key].toString();
            lines[i] = key + "=" + newValue;
            updatedKeys.insert(key);
        }
    }

    // Prepare final output
    QStringList output = lines;

    // Add missing keys at the end with a double newline
    // Launcher configs are ignored
    bool firstMissing = true;

    for (auto it = tmpMap.constBegin(); it != tmpMap.constEnd(); ++it) {
        if (!updatedKeys.contains(it.key())) {

            if (firstMissing) {

                // Don't add newlines in launcher config
                if(filePath.endsWith("keeperfx-launcher-qt.cfg") == false && filePath.endsWith("launcher.cfg") == false){
                    output.append("");
                    output.append("");
                }

                firstMissing = false;
            }

            output.append(it.key() + "=" + it.value().toString());
        }
    }

    // Trim leading empty lines
    // Sometimes non commented cfg files have them
    while (!output.isEmpty() && output.first().trimmed().isEmpty()) {
        output.removeFirst();
    }

    // Write back to QSaveFile
    QTextStream out(&device);
    for (int i = 0; i < output.size(); ++i) {
        out << output[i];
        if (i + 1 < output.size())
            out << "\n";
    }

    return true;
}
