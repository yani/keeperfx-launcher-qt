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
    QTextStream stream(&device);

    // Write key-value pairs
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        stream << it.key() << "=" << it.value().toString() << "\n";
    }

    return true;
}
