#include <QSaveFile>
#include <QFile>

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
    // Original settings file content
    QString originalContent;

    // Try to get the filename
    QString fileName;
    if (QSaveFile *sf = qobject_cast<QSaveFile*>(&device)) {
        fileName = sf->fileName();
    } else if (QFile *f = qobject_cast<QFile*>(&device)) {
        fileName = f->fileName();
    }

    // Check if we got the filename of the device
    if (!fileName.isEmpty()) {
        QFile readFile(fileName);
        if (readFile.open(QIODevice::ReadOnly | QIODevice::Text)) {

            // Read the original content
            QTextStream in(&readFile);
            originalContent = in.readAll();
            readFile.close();
        }
    }

    // Split into lines
    QStringList lines = originalContent.split('\n');
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
                if(fileName.endsWith("keeperfx-launcher-qt.cfg") == false && fileName.endsWith("launcher.cfg") == false){
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
